/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	ims332.c,v $
 * Revision 2.6  93/01/14  17:16:29  danner
 * 	Added support for different monitors.
 * 	[92/11/30            jtp]
 * 
 * Revision 2.4.1.2  92/09/25  13:09:15  af
 * 	Reversed Red and Blue, so it works with color Maxines.
 * 	The chip spec doesn't really say where r, g and b go.
 * 	I'm also a little confused about whether r, g and b are
 * 	16 or 8 bit quantities here.  For now, assume they are 8,
 * 	because that appears to work.
 * 	[92/09/25            moore]
 * 
 * Revision 2.5  92/05/22  15:48:03  jfriedl
 * 	Some fields in user_info_t got renamed.
 * 	[92/05/13  20:40:39  af]
 * 
 * Revision 2.4  92/05/05  10:04:55  danner
 * 	Unconditionally do the reset at init time.
 * 	Fixed blooper in loading cursor colors.
 * 	[92/04/14  11:53:29  af]
 * 
 * Revision 2.3  92/03/05  11:36:47  rpd
 * 	Got real specs ( thanks Jukki!! ):
 * 	"IMS G332 colour video controller"
 * 	1990 Databook, pp 139-163, Inmos Ltd.
 * 	[92/03/03            af]
 * 
 * Revision 2.2  92/03/02  18:32:50  rpd
 * 	Created stub copy to make things compile.
 * 	[92/03/02            af]
 * 
 */
/*
 *	File: ims332.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	1/92
 *
 *	Routines for the Inmos IMS-G332 Colour video controller
 */

#include "IMS332.H"

/*
 * Generic register access
 */

unsigned int
ims332_read_register(regs, regno)
	ims332_padded_regmap_t regs;
	int regno;
{
	ims332_padded_regmap_t rptr;
	rptr = regs + (regno * 4);  /* 32 bit word addressing */
	return *rptr;
}

void ims332_write_register(regs, regno, val)
	ims332_padded_regmap_t regs;
	int	regno;
	register unsigned int	val;
{
	ims332_padded_regmap_t wptr;
	wptr = regs + (regno * 4);  /* 32 bit word addressing */
    *wptr = val;
}

/*
 * Color map
 */
void ims332_load_colormap( regs, map)
	ims332_padded_regmap_t	regs;
	color_map_t				*map;
{
	register int    i;

	for (i = 0; i < 256; i++, map++)
		ims332_load_colormap_entry(regs, i, map);
}

void ims332_load_colormap_entry( regs, entry_, map)
	ims332_padded_regmap_t	regs;
	int entry_;
	color_map_t	*map;
{
	ims332_write_register(regs, IMS332_REG_LUT_BASE + (entry_ & 0xff),
			      (map->blue << 16) |
			      (map->green << 8) |
			      (map->red));
}

/*
 * Video on/off
 *
 */

void ims332_video_off(vstate)
	struct vstate	*vstate;
{
	register ims332_padded_regmap_t	regs = vstate->regs;
	register unsigned		*save, csr;

	if (vstate->off)
		return;

	ims332_write_register(regs, IMS332_REG_LUT_BASE, 0);

	ims332_write_register( regs, IMS332_REG_COLOR_MASK, 0);

	/* cursor now */
	csr = ims332_read_register(regs, IMS332_REG_CSR_A);
	csr |= IMS332_CSR_A_DISABLE_CURSOR;
	ims332_write_register(regs, IMS332_REG_CSR_A, csr);

	vstate->off = 1;
}

void ims332_video_on(vstate)
	struct vstate	*vstate;
{
	register ims332_padded_regmap_t	regs = vstate->regs;
	register unsigned		*save, csr;

	if (!vstate->off)
		return;

	ims332_write_register( regs, IMS332_REG_COLOR_MASK, 0xffffffff);

	/* cursor now */
	csr = ims332_read_register(regs, IMS332_REG_CSR_A);
	csr &= ~IMS332_CSR_A_DISABLE_CURSOR;
	ims332_write_register(regs, IMS332_REG_CSR_A, csr);

	vstate->off = 0;
}

/*
 * Cursor
 */
void ims332_pos_cursor(regs,x,y)
	ims332_padded_regmap_t	regs;
	register int	x,y;
{
	ims332_write_register( regs, IMS332_REG_CURSOR_LOC,
		((x & 0xfff) << 12) | (y & 0xfff) );
}


void ims332_cursor_color( regs, color)
	ims332_padded_regmap_t	regs;
	color_map_t	*color;
{
	/* Bg is color[0], Fg is color[1] */
	ims332_write_register(regs, IMS332_REG_CURSOR_LUT_0,
			      (color->blue << 16) |
			      (color->green << 8) |
			      (color->red));
	color++;
	ims332_write_register(regs, IMS332_REG_CURSOR_LUT_1,
			      (color->blue << 16) |
			      (color->green << 8) |
			      (color->red));
}

void ims332_cursor_sprite( regs, cursor)
	ims332_padded_regmap_t	regs;
	unsigned short		*cursor;
{
	register int i;

	/* We *could* cut this down a lot... */
	for (i = 0; i < 512; i++, cursor++)
		ims332_write_register( regs,
			IMS332_REG_CURSOR_RAM+i, *cursor);
}

void pretend_usleep(delay) 
	int delay;
{
	int i;
	for (i=0; i < 10000*delay; i++);
}

void ims332_init(regs, mon)
	ims332_padded_regmap_t	regs;
	MONITOR_TYPE *mon;
{
	int CSRA = 0;

    /* PLL multipler in bits 0..4 (values from 5 to 31 allowed) */
    /* B438 TRAM derives clock from TRAM clock (5MHz) */
    int clock = 5;
    int pll_multiplier = mon->frequency/clock;
	ims332_write_register(regs, IMS332_REG_BOOT, pll_multiplier | IMS332_BOOT_CLOCK_PLL);
    pretend_usleep(100);

	/* disable VTG */
	ims332_write_register(regs, IMS332_REG_CSR_A, 0);
    pretend_usleep(100);

    /*
	B438 magic from f003e
        0x08 = VRAM SRAM style=Split SAM
        0x02 = Sync on Green only
        0x01 = External pixel sampling mode)
	*/
	ims332_write_register(regs, IMS332_REG_CSR_B, 0xb);

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

    CSRA |= IMS332_CSR_A_DISABLE_CURSOR;
    CSRA |= IMS332_BPP_8;
    CSRA |= IMS332_CSR_A_PIXEL_INTERLEAVE;
    CSRA |= IMS332_VRAM_INC_1024;
    CSRA |= IMS332_CSR_A_PLAIN_SYNC;
    CSRA |= IMS332_CSR_A_VTG_ENABLE;

    CSRA |= IMS332_CSR_A_SEPARATE_SYNC;
    CSRA |= IMS332_CSR_A_VIDEO_ONLY;

	ims332_write_register(regs, IMS332_REG_CSR_A, CSRA);
}

void B438_reset_G335(void) {
	int *reset_reg = (int *)BOARD_REG_BASE;
    *reset_reg = 0;
    *reset_reg = 1;
    pretend_usleep(1);
    *reset_reg = 0;
}
