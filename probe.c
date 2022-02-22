
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

int poke(uint32_t addr, uint32_t val) {
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

	/* CLOCKIN appears to receive a 6.25 Mhz clock --> PLL 12 for 75Mhz monitor */
	ims332_write_register(regs, IMS332_REG_BOOT, 12 | IMS332_BOOT_CLOCK_PLL);

	/* initialize VTG */
	ims332_write_register(regs, IMS332_REG_CSR_A,
				IMS332_BPP_8 | IMS332_CSR_A_DISABLE_CURSOR);
	/* TODO delay(50);	/* spec does not say */

	/* datapath registers (values taken from prom's settings) */

	frontporch = mon->line_time - (mon->half_sync * 2 +
				       mon->back_porch +
				       mon->frame_visible_width / 4);

	shortdisplay = mon->line_time / 2 - (mon->half_sync * 2 +
					     mon->back_porch + frontporch);
	broadpulse = mon->line_time / 2 - frontporch;

	ims332_write_register( regs, IMS332_REG_HALF_SYNCH,     mon->half_sync);
	ims332_write_register( regs, IMS332_REG_BACK_PORCH,     mon->back_porch);
	ims332_write_register( regs, IMS332_REG_DISPLAY,
			      mon->frame_visible_width / 4);
	ims332_write_register( regs, IMS332_REG_SHORT_DIS,	shortdisplay);
	ims332_write_register( regs, IMS332_REG_BROAD_PULSE,	broadpulse);
	ims332_write_register( regs, IMS332_REG_V_SYNC,		mon->v_sync * 2);
	ims332_write_register( regs, IMS332_REG_V_PRE_EQUALIZE,
			      mon->v_pre_equalize);
	ims332_write_register( regs, IMS332_REG_V_POST_EQUALIZE,
			      mon->v_post_equalize);
	ims332_write_register( regs, IMS332_REG_V_BLANK,	mon->v_blank * 2);
	ims332_write_register( regs, IMS332_REG_V_DISPLAY,
			      mon->frame_visible_height * 2);
	ims332_write_register( regs, IMS332_REG_LINE_TIME,	mon->line_time);
	ims332_write_register( regs, IMS332_REG_LINE_START,	mon->line_start);
	ims332_write_register( regs, IMS332_REG_MEM_INIT, 	mon->mem_init);
	ims332_write_register( regs, IMS332_REG_XFER_DELAY,	mon->xfer_delay);

	ims332_write_register( regs, IMS332_REG_COLOR_MASK, 0xffffff);

	//ims332_init_colormap( regs );

	ims332_write_register(regs, IMS332_REG_CSR_A,
		IMS332_BPP_8 | IMS332_CSR_A_DMA_DISABLE | IMS332_CSR_A_VTG_ENABLE);

}

int main(int argc, char **argv) {
    int i,aok = 1;
    char *s;

    XFCB_MONITOR_TYPE mon = { (const char *)"VRM17", 1024, 768, 1024, 1024, 16, 33, 6, 2, 2, 21, 326, 16, 10, 10 };

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    init_lkio();
    rst_adpt();

    B438_reset_G335();
    probe_ims332_init (0, &mon);

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

    /*test (0x90000000, 4);
    test (0xA0000000, 4);
    test (0xB0000000, 4);
    test (0xC0000000, 4);
    test (0xD0000000, 4);
    test (0xE0000000, 4);
    test (0xF0000000, 4);
    test (0x80000000, 4096);    // internal memory
    */
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



