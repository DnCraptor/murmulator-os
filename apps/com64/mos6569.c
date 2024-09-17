#include "mos6569.h"

// Rahmen Y Start und Stop
//RSEL=0
#define BORDER_RSEL0_YSTART 55
#define BORDER_RSEL0_YSTOP  247

// RSEL=1
#define BORDER_RSEL1_YSTART 51
#define BORDER_RSEL1_YSTOP  251

// Rahmen X Start und Stop
//CSEL=0
#define BORDER_CSEL0_LEFT   18  // Eigtl XKoordinate 0x01f  (18)
#define BORDER_CSEL0_RIGHT  56  // Eigtl XKoordinate 0x14f  (56)

// CSEL=1
#define BORDER_CSEL1_LEFT   17  // Eigtl XKoordinate 0x018  (17)
#define BORDER_CSEL1_RIGHT  57  // Eigtl XKoordinate 0x158  (57)

// erste and letzte mögliche Rasterzeile für Bad Lines
#define FIRST_DMA_LINE		0x30
#define LAST_DMA_LINE		0xf7

// länge der X Werte der Destination BitMap
#define XMOD				504
#define YMOD                312

static void VICII_VICII(VICII* p)
{
    p->video_buffer = get_buffer(); /// p->video_buffer_back[0];
    p->current_cycle = 1;
    // XKoordinaten in Tabelle schreiben
    uint16_t x_pos = 0x0194;
    for(int i = 1; i < 14; ++i)
    {
        p->x_coordinates_table[i] = x_pos;
        x_pos += 8;
    }

    x_pos = 0x0004;
    for(int i = 14; i < 64; ++i)
    {
        p->x_coordinates_table[i] = x_pos;
        x_pos += 8;
    }

	VICII_SetVicType(p, 0);

    p->sprite_y_exp_flip_flop = 0xFF;

    uint16_t spr_x_start = 0x194;
    for(uint16_t x = 0; x < 0x200; ++x)
	{
        p->sprite_x_display_table[spr_x_start]=x;
        if(x < 107) p->sprite_x_display_table[spr_x_start] += 8;
		spr_x_start++;
		spr_x_start &= 0x1FF;
	}

    for(int i = 0; i < VIC_CONFIG_NUM; ++i)
        p->vic_config[i] = true;

    p->first_display_line_pal = 16;                       //VICE - 16, Emu64 - 26
    p->last_display_line_pal = 288;                       //VICE - 288, Emu64 - 292

    p->first_display_line_ntsc = 30;
    p->last_display_line_ntsc = 288;

    p->first_display_line = p->first_display_line_pal;
    p->last_display_line = p->last_display_line_pal;

    p->h_border_compare_left[0]=BORDER_CSEL0_LEFT;
    p->h_border_compare_right[0]=BORDER_CSEL0_RIGHT;
    p->v_border_compare_top[0]=BORDER_RSEL0_YSTART;
    p->v_border_compare_bottom[0]=BORDER_RSEL0_YSTOP;

    p->h_border_compare_left[1]=BORDER_CSEL1_LEFT;
    p->h_border_compare_right[1]=BORDER_CSEL1_RIGHT;
    p->v_border_compare_top[1]=BORDER_RSEL1_YSTART;
    p->v_border_compare_bottom[1]=BORDER_RSEL1_YSTOP;
}

static void VICII_SetVicType(VICII* p, uint8_t system)
{
    p->system = system;

    switch(system)
	{
	case 0:
        p->total_rasterlines=312;
        p->total_cycles_per_rasterline=63;
        p->total_x_coordinate=504;
        p->raster_y = p->total_rasterlines - 1;
        p->first_display_line = p->first_display_line_pal;
        p->last_display_line = p->last_display_line_pal;
		break;
	case 1:
        p->total_rasterlines=262;
        p->total_cycles_per_rasterline=64;
        p->total_x_coordinate=512;
        p->raster_y = p->total_rasterlines - 1;
        p->first_display_line = p->first_display_line_ntsc;
        p->last_display_line = p->last_display_line_ntsc;
		break;
	case 2:
        p->total_rasterlines=263;
        p->total_cycles_per_rasterline=65;
        p->total_x_coordinate=520;
        p->raster_y = p->total_rasterlines - 1;
        p->first_display_line = p->first_display_line_ntsc;
        p->last_display_line = p->last_display_line_ntsc;
		break;
	}
}
