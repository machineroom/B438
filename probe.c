
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

void poke_words(uint32_t addr, uint32_t count, uint32_t val) {
    for (int i=0; i < count; i++) {
        poke (addr, val);
        addr += 4;
    }
}

#define BOARD_REG_BASE 0x200000

void B438_reset_G335(void) {
    poke (BOARD_REG_BASE,0);
    poke (BOARD_REG_BASE,1);
    usleep(1);
    poke (BOARD_REG_BASE,0);
    usleep(1);  // >50ns
}

void ims332_write_register(uint32_t regs, int regno, unsigned int val)
{
	uint32_t wptr;

	wptr = regs + (regno * 4);  //32 bit word addressing
    poke (wptr, val);
}

static void probe_ims332_init(uint32_t regs, MONITOR_TYPE *mon)
{
	int shortdisplay;
	int broadpulse;
	int frontporch;
    int display;

    // PLL multipler in bits 0..4 (values from 5 to 31 allowed)
    /* B438 TRAM derives clock from TRAM clock (5MHz) */
    int clock = 5;
    int pll_multiplier = mon->frequency/clock;
    assert (pll_multiplier>=5);
    assert (pll_multiplier<=31);
	ims332_write_register(regs, IMS332_REG_BOOT, pll_multiplier | IMS332_BOOT_CLOCK_PLL);
    usleep(100);

	/* disable VTG */
	ims332_write_register(regs, IMS332_REG_CSR_A, 0);
    usleep(100);

    //B438 magic from f003e
        //0x08 = VRAM SRAM style=Split SAM
        //0x02 = Sync on Green only
        //0x01 = External pixel sampling mode)
	ims332_write_register(regs, IMS332_REG_CSR_B, 0xb);


    printf ("programming G335 data path registers:\n");
    printf ("\tline_time\t%d\n", mon->line_time);
    printf ("\thalf_sync\t%d\n", mon->half_sync);
    printf ("\tback_porch\t%d\n", mon->back_porch);
    printf ("\tdisplay\t\t%d\n", mon->display);
    printf ("\tshortdisplay\t%d\n", mon->short_display);
    printf ("\tv_display\t%d\n", mon->v_display);
    printf ("\tv_blank\t\t%d\n", mon->v_blank);
    printf ("\tv_sync\t\t%d\n", mon->v_sync);
    printf ("\tv_pre_equalize\t%d\n", mon->v_pre_equalize);
    printf ("\tv_post_equalize\t%d\n", mon->v_post_equalize);
    printf ("\tbroadpulse\t%d\n", mon->broad_pulse);
    printf ("\tmem_init\t%d\n", mon->mem_init);
    printf ("\txfer_delay\t%d\n", mon->xfer_delay);
    printf ("\tline_start\t%ld\n", mon->line_start);
    

	ims332_write_register( regs, IMS332_REG_LINE_TIME,	    mon->line_time);
	ims332_write_register( regs, IMS332_REG_HALF_SYNCH,     mon->half_sync);
	ims332_write_register( regs, IMS332_REG_BACK_PORCH,     mon->back_porch);
	ims332_write_register( regs, IMS332_REG_DISPLAY,        mon->display);
	ims332_write_register( regs, IMS332_REG_SHORT_DIS,	    mon->short_display);
	ims332_write_register( regs, IMS332_REG_V_DISPLAY,      mon->v_display);
	ims332_write_register( regs, IMS332_REG_V_BLANK,	    mon->v_blank);
	ims332_write_register( regs, IMS332_REG_V_SYNC,		    mon->v_sync);
	ims332_write_register( regs, IMS332_REG_V_PRE_EQUALIZE, mon->v_pre_equalize);
	ims332_write_register( regs, IMS332_REG_V_POST_EQUALIZE,mon->v_post_equalize);
	ims332_write_register( regs, IMS332_REG_BROAD_PULSE,	mon->broad_pulse);
	ims332_write_register( regs, IMS332_REG_MEM_INIT, 	    mon->mem_init);
	ims332_write_register( regs, IMS332_REG_XFER_DELAY,	    mon->xfer_delay);
	ims332_write_register( regs, IMS332_REG_LINE_START,	    mon->line_start);

	ims332_write_register( regs, IMS332_REG_COLOR_MASK, 0xffffff);

    int CSRA = 0;
    CSRA |= IMS332_CSR_A_DISABLE_CURSOR;
    CSRA |= IMS332_BPP_8;
    CSRA |= IMS332_CSR_A_PIXEL_INTERLEAVE;
    CSRA |= IMS332_VRAM_INC_1024;
    CSRA |= IMS332_CSR_A_PLAIN_SYNC;
    CSRA |= IMS332_CSR_A_VTG_ENABLE;

    //CSRA |= IMS332_CSR_A_SEPARATE_SYNC;
    //CSRA |= IMS332_CSR_A_VIDEO_ONLY;

    //B43011
    //101101000011000000010001
    // enable VTG
    // plain sync
    // VRAM addr inc=11
    // 18=interleaved pixel prot
    // 20,21,22 BPP=011
    // 23= disable cursor
	ims332_write_register(regs, IMS332_REG_CSR_A, CSRA);

}

void set_palette (uint32_t regs, int index, uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t red32 = red;
    uint32_t green32 = green;
    ims332_write_register(regs, IMS332_REG_LUT_BASE + (index & 0xff),
                (red32 << 16) |
                (green32 << 8) |
                blue);
}

int main(int argc, char **argv) {
    int i,aok = 1;
    char *s;
    #if 0
    char *name;
    short frequency;        /* dot clock MHz */
    short line_time;        /* screen units (= 4 pixels) */
    short half_sync;        
    short back_porch;
    short display;
    short short_display;
    short v_display;
    short v_blank;
    short v_sync;           /* lines */
    short v_pre_equalize;
    short v_post_equalize;
    short broad_pulse;
    short mem_init;
    short xfer_delay;
    long  line_start;
    #endif

    MONITOR_TYPE vga = { 
                        (const char *)"VGA", 
                        25,        //frequency (MHz)
                        198,
                        12,
                        12,
                        160,
                        62,
                        960,
                        64,
                        4,
                        22,
                        4,
                        75,
                        512,
                        1,
                        0
                       };

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    init_lkio();
    rst_adpt();

    B438_reset_G335();
    uint32_t regs = 0;
    probe_ims332_init (regs, &vga);

    //B438 equipped with:
    // 8 * NEC B424400 DRAM 1Mb*4 bit = 4MB DRAM
    // 8 * NEC D482234 VRAM 256K*8 bit = 2MB VRAM

    // B438 'guessed' map:
    // G335          00000000-7FFFFFFF (!CS asserted on write)
    // memint        80000000-80000FFF
    // RAM           80001000-805FFFFF  (80001000 start of T805 external memory)    6MB DRAM+VRAM
    // DRAM+VRAM repeat in -ve memory (i.e 0x90001000..0xF0001000)
    // 335 reset reg 7FF00000 (0 reset low, 1 reset high - active high)

    // actual map (from F003e)
    // VRAM 0x8040000 - 0x805FFFFF
    // board control 0x2000000
    // CVC (G335) 0x00000000


    //setup colour palette
    printf ("set palette\n");
    // 0 = black
    set_palette (regs, 0, 0, 0, 0);
    // F = grey
    //TODO something odd here - 0,0,0 produces a dirty green. Sync on green?
    set_palette (regs, 0xF, 40, 40, 40);
    // 1 = red
    set_palette (regs, 1, 255, 0, 0);
    // 2 = green
    set_palette (regs, 2, 0, 255, 0);
    // 3 = blue
    set_palette (regs, 3, 0, 0, 255);
    // 4 = yellow
    set_palette (regs, 4, 255, 255, 0);
    // 5 = white
    set_palette (regs, 5, 255, 255, 255);

    poke_words(0x80400000, 640*60, 0x0F0F0F0F);
    poke_words(0x80400000+(640*2*4),640/2,0x01010101);
    poke_words(0x80400000+(640*4*4),640/2,0x02020202);
    poke_words(0x80400000+(640*6*4),640/2,0x03030303);
    poke_words(0x80400000+(640*8*4),640/2,0x04040404);
    poke_words(0x80400000+(640*10*4),640/2,0x05050505);
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



