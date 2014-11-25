/* linux/drivers/video/samsung/s3cfb_wa101s.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * 101WA01S 10.1" Landscape LCD module driver for the SMDK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "s3cfb.h"

static struct s3cfb_lcd lcd7a = {
//	.width	= 1366,
/*	.width	= 800,
	.height	= 480,
	.bpp	= 24,
	.freq	= 60,

	.timing = {
		.h_fp = 260,
		.h_bp = 480,
		.h_sw = 36,
		.v_fp = 16,
		.v_fpe = 1,
		.v_bp = 6,
		.v_bpe = 1,
		.v_sw = 3,
	},

	.polarity = {
		.rise_vclk	= 0,
		.inv_hsync	= 1,
		.inv_vsync	= 1,
		.inv_vden	= 0,
	},  */
	.width = 800,
        .height = 480,
        .bpp = 32,
        .freq = 60,

	.timing = {
		.h_fp = 10,
		.h_bp = 78,
		.h_sw = 10,
		.v_fp = 30,
		.v_fpe = 1,
		.v_bp = 30,
		.v_bpe = 1,
		.v_sw = 2,
	},
        .polarity = {
                .rise_vclk = 0,
                .inv_hsync = 1,
                .inv_vsync = 1,
                .inv_vden = 0,
        },
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	lcd7a.init_ldi = NULL;
	ctrl->lcd = &lcd7a;
}
