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

static void VICII_WriteIO(VICII* p, uint16_t address, uint8_t value)
{
    address &= 63;
    switch (address)
	{
		/// Sprite X Position
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0A: case 0x0C: case 0x0E:
            p->reg_mx[address >> 1] = (p->reg_mx[address >> 1] & 0xFF00) | value;
		break;

		/// Sprite Y Position
		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0B: case 0x0D: case 0x0F:
            p->reg_my[address >> 1] = value;
		break;

		/// Sprite X Position MSB
		case 0x10: {
			int i, j;
                        p->reg_mx8 = value;
			for (i=0, j=1; i<8; i++, j<<=1)
			{
                                if (p->reg_mx8 & j) p->reg_mx[i] |= 0x100;
                                else p->reg_mx[i] &= 0xFF;
			}
			break;
        }
		// Control Register 1
		case 0x11:
                        p->reg_ctrl_1 = value;
                        p->reg_y_scroll = value & 7;

                        p->new_irq_raster = (uint16_t)((p->reg_irq_raster & 0xFF) | ((value & 0x80) << 1));
                        if (p->reg_irq_raster != p->new_irq_raster && p->current_rasterline == p->new_irq_raster) {
                            VICII_RasterIRQ(p);
                        }
                        p->reg_irq_raster = p->new_irq_raster;
			
                        p->graphic_mode = ((p->reg_ctrl_1 & 0x60) | (p->reg_ctrl_2 & 0x10)) >> 4;
		
			// Prüfen ob Badlines zugelassen sind
                        if ((p->current_rasterline == 0x30) && (value & 0x10)) p->badline_enable = true;

			// Prüfen auf Badline zustand
                        if((p->current_rasterline>=0x30) && (p->current_rasterline<=0xF7) && (p->reg_y_scroll == (p->current_rasterline&7)) && (p->badline_enable == true))
			{
                                p->badline_status = true;
			}
            else p->badline_status = false;

			// RSEL
            p->rsel = (value & 0x08)>>3;

			// DEN
            p->den = (value & 0x10)>>4;
            p->write_reg_0x11 = true;
			break;

		/// Rasterzähler
		case 0x12:
                        //p->reg_irq_raster = (p->reg_irq_raster & 0xFF00) | wert;

                        p->new_irq_raster = (p->reg_irq_raster & 0xFF00) | value;
                        if (p->reg_irq_raster != p->new_irq_raster && p->current_rasterline == p->new_irq_raster) {
                            VICII_RasterIRQ(p);
                        }
                        p->reg_irq_raster = p->new_irq_raster;
			break;

		/// Sprite Enable
		case 0x15:
            p->reg_me = value;
			break;

		// Control Register 2
		case 0x16:
            p->reg_ctrl_2 = value;
            p->reg_x_scroll = value & 7;
            p->graphic_mode = ((p->reg_ctrl_1 & 0x60) | (p->reg_ctrl_2 & 0x10)) >> 4;
			
            // p->csel
            p->csel = (value & 0x08)>>3;
			
			break;

		/// Sprite Y-Expansion
		case 0x17:
                        p->reg_mye = value;
                        p->sprite_y_exp_flip_flop |= ~value;
			break;

		/// Speicher Pointer
		case 0x18:
                        p->reg_vbase = value;
                        p->matrix_base = (uint16_t)((value & 0xf0) << 6);
                        p->char_base = (uint16_t)((value & 0x0e) << 10);
                        p->bitmap_base = (uint16_t)((value & 0x08) << 10);
			break;

		/// IRQ Flags
		case 0x19: 
                        p->irq_flag = p->irq_flag & (~value & 0x0F);
                        if (p->irq_flag & p->irq_mask) p->irq_flag |= 0x80;
                        else p->CpuClearInterrupt.fn(p->CpuClearInterrupt.p, VIC_IRQ);
			break;
		
		/// IRQ Mask
		case 0x1A:
            p->irq_mask = value & 0x0F;
            if (p->irq_flag & p->irq_mask)
			{
                p->irq_flag |= 0x80;
                p->CpuTriggerInterrupt.fn(p->CpuTriggerInterrupt.p, VIC_IRQ);
			} else {
                p->irq_flag &= 0x7F;
                p->CpuClearInterrupt.fn(p->CpuClearInterrupt.p, VIC_IRQ);
			}
			break;

		/// Sprite Daten Priorität
		case 0x1B:
                        p->reg_mdp = value;
			break;

		/// Sprite Multicolor
		case 0x1C:
                        p->reg_mmc = value;
			break;

		/// Sprite X-Expansion
		case 0x1D:
                        p->reg_mxe = value;
			break;

		/// Rahmenfarbe
		case 0x20:
                        p->reg_ec = value&15;
                        if(p->vic_config[VIC_GREY_DOTS_ON]) p->is_write_reg_ec = true;
			break;
		/// Hintergrundfarbe 0
        case 0x21:
                        p->reg_b0c = value&15;
                        if(p->vic_config[VIC_GREY_DOTS_ON]) p->is_write_reg_b0c = true;
			break;
		/// Hintergrundfarbe 1
        case 0x22: p->reg_b1c	= value&15;
                        //isWriteColorReg = true;
			break;
		/// Hintergrundfarbe 2
        case 0x23: p->reg_b2c	= value&15;
                        //isWriteColorReg = true;
			break;
		/// Hintergrundfarbe 3
        case 0x24: p->reg_b3c	= value&15;
                        //isWriteColorReg = true;
			break;
		/// Sprite Multicolor 0
        case 0x25: p->reg_mm0  = value&15;
                        //isWriteColorReg = true;
			break;
		/// Sprite Multicolor 1
        case 0x26: p->reg_mm1  = value&15;
                        //isWriteColorReg = true;
			break;

		/// Sprite Farbe
		case 0x27: case 0x28: case 0x29: case 0x2A:
		case 0x2B: case 0x2C: case 0x2D: case 0x2E:
                        p->reg_mcolor[address - 0x27] = value&15;
			break;
	}
}

static uint8_t VICII_ReadIO(VICII* p, uint16_t address)
{
    uint8_t ret;
    address &= 63;

    switch (address)
	{
		/// Sprite X Position
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0A: case 0x0C: case 0x0E:
            return (uint8_t)(p->reg_mx[address >> 1]);

		/// Sprite Y Position
		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0B: case 0x0D: case 0x0F:
            return p->reg_my[address >> 1];

		/// Sprite X Position MSB
        case 0x10:
            return p->reg_mx8;

		/// Control Register 1
		case 0x11:	
            if(p->write_reg_0x11) return (p->reg_ctrl_1 & 0x7F) | ((p->current_rasterline & 0x100) >> 1);
            else return 0;

		/// Rasterzähler
		case 0x12:
            return (uint8_t)(p->current_rasterline);

		/// Ligthpen X
        case 0x13:
            return p->reg_lpx;

		/// Ligthpen Y
		case 0x14:
            return p->reg_lpy;

		case 0x15:	// Sprite enable
            return p->reg_me;

		// Control Register 2
		case 0x16:
            return p->reg_ctrl_2 | 0xC0;

		/// Sprite Y-Expansion
		case 0x17:
            return p->reg_mye;

		/// Speicher Pointer
		case 0x18:
            return p->reg_vbase | 0x01;

		/// IRQ Flags
		case 0x19:
            return p->irq_flag | 0x70;

		/// IRQ Mask
		case 0x1A:
            return p->irq_mask | 0xF0;

		/// Sprite Daten Priorität
		case 0x1B:
            return p->reg_mdp;

		/// Sprite Multicolor
		case 0x1C:
            return p->reg_mmc;

		/// Sprite X-Expansion
		case 0x1D:
            return p->reg_mxe;

		/// Sprite - Sprite Kollision
		case 0x1E:
            ret = p->reg_mm;
            p->reg_mm = 0;
            return ret;

		/// Sprite - Daten Kollision
		case 0x1F:
            ret = p->reg_md;
            p->reg_md = 0;
            return ret;

        case 0x20: return p->reg_ec | 0xf0;
        case 0x21: return p->reg_b0c | 0xf0;
        case 0x22: return p->reg_b1c | 0xf0;
        case 0x23: return p->reg_b2c | 0xf0;
        case 0x24: return p->reg_b3c | 0xf0;
        case 0x25: return p->reg_mm0 | 0xf0;
        case 0x26: return p->reg_mm1 | 0xf0;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
            return p->reg_mcolor[address - 0x27] | 0xf0;

		default:
			return 0xFF;
	}
}
