#include <linux/platform_device.h>
#include <linux/leds.h>
#include "s3cfb.h"

#include "../../urbetter/power_gpio.h"

#define MAX_LCD_COUNT  64
static struct s3cfb_lcd * s_lcd[MAX_LCD_COUNT];
static int s_lcd_count = 0;
char s_selected_lcd_name[32] = {'\0'};
EXPORT_SYMBOL(s_selected_lcd_name);

int ut_register_lcd(struct s3cfb_lcd *lcd)
{
	if(!lcd)
		return -1;

	if(s_lcd_count >= MAX_LCD_COUNT - 1){
		printk("ut_register_lcd: can not add lcd: %s, reach max count.\n", lcd->name);
		return -1;
	}

	s_lcd[s_lcd_count] = lcd;
	s_lcd_count++;
	printk("ut_register_lcd: add lcd: %s\n", lcd->name);	
	return 0;
}


static int __init select_lcd_name(char *str)
{
	printk("select_lcd_name: str=%s\n", str);
	strcpy(s_selected_lcd_name, str);
	return 0;
}

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	int i;
	for(i = 0; i < s_lcd_count; i++)
	{
		if(!strcmp(s_lcd[i]->name, s_selected_lcd_name))
		{
			ctrl->lcd = s_lcd[i];
			return;
		}
	}

	//could not find lcd, use default.
	ctrl->lcd = s_lcd[0];
}


__setup("lcd=", select_lcd_name);


//******************** lcd parameter begin ************************
static void ut_lcd_init(void)
{
	static int s_init = 0;
	printk("LCD ut lcd selecter init.");
	if(!s_init)
	{
		write_power_item_value(POWER_LCD33_EN, 1);
		write_power_item_value(POWER_LVDS_PD, 1);
		s_init = 1;
	}
}

static struct s3cfb_lcd s_ut_lcd_param[] = 
{
	{
		.name = "wa101",
		.init_ldi = ut_lcd_init,
		.width	= 1024,
		.height	= 768,
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
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},
	{
		.name = "ut7gm",
		.init_ldi = ut_lcd_init,
		.width = 800,
		.height = 480,
		.bpp = 32/*24*/,
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
	},
	{
		.name = "ut8gm",
		.init_ldi = ut_lcd_init,
		.width = 800,
		.height = 480,
		.bpp = 32/*24*/,
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
	},
	{
		.name = "wa101hd",
		.init_ldi = ut_lcd_init,
		.width	= 1280,
		.height	= 800,
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
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},

};

static int __init regiser_lcd(void)
{
	int i;
	for(i = 0; i < sizeof(s_ut_lcd_param) / sizeof(s_ut_lcd_param[0]); i++)
		ut_register_lcd(&(s_ut_lcd_param[i]));
	return 0;
}

early_initcall(regiser_lcd);

