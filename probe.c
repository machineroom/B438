
#include <stdio.h>
#include <math.h>
#include "LKIO.H"
#include <stdint.h>
#include <cstring>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include "c011.h"
#include <gflags/gflags.h>
#include "IMS332.H"


#define JOBCOM 0L
#define PRBCOM 1L
#define DATCOM 2L
#define RSLCOM 3L
#define FLHCOM 4L

#define TRUE 1
#define FALSE 0


DEFINE_bool (verbose, false, "print verbose messages during initialisation");

int test(uint32_t addr, int count) {
    printf ("testing %d bytes from 0x%X\n", count, addr);
    // count is in bytes. read & write 4 byte words
    for (int i=0; i < count; i+=4) {
        uint32_t out = 0x55AA55AA;
        byte_out (0);  // poke
        word_out (addr);
        word_out (out);

        byte_out (1);   // peek
        word_out (addr);
        uint32_t in=0;
        word_in (&in);
        if (out != in ) {
            printf ("*E* wrote 0x%X read 0x%X at 0x%X\n", out, in, addr);
            return -1;
        }
        //printf ("%d\n",i);
        if (i%4096==0) {
            printf ("."); fflush(stdout);
        }
        out = ~out;
        addr += 4;
    }
    printf ("\n");
    return 0;
}

void poke(uint32_t addr, uint32_t val) {
    printf ("poking 0x%X to 0x%X\n", val, addr);
    byte_out (0);  // poke
    word_out (addr);
    word_out (val);
}

void B438_reset_G335(void) {
    poke (0x7FF00000,0);
    poke (0x7FF00000,1);
    usleep(1);
    poke (0x7FF00000,0);
    usleep(1);  // >50ns
}

void ims332_write_register(uint32_t regs, int regno, unsigned int val)
{
	uint32_t wptr;

	wptr = regs + (regno * 4);  //32 bit word addressing
    poke (wptr, val);
}

static void probe_ims332_init(uint32_t regs, xcfb_monitor_type_t mon)
{
	int shortdisplay;
	int broadpulse;
	int frontporch;
    int display;

	/* CLOCKIN appears to receive a 6.25 Mhz clock --> PLL 12 for 75Mhz monitor */
	//ims332_write_register(regs, IMS332_REG_BOOT, 12 | IMS332_BOOT_CLOCK_PLL);

    // PLL multipler in bits 0..4 (values from 5 to 31 allowed)
    /* B438 TRAM derives clock from TRAM clock (5MHz) */
    int clock = 5;
    int pll_multiplier = mon->frequency/clock;
    assert (pll_multiplier>=5);
    assert (pll_multiplier<=31);
	ims332_write_register(regs, IMS332_REG_BOOT, pll_multiplier | IMS332_BOOT_CLOCK_PLL);

    int CSRA = IMS332_BPP_8 | IMS332_CSR_A_DISABLE_CURSOR | IMS332_CSR_A_DMA_DISABLE;   // sync on green
    CSRA | IMS332_CSR_A_BLANK_DISABLE;
    CSRA |= IMS332_CSR_A_CBLANK_IS_OUT;     // sync on green still produces picture
    CSRA |= IMS332_CSR_A_PLAIN_SYNC;        // sync on green still produces picture
    CSRA |= IMS332_CSR_A_SEPARATE_SYNC;     // sync on green still produces picture
    //CSRA |= IMS332_CSR_A_VIDEO_ONLY;

	/* initialize VTG */
	ims332_write_register(regs, IMS332_REG_CSR_A, CSRA);
	/* TODO delay(50);	/* spec does not say */

	frontporch = mon->line_time - (mon->half_sync * 2 +
				       mon->back_porch +
				       mon->frame_visible_width / 4);

	shortdisplay = mon->line_time / 2 - (mon->half_sync * 2 +
					     mon->back_porch + frontporch);
	broadpulse = mon->line_time / 2 - frontporch;
    display = mon->frame_visible_width / 4;
    
    // as per Inmos graphics databook 2nd edition pp 154
    /*if (display <= mon->line_time/2) {
        printf ("timing calc error (display = %d, line_time=%d(line_time/2=%d)\n", display, mon->line_time, mon->line_time/2);
        return;
    }*/

    printf ("programming G335 data path registers:\n");
    printf ("\thalf_sync\t%d\n", mon->half_sync);
    printf ("\tback_porch\t%d\n", mon->back_porch);
    printf ("\tdisplay\t\t%d\n", display);
    printf ("\tshortdisplay\t%d\n", shortdisplay);
    printf ("\tbroadpulse\t%d\n", broadpulse);
    printf ("\tv_sync\t\t%d\n", mon->v_sync * 2);
    printf ("\tv_pre_equalize\t%d\n", mon->v_pre_equalize);
    printf ("\tv_post_equalize\t%d\n", mon->v_post_equalize);
    printf ("\tv_blank\t\t%d\n", mon->v_blank*2);
    printf ("\tframe_visible_height\t%d\n", mon->frame_visible_height*2);
    printf ("\tline_time\t%d\n", mon->line_time);
    printf ("\tline_start\t%d\n", mon->line_start);
    printf ("\tmem_init\t%d\n", mon->mem_init);
    printf ("\txfer_delay\t%d\n", mon->xfer_delay);
    

	ims332_write_register( regs, IMS332_REG_HALF_SYNCH,     mon->half_sync);
	ims332_write_register( regs, IMS332_REG_BACK_PORCH,     mon->back_porch);
	ims332_write_register( regs, IMS332_REG_DISPLAY,        display);
	ims332_write_register( regs, IMS332_REG_SHORT_DIS,	    shortdisplay);
	ims332_write_register( regs, IMS332_REG_BROAD_PULSE,	broadpulse);
	ims332_write_register( regs, IMS332_REG_V_SYNC,		    mon->v_sync * 2);
	ims332_write_register( regs, IMS332_REG_V_PRE_EQUALIZE, mon->v_pre_equalize);
	ims332_write_register( regs, IMS332_REG_V_POST_EQUALIZE,mon->v_post_equalize);
	ims332_write_register( regs, IMS332_REG_V_BLANK,	    mon->v_blank * 2);
	ims332_write_register( regs, IMS332_REG_V_DISPLAY,      mon->frame_visible_height * 2);
	ims332_write_register( regs, IMS332_REG_LINE_TIME,	    mon->line_time);
	ims332_write_register( regs, IMS332_REG_LINE_START,	    mon->line_start);
	ims332_write_register( regs, IMS332_REG_MEM_INIT, 	    mon->mem_init);
	ims332_write_register( regs, IMS332_REG_XFER_DELAY,	    mon->xfer_delay);

	ims332_write_register( regs, IMS332_REG_COLOR_MASK, 0xffffff);

	//ims332_init_colormap( regs );

	ims332_write_register(regs, IMS332_REG_CSR_A, CSRA | IMS332_CSR_A_VTG_ENABLE);

}

int main(int argc, char **argv) {
    int i,aok = 1;
    char *s;
    #if 0
    short frame_visible_width; /* pixels */
    short frame_visible_height;
    short frame_scanline_width;
    short frame_height;
    short half_sync;        /* screen units (= 4 pixels) */
    short back_porch;
    short v_sync;           /* lines */
    short v_pre_equalize;
    short v_post_equalize;
    short v_blank;
    short line_time;        /* screen units */
    short line_start;
    short mem_init;         
    short xfer_delay;
    #endif

    XFCB_MONITOR_TYPE mon = { (const char *)"VRM17", 75, 1024, 768, 1024, 1024, 16, 33, 6, 2, 2, 21, 326, 16, 10, 10 };
    // line_time=326
    // 1024/4=256       (256)
    // back_porch = 33  (289)
    // half_sync = 16 (305)
    // 326-305 = 21 units for front_porch

    // VGA back_porch = 48
    // VGA front_porch = 16
    //vga not working? no
    XFCB_MONITOR_TYPE vga = { (const char *)"VGA", 
                             25,        //frequency (MHz)
                             640,       //frame_visible_width (pixels)
                             480,       //frame_visible_height (pixels)
                             640,       //frame_scanline_width (pixels)
                             640,       //frame_width (pixels)
                             96/2,    //half_sync (screen units) (VGA HSync=96 clocks)
                             48,      //back_porch (screen units)
                             6,         //v_sync (lines)
                             2,         //v_pre_equalize (half lines)
                             2,         //v_post_equalize (half lines)
                             33,        //v_blank (lines)
                             200,       //line_time (screen units) = half_sync + back_porch + display + front_porch
                             16,        //line_start (screen units)
                             10,        //mem_init (screen units)
                             10         //xfer_delay (screen units)
                            };

    XFCB_MONITOR_TYPE vesa1280 = { (const char *)"1280", 
                             108,        //frequency (MHz)
                             1280,       //frame_visible_width (pixels)
                             1024,       //frame_visible_height (pixels)
                             1280,       //frame_scanline_width (pixels)
                             1280,       //frame_width (pixels)
                             112/2/4,    //half_sync (screen units) (VGA HSync=96 clocks)
                             248/4,      //back_porch (screen units)
                             3,         //v_sync (lines)
                             2,         //v_pre_equalize (half lines)
                             2,         //v_post_equalize (half lines)
                             33,        //v_blank (lines)
                             1688/4,       //line_time (screen units) = half_sync + back_porch + display + front_porch
                             3,        //line_start (screen units)
                             10,        //mem_init (screen units)
                             10         //xfer_delay (screen units)
                            };

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    init_lkio();
    rst_adpt();

    B438_reset_G335();
    probe_ims332_init (0, &vesa1280);

    //B438 equipped with:
    // 8 * NEC B424400 DRAM 1Mb*4 bit = 4MB DRAM
    // 8 * NEC D482234 VRAM 256K*8 bit = 2MB VRAM

    // B438 derived map:
    // G335          00000000-7FFFFFFF (!CS asserted on write)
    // memint        80000000-80000FFF
    // RAM           80001000-805FFFFF  (80001000 start of T805 external memory)    6MB DRAM+VRAM
    // DRAM+VRAM repeat in -ve memory (i.e 0x90001000..0xF0001000)
    // 335 reset reg 7FF00000 (0 reset low, 1 reset high - active high)
    /*
    uint32_t addr = 0x00000000;
    for (int i=0; i < 16; i++) {
      for (int j=0; j < 64*4; j += 4) {
        poke (addr+j,0);
        usleep(50*1000);
        poke (addr+j,-1);
        usleep(50*1000);
      }
      addr += 0x10000000;
    }
    exit (0);
    */

    /*uint32_t addr = 0x7FF00000;
    for (int i=0; i < 16; i++) {
      for (int j=0; j < 0xFFFF; j += 4) {
        poke (addr+j,0);
        usleep(1000*1000);
        poke (addr+j,1);
        usleep(1000*1000);
      }
      addr += 0x10000000;
    }*/

    int q=100*4096;
    test (0x90000000, q);
    test (0xA0000000, q);
    test (0xB0000000, q);
    test (0xC0000000, q);
    test (0xD0000000, q);
    test (0xE0000000, q);
    test (0xF0000000, q);
    //test (0x80001000, 8*1024*1024);    // start of DRAM
    return(0);
}


void memdump (char *buf, int cnt) {
    char *p = buf;
    int byte_cnt=0;
    char str[200] = {'\0'};
    while (cnt > 0) {
        char b[10];
        sprintf (b, "0x%02X ", *p++);
        strcat (str,b);
        if (byte_cnt++==16) {
            printf ("%s\n", str);
            str[0] = '\0';
            byte_cnt=0;
        }
        cnt--;
    }
}



