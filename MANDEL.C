/******************************** MANDEL.C **********************************
*  (C) Copyright 1987-1993  Computer System Architects, Provo UT.           *
*  This  program is the property of Computer System Architects (CSA)        *
*  and is provided only as an example of a transputer/PC program for        *
*  use  with CSA's Transputer Education Kit and other transputer products.  *
*  You may freely distribute copies or modifiy the program as a whole or in *
*  part, provided you insert in each copy appropriate copyright notices and *
*  disclaimer of warranty and send to CSA a copy of any modifications which *
*  you plan to distribute.						    *
*  This program is provided as is without warranty of any kind. CSA is not  *
*  responsible for any damages arising out of the use of this program.      *
****************************************************************************/

/****************************************************************************
* This program mandel.c is written in Logical System's C and Transputer     *
* assembly. This program provides fuctions to calculate mandelbrot loop     *
* in a fixed point or floating point airthmatic.                            *
* This program is compiled and output is converted to binary hex array and  *
* is down loaded onto transputer network by program MAN.C                   *
****************************************************************************/
/* compile: tcx file -cf1p0rv */

#include "d:\include\conc.h"

#include "IMS332.H"
/*{{{  notes on compiling and linking*/
/*
This module is compiled as indicated above so it will be relocatable.
It is then linked with main (not _main) specified as the entry point.
This is because the fload loader loads the code in and thens jumps
to the first byte of the code read in.  By specifying main as the
entry point the code is correctly linked and the first instruction of
main will be the first byte in the code.
*/
/*}}}  */

/*{{{  define constants*/
#define TRUE  1
#define FALSE 0


/*{{{  struct type def shared with loader and ident asembler code */
typedef struct {
    int id;
    void *minint;
    void *memstart;
    Channel *up_in;
    Channel *dn_out[3];
    void *ldstart;
    void *entryp;
    void *wspace;
    void *ldaddr;
    void *trantype;
} LOADGB;

/* from the f003e header. My old Dell LCD does lock to this */
MONITOR_TYPE vga = { 
                    "IBM VGA", 
                    25,
                    202,
                    8,
                    20,
                    160,
                    61,
                    960,
                    80,
                    4,
                    4,
                    4,
                    75,
                    512,
                    1,
                    0
                    };


/*
B438 map (from F003e)
VRAM 0x80400000 - 0x805FFFFF
board control 0x2000000
CVC (G335) 0x00000000
*/

void set(int *addr, int count, int val) {
    int i;
    for (i=0; i < count; i++) {
        *addr = val;
        addr += 4;
    }
}

main(ld)
LOADGB *ld;
{
    Channel *fd_in = ld->up_in;
    Channel *fd_out = ld->up_in-4;
    int cmd;

    ChanOutInt(fd_out,1);
    cmd = ChanInInt(fd_in);
    if (cmd == 0x41)
    {
        ChanOutInt(fd_out,0x42);
    }

    {
        /* address for memory mapped IMS335 on the B438 */
        ims332_padded_regmap_t regs = (ims332_padded_regmap_t)0x00000000;
        struct vstate state;
        B438_reset_G335();
        ims332_init(regs, &vga);
        {

            color_map_t m;
            m.red = m.green = m.blue = 0;
            ims332_load_colormap_entry( regs, 0, &m);
            m.red = m.green = m.blue = 255;
            ims332_load_colormap_entry( regs, 1, &m);
        }
        set(0x80400000, 640*480/4, 0);
        set(0x80400000, 640*100/4, 0x01010101);
    }
}


