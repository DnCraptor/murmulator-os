//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6569_class.cpp                     //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 19.09.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "mos6569_class.h"

//PAL

#define ReadCPU(adresse) ReadProcTbl[(adresse)>>8](adresse)

///////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////

VICII::VICII()
{
    current_video_buffer = 0;
    video_buffer = video_buffer_back[0];

    for(int i=0; i<8; i++)
	{
        reg_mx[i] = 0;
        reg_my[i] = 0;
        reg_mcolor[i] = 0;
	}

    reg_mx8 = 0;
    reg_ctrl_1 = 0;
    reg_ctrl_2 = 0;
    reg_y_scroll = 0;
    reg_x_scroll = 0;
    reg_vbase = 0;
    reg_irq_raster = 0;
    reg_lpx = 0;
    reg_lpy = 0;
    reg_me = 0;
    reg_mye = 0;
    reg_mdp = 0;
    reg_mmc = 0;
    reg_mxe = 0;
    reg_mm = 0;
    reg_md = 0;
    reg_ec = 0;
    reg_b0c = 0;
    reg_b1c = 0;
    reg_b2c = 0;
    reg_b3c = 0;
    reg_mm0 = 0;
    reg_mm1 = 0;

    current_cycle = 1;
    current_rasterline = 0;
    graphic_mode = 0;
    irq_flag = 0;
    irq_mask = 0;

    // XKoordinaten in Tabelle schreiben
    uint16_t x_pos = 0x0194;
    for(int i=1; i<14; i++)
    {
        x_coordinates_table[i] = x_pos;
        x_pos += 8;
    }

    x_pos = 0x0004;
    for(int i=14; i<64; i++)
    {
        x_coordinates_table[i] = x_pos;
        x_pos += 8;
    }

	SetVicType(0);

    draw_line_counter = 0;
    lp_triggered = draw_this_line = vertical_blanking = false;

    is_write_reg_ec = false;
    is_write_reg_b0c = false;

    badline_enable = false;
    badline_status = false;
    display_status = false;
    matrix_base = 0;
    char_base = 0;
    bitmap_base = 0;

    rc = 0;
    vc = 0;
    vc_base = 0;
    vmli = 0;

    c_address = 0;
    g_address = 0;

    for(int i=0; i<40; i++)
	{
            c_data_buffer_8bit[i] = 0;
            c_data_buffer_4bit[i] = 0;
	}

    for(int i=0; i<8; i++)
	{
        mc[i] = 0;
        mc_base[i] = 0;
	}

    sprite_y_exp_flip_flop = 0xFF;
    sprite_dma = 0;
    sprite_view = 0;

    uint16_t spr_x_start = 0x194;
    for(uint16_t x=0; x<0x200; x++)
	{
        sprite_x_display_table[spr_x_start]=x;
        if(x<107) sprite_x_display_table[spr_x_start] += 8;
		spr_x_start++;
		spr_x_start &= 0x1FF;
	}

    for(int i=0; i<VIC_CONFIG_NUM; i++)
        vic_config[i] = true;

    write_reg_0x11 = false;

    first_display_line_pal = 16;                       //VICE - 16, Emu64 - 26
    last_display_line_pal = 288;                       //VICE - 288, Emu64 - 292

    first_display_line_ntsc = 30;
    last_display_line_ntsc = 288;

    first_display_line = first_display_line_pal;
    last_display_line = last_display_line_pal;

    h_border_compare_left[0]=BORDER_CSEL0_LEFT;
    h_border_compare_right[0]=BORDER_CSEL0_RIGHT;
    v_border_compare_top[0]=BORDER_RSEL0_YSTART;
    v_border_compare_bottom[0]=BORDER_RSEL0_YSTOP;

    h_border_compare_left[1]=BORDER_CSEL1_LEFT;
    h_border_compare_right[1]=BORDER_CSEL1_RIGHT;
    v_border_compare_top[1]=BORDER_RSEL1_YSTART;
    v_border_compare_bottom[1]=BORDER_RSEL1_YSTOP;
}

VICII::~VICII()
{
}

void VICII::SwitchVideoBuffer()
{
    current_video_buffer++;
    video_buffer = video_buffer_back[current_video_buffer & 1];
}

void VICII::GetRegister(VIC_STRUCT *vic_reg)
{
    vic_reg->current_cycle = current_cycle;

    if(current_cycle == 1)
        vic_reg->current_rasterline = current_rasterline;
    else
        vic_reg->current_rasterline = current_rasterline;

    vic_reg->raster_latch = reg_irq_raster;
    vic_reg->display_status = display_status;
    vic_reg->graphic_mode = graphic_mode;
    vic_reg->vic_bank = *cia2_port_a & 3;
    vic_reg->matrix_base = matrix_base;
    vic_reg->char_base = char_base;
    vic_reg->irq = irq_flag & 0x80;

	for(int i=0;i<8;i++)
	{
        vic_reg->sprite_x[i] = reg_mx[i];
        vic_reg->sprite_y[i] = reg_my[i];
    }
}

void VICII::SetVicVDisplayPalSize(uint16_t first_line, uint16_t last_line)
{
    if(first_line < MAX_RASTERLINES && last_line < MAX_RASTERLINES && first_line <= last_line)
    {
        first_display_line_pal = first_line;
        last_display_line_pal = last_line;
        SetVicType(system);
    }
}

uint16_t VICII::GetVicFirstDisplayLinePal()
{
    return first_display_line_pal;
}

uint16_t VICII::GetVicLastDisplayLinePal()
{
    return last_display_line_pal;
}

uint16_t VICII::GetAktVicDisplayFirstLine()
{
    return first_display_line;
}

uint16_t VICII::GetAktVicDisplayLastLine()
{
    return last_display_line;
}

void VICII::SetVicVDisplayNtscSize(uint16_t first_line, uint16_t last_line)
{
    if(first_line < MAX_RASTERLINES && last_line < MAX_RASTERLINES && first_line <= last_line)
    {
        first_display_line_ntsc = first_line;
        last_display_line_ntsc = last_line;
        SetVicType(system);
    }
}

uint16_t VICII::GetVicFirstDisplayLineNtsc()
{
    return first_display_line_ntsc;
}

uint16_t VICII::GetVicLastDisplayLineNtsc()
{
    return last_display_line_ntsc;
}

void VICII::SetVicType(uint8_t system)
{
    this->system = system;

    if(system == 0)
	{
        total_rasterlines=312;
        total_cycles_per_rasterline=63;
        total_x_coordinate=504;
        raster_y = total_rasterlines - 1;
        first_display_line = first_display_line_pal;
        last_display_line = last_display_line_pal;
    }
    else if(system == 1)
	{
        total_rasterlines=262;
        total_cycles_per_rasterline=64;
        total_x_coordinate=512;
        raster_y = total_rasterlines - 1;
        first_display_line = first_display_line_ntsc;
        last_display_line = last_display_line_ntsc;
    }
    else if(system == 2)
	{
        total_rasterlines=263;
        total_cycles_per_rasterline=65;
        total_x_coordinate=520;
        raster_y = total_rasterlines - 1;
        first_display_line = first_display_line_ntsc;
        last_display_line = last_display_line_ntsc;
	}
}

inline void VICII::RasterIRQ()
{
    irq_flag |= 0x01;
    if (irq_mask & 0x01)
	{
        irq_flag |= 0x80;
		CpuTriggerInterrupt(VIC_IRQ);
	}
}

inline void VICII::SetBALow()
{
    if (*ba)
    {
        first_ba_cycle = current_cycle;
        *ba = false;
    }
}

inline void VICII::cAccess()
{
    if (*ba == false)
	{
		/// Grafikdaten ///
        if(current_cycle >= 15 && current_cycle <= 54 && display_status == true && badline_status == true)
        {
            c_address = static_cast<uint16_t>((*cia2_port_a<<14)|matrix_base|(vc & 0x3FF));
            c_data_buffer_8bit[vmli] = ReadCPU(c_address);
            c_data_buffer_4bit[vmli] = color_ram[(vc & 0x3FF)] & 0x0F;
        }
	}
}

inline void VICII::gAccess()
{
    if (display_status) // DisplayModus
	{
        if (reg_ctrl_1 & 0x20) g_address = static_cast<uint16_t>((*cia2_port_a<<14)|(bitmap_base)|((vc & 0x3FF)<<3)|rc);
        else g_address = static_cast<uint16_t>((*cia2_port_a<<14)|char_base|(c_data_buffer_8bit[vmli]<<3)|rc);
        if (reg_ctrl_1 & 0x40) g_address &= 0xf9ff;

        if(graphic_mode==4)
            gfx_data = ReadCPU(g_address & 0xF9FF);
        else
            gfx_data = ReadCPU(g_address);

        char_data = c_data_buffer_8bit[vmli];
        color_data = c_data_buffer_4bit[vmli];

        vmli++;
        vc++;
	} 
    else    // IdleModus
	{
        g_address = reg_ctrl_1 & 0x40 ? 0x39FF : 0x3FFF;
        g_address |=(*cia2_port_a<<14);
        gfx_data = ReadCPU(g_address);
        char_data = 0;
        color_data = 0;
	}

    last_read_gp_access = gfx_data;
}

inline void VICII::pAccess(uint8_t sp_nr)
{
    p_address = static_cast<uint16_t>((*cia2_port_a<<14)|(matrix_base)|0x3F8|sp_nr);
    p_data[sp_nr] = ReadCPU(p_address);

    last_read_gp_access = p_data[sp_nr];
}

inline void VICII::sAccess(uint8_t sp_nr)
{
	//// Es werden gleich alle 3 Bytes geladen ////
    s_address = static_cast<uint16_t>((*cia2_port_a<<14)|(p_data[sp_nr]<<6)|(mc[sp_nr] & 0x3F));
    sprite_sequenzer[sp_nr] = static_cast<uint32_t>((ReadCPU(s_address)) << 16); s_address++;
    sprite_sequenzer[sp_nr] |= static_cast<uint32_t>(((ReadCPU(s_address)) << 8)); s_address++;
    sprite_sequenzer[sp_nr] |= ReadCPU(s_address);
}

inline void VICII::CheckBorder()
{
	///////////////// Rahmen ///////////////
    if(border_flip_flop0)
	{
        if(csel==1 && current_cycle == BORDER_CSEL1_RIGHT && border_flip_flop1 == false)
        {
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
        }
        else if(csel==0 && current_cycle == BORDER_CSEL0_RIGHT && border_flip_flop1 == false)
        {
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = reg_ec;
        }
        else
        {
            if(is_write_reg_ec)
                border_line[border_line_pos++] = 0x0f;
            else
                border_line[border_line_pos++] = reg_ec;

            int i=7;
            do{
                border_line[border_line_pos++] = reg_ec;
                i--;
            } while(i>0);

        }
	}
	else
	{
        if(csel==0 && current_cycle == BORDER_CSEL0_LEFT && border_flip_flop1 == true)
        {
            if(is_write_reg_ec)
                border_line[border_line_pos++] = 0x0f;
            else
                border_line[border_line_pos++] = reg_ec;

            border_line[border_line_pos++] = reg_ec;
            border_line[border_line_pos++] = reg_ec;
            border_line[border_line_pos++] = reg_ec;
            border_line[border_line_pos++] = reg_ec;
            border_line[border_line_pos++] = reg_ec;
            border_line[border_line_pos++] = 128;
            border_line[border_line_pos++] = 128;
        }
        else
        {
            int i=8;
            do{
                border_line[border_line_pos++] = 128;
                i--;
            } while(i>0);
        }
	}
}

inline void VICII::DrawGraphics()
{
    static uint16_t next_x_scroll;

    if (!draw_this_line) return;

    video_buffer_line_xscroll = video_buffer_line + 16;

    if(current_cycle == 14)
	{
        for(int i=0;i<8;i++) video_buffer_line_xscroll[i] = reg_b0c | 0x40;
        next_x_scroll = reg_x_scroll;
	}

    video_buffer_line_xscroll += next_x_scroll;

    if(display_status)	// Display Mode
    {
        if((current_cycle >= 16) && (current_cycle <= 55)) // Grafiksequenzer --> muss um 16 Pixel verzögert werden
        {
            if(graphic_mode == 0) {
				/// Standard Text Modus (ECM/BMM/MCM = 0/0/0) ///
                colors[0] = reg_b0c | 0x40;
                colors[1] = color_data | 0x80;
					
                for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                video_buffer_line += 8;
            }
			else if(graphic_mode == 1) {
				/// Multicolor Text Modus (ECM/BMM/MCM = 0/0/1) ///
                if(!(color_data&8))
				{
					/// MC-Flag = 0 ///
                    colors[0] = reg_b0c | 0x40;
                    colors[1] = (color_data&7) | 0x80;

                    for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                    video_buffer_line += 8;
				}
				else
				{
					/// MC-Flag = 1 ///
                    colors[0] = reg_b0c | 0x40;
                    colors[1] = reg_b1c;
                    colors[2] = reg_b2c | 0x80;
                    colors[3] = (color_data&7) | 0x80;

					for(int i=6;i>-1;i-=2)
					{
                        *video_buffer_line_xscroll = colors[(gfx_data>>(i))&3];
                        video_buffer_line_xscroll++;
                        *video_buffer_line_xscroll = video_buffer_line_xscroll[-1];
                        video_buffer_line_xscroll++;
					}
                    video_buffer_line += 8;
				}
            }
			else if(graphic_mode == 2) {
				/// Standard Bitmap Modus (ECM/BMM/MCM = 0/1/0) ///
                colors[0] = char_data&0x0F;
                colors[1] = (char_data>>4) | 0x80;
				
                for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                video_buffer_line += 8;
            }
			else if(graphic_mode == 3) {
				/// Multicolor Bitmap Modus (ECM/BMM/MCM = 0/1/1) ///
                colors[0] = reg_b0c | 0x40;
                colors[1] = char_data>>4;
                colors[2] = (char_data&0x0F) | 0x80;
                colors[3] = color_data | 0x80;
				
				for(int i=6;i>-1;i-=2)
				{
                    *video_buffer_line_xscroll = colors[(gfx_data>>(i))&3];
                    video_buffer_line_xscroll++;
                    *video_buffer_line_xscroll = video_buffer_line_xscroll[-1];
                    video_buffer_line_xscroll++;
				}
                video_buffer_line += 8;
            }
			else if(graphic_mode == 4) {
				/// ECM Text Modus (ECM/BMM/MCM = 1/0/0) ///
                uint8_t t = (char_data >> 6) & 0x03;
                if ( t == 0 )
                    colors[0] = reg_b0c | 0x40;
                else if ( t == 1 )
                    colors[0] = reg_b1c;
                else if ( t == 2 )
                    colors[0] = reg_b2c;
                else if ( t == 3 )
                    colors[0] = reg_b3c;
                colors[1] = color_data | 0x80;
					
                for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                video_buffer_line += 8;
			}
            else {
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                video_buffer_line += 8;
			}
		}
        else
        {
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                video_buffer_line += 8;
        }
	}
	else	// Idle Mode
    {
        colors[0] = 0 | 0x40;

        if((current_cycle >= 16) && (current_cycle <= 55)) // Grafiksequenzer --> muss um 16 Pixel verzögert werden
		{
            // Grafiksequenzer --> muss um 16 Pixel verzögert werden
            if ( graphic_mode == 0 ) {
                /// Standard Text Modus (ECM/BMM/MCM = 0/0/0) ///
                colors[1] = 0 | 0x80;
                for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                video_buffer_line += 8;
            }
            else if ( graphic_mode == 1 ) {
				/// Multicolor Text Modus (ECM/BMM/MCM = 0/0/1) ///
                if(!(color_data&8))
				{
                    /// MC-Flag = 0 ///
                    colors[1] = 0 | 0x80;

                    for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                    video_buffer_line += 8;
				}
				else
				{
                    /// MC-Flag = 1 ///
                    colors[1] = 0;
                    colors[2] = 0 | 0x80;
                    colors[3] = 0 | 0x80;

					for(int i=6;i>-1;i-=2)
					{
                        *video_buffer_line_xscroll = colors[(gfx_data>>(i))&3];
                        video_buffer_line_xscroll++;
                        *video_buffer_line_xscroll = video_buffer_line_xscroll[-1];
                        video_buffer_line_xscroll++;
					}
                    video_buffer_line += 8;
				}
            }
            else if ( graphic_mode == 2 ) {
				/// Standard Bitmap Modus (ECM/BMM/MCM = 0/1/0) ///
                colors[1] = 0 | 0x80;
				
                for(int i=7;i>-1;i--) *video_buffer_line_xscroll++ = colors[(gfx_data>>(i))&1];
                video_buffer_line += 8;
            }
            else if ( graphic_mode == 3 ) {
                /// Multicolor Bitmap Modus (ECM/BMM/MCM = 1/0/0) ///
                colors[1] = 0;
                colors[2] = 0 | 0x80;
                colors[3] = 0 | 0x80;
				
				for(int i=6;i>-1;i-=2)
				{
                    *video_buffer_line_xscroll = colors[(gfx_data>>(i))&3];
                    video_buffer_line_xscroll++;
                    *video_buffer_line_xscroll = video_buffer_line_xscroll[-1];
                    video_buffer_line_xscroll++;
				}
                video_buffer_line += 8;
            }
            else {
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                *video_buffer_line_xscroll++ = 0;
                video_buffer_line += 8;
			}
		}
        else
        {
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                *video_buffer_line_xscroll++ = colors[0];
                video_buffer_line += 8;
        }
	}

    /// Im Aktuellen Zyklus Prüfen ob B0C Color verwendet wurde (16 Pixel vorlauf)
    /// Wenn ja mit aktuellen B0C überschreiben

    video_buffer_line_xscroll = video_buffer_line - 8;

    /// Grey Dot zeichnen
    ///
    if(is_write_reg_b0c && (video_buffer_line_xscroll[0] & 0x40) && display_status)
        video_buffer_line_xscroll[0] = 0x0f;


    for(int i=0;i<8;i++)
    {
        if(video_buffer_line_xscroll[i] & 0x40) video_buffer_line_xscroll[i] =  reg_b0c;
    }

    ////////////////////////// Draw Border //////////////////////////
    CheckBorder();
}

inline void VICII::DrawGraphicsPseudo()
{
    /// Für Anzeige Aller Zyklen
    video_buffer_line += 8;

    //CheckBorder
    border_line_pos += 8;
    return;
}

inline void VICII::DrawSprites()
{
    uint8_t current_sprite_bit = 0x80;          // Sprites werden hier von 7 nach 0 gezeichnet (Sprite 0 hat die höchste Priorität und 7 die niedrigste)
    uint8_t current_sprite_collision = 0;       // Noch keine Kollision Sprite-Sprite
    uint8_t current_gfx_collision = 0;          // Noch keine Kollision Sprite-Hintergrund

    /*
    if(!vic_config[VIC_SPRITES_ON]) return;

    uint8_t mask=1;

    for(int SpriteNr=0; SpriteNr<8; SpriteNr++)
    {
        if(SpriteView & mask)
        {
            if((reg_mx[SpriteNr] >= AktXKoordinate) && (reg_mx[SpriteNr] < AktXKoordinate+8))
            {
                video_buffer_line_xscroll = (video_buffer_line-8) + (reg_mx[SpriteNr]-AktXKoordinate);
                *video_buffer_line_xscroll++ = SpriteNr+1;
            }
        }
        mask<<=1;
    }
    return;
    */

    /////////////////////////////////////////
    /////////////////////////////////////////

    if(!vic_config[VIC_SPRITES_ON]) return;
    if(current_rasterline < first_display_line+1) return;

    // Spritekollisionspuffer löschen
    for (int i=0; i < 520; i++) sprite_collision_buffer[i] = 0;

    int sprite_nr = 7;
    while(sprite_nr >= 0)
    {
        // Ist Sprite Sichtbar ?
        if(((sprite_view_current_line & current_sprite_bit) == current_sprite_bit) && (reg_mx[sprite_nr]<0x1F8))
		{
            sprite_buffer_line = (video_buffer_line - (62 * 8)) -4 + sprite_x_display_table[reg_mx[sprite_nr]];

            /// Temporärer Zeiger inerhalb des SpriteCollisionsPuffers auf das Aktuellen Sprites
            unsigned char *q = sprite_collision_buffer + reg_mx[sprite_nr] + 8;

            if((reg_mdp & current_sprite_bit) == 0)	/// Prüfen auf Sprite Hinter Vordergrund
			{
				/// Sprites vor der Vordergrundgrafik
                if((reg_mxe & current_sprite_bit) == current_sprite_bit)
				{
                    /// Expandiertes Sprite
                    if((reg_mmc & current_sprite_bit) == current_sprite_bit)
					{
                        /// MultiColor Sprite
                        for(int i=0;i<12;i++)
						{
                            uint32_t t = sprite_sequencer_current_line[sprite_nr] & 0xC00000;
                            if ( t == 0x000000 ) {
                                sprite_buffer_line += 4;
                            }
                            else if ( t == 0x400000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+2) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+3) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mm0;
                                *sprite_buffer_line++ = reg_mm0;
                                *sprite_buffer_line++ = reg_mm0;
                                *sprite_buffer_line++ = reg_mm0;

                                /// Sprite-Sprite Kollision (MuliColor Expand)
                                if (q[i<<2]) current_sprite_collision |= q[i<<2] | current_sprite_bit;
                                else q[i<<2] = current_sprite_bit;
                                if (q[(i<<2) + 1]) current_sprite_collision |= q[(i<<2) + 1] | current_sprite_bit;
                                else q[(i<<2) + 1] = current_sprite_bit;
                                if (q[(i<<2) + 2]) current_sprite_collision |= q[(i<<2) + 2] | current_sprite_bit;
                                else q[(i<<2) + 2] = current_sprite_bit;
                                if (q[(i<<2) + 3]) current_sprite_collision |= q[(i<<2) + 3] | current_sprite_bit;
                                else q[(i<<2) + 3] = current_sprite_bit;
                            }
                            else if ( t == 0x800000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+2) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+3) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];

                                /// Sprite-Sprite Kollision (MuliColor Expand)
                                if (q[i<<2]) current_sprite_collision |= q[i<<2] | current_sprite_bit;
                                else q[i<<2] = current_sprite_bit;
                                if (q[(i<<2) + 1]) current_sprite_collision |= q[(i<<2) + 1] | current_sprite_bit;
                                else q[(i<<2) + 1] = current_sprite_bit;
                                if (q[(i<<2) + 2]) current_sprite_collision |= q[(i<<2) + 2] | current_sprite_bit;
                                else q[(i<<2) + 2] = current_sprite_bit;
                                if (q[(i<<2) + 3]) current_sprite_collision |= q[(i<<2) + 3] | current_sprite_bit;
                                else q[(i<<2) + 3] = current_sprite_bit;
                            }
                            else if ( t == 0xC00000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+2) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+3) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mm1;
                                *sprite_buffer_line++ = reg_mm1;
                                *sprite_buffer_line++ = reg_mm1;
                                *sprite_buffer_line++ = reg_mm1;

                                /// Sprite-Sprite Kollision (MuliColor Expand)
                                if (q[i<<2]) current_sprite_collision |= q[i<<2] | current_sprite_bit;
                                else q[i<<2] = current_sprite_bit;
                                if (q[(i<<2) + 1]) current_sprite_collision |= q[(i<<2) + 1] | current_sprite_bit;
                                else q[(i<<2) + 1] = current_sprite_bit;
                                if (q[(i<<2) + 2]) current_sprite_collision |= q[(i<<2) + 2] | current_sprite_bit;
                                else q[(i<<2) + 2] = current_sprite_bit;
                                if (q[(i<<2) + 3]) current_sprite_collision |= q[(i<<2) + 3] | current_sprite_bit;
                                else q[(i<<2) + 3] = current_sprite_bit;
							}
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 2;
						}
					}
					else
					{
                        /// SingleColor Sprite
                        for(int i=0;i<24;i++)
						{
                            if(sprite_sequencer_current_line[sprite_nr] & 0x800000)
							{
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];

                                /// Sprite-Sprite Kollision (SingleColor Expand)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;
                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
							}
                            else sprite_buffer_line += 2;
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 1;
						}
					}
				}
				else
				{
                    /// Nicht Expandiertes Sprite
                    if((reg_mmc & current_sprite_bit) == current_sprite_bit)
					{
                        /// MultiColor Sprite
                        for(int i=0;i<12;i++)
						{
                            uint32_t t = sprite_sequencer_current_line[sprite_nr] & 0xC00000;
                            if ( t == 0x000000 ) {
                                sprite_buffer_line += 2;
                            }
                            else if ( t == 0x400000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mm0;
                                *sprite_buffer_line++ = reg_mm0;

                                /// Sprite-Sprite Kollision (MuliColor)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;

                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
                            }
                            else if ( t == 0x800000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];

                                /// Sprite-Sprite Kollision (MuliColor)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;

                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
                            }
                            else if ( t == 0xC00000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mm1;
                                *sprite_buffer_line++ = reg_mm1;

                                /// Sprite-Sprite Kollision (MuliColor)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;

                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
							}
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 2;
						}
					}
					else
					{
                        /// SingleColor Sprite
                        for(int i=0;i<24;i++)
						{                            
                            if(sprite_sequencer_current_line[sprite_nr] & 0x800000)
                            {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;

                                *sprite_buffer_line++ = reg_mcolor[sprite_nr];

                                /// Sprite-Sprite Kollision
                                if (q[i]) current_sprite_collision |= q[i] | current_sprite_bit;
                                else q[i] = current_sprite_bit;
                            }
                            else sprite_buffer_line++;
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 1;
						}
					}
				}
			}
			else
			{
				/// Sprites hinter der Vordergrundgrafik
                if((reg_mxe & current_sprite_bit) == current_sprite_bit)
				{
                    /// Expandiertes Sprite
                    if((reg_mmc & current_sprite_bit) == current_sprite_bit)
					{
                        /// MultiColor Sprite
                        for(int i=0;i<12;i++)
						{
                            uint32_t t = sprite_sequencer_current_line[sprite_nr] & 0xC00000;
                            if ( t == 0x000000 ) {
                                sprite_buffer_line += 4;
                            }
                            else if ( t == 0x400000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+2) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+3) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm0;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm0;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm0;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm0;
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (MuliColor Expand)
                                if (q[i<<2]) current_sprite_collision |= q[i<<2] | current_sprite_bit;
                                else q[i<<2] = current_sprite_bit;
                                if (q[(i<<2) + 1]) current_sprite_collision |= q[(i<<2) + 1] | current_sprite_bit;
                                else q[(i<<2) + 1] = current_sprite_bit;
                                if (q[(i<<2) + 2]) current_sprite_collision |= q[(i<<2) + 2] | current_sprite_bit;
                                else q[(i<<2) + 2] = current_sprite_bit;
                                if (q[(i<<2) + 3]) current_sprite_collision |= q[(i<<2) + 3] | current_sprite_bit;
                                else q[(i<<2) + 3] = current_sprite_bit;
                            }
                            else if ( t == 0x800000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+2) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+3) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (MuliColor Expand)
                                if (q[i<<2]) current_sprite_collision |= q[i<<2] | current_sprite_bit;
                                else q[i<<2] = current_sprite_bit;
                                if (q[(i<<2) + 1]) current_sprite_collision |= q[(i<<2) + 1] | current_sprite_bit;
                                else q[(i<<2) + 1] = current_sprite_bit;
                                if (q[(i<<2) + 2]) current_sprite_collision |= q[(i<<2) + 2] | current_sprite_bit;
                                else q[(i<<2) + 2] = current_sprite_bit;
                                if (q[(i<<2) + 3]) current_sprite_collision |= q[(i<<2) + 3] | current_sprite_bit;
                                else q[(i<<2) + 3] = current_sprite_bit;
                            }
                            else if ( t == 0xC00000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+2) & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+3) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm1;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm1;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm1;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm1;
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (MuliColor Expand)
                                if (q[i<<2]) current_sprite_collision |= q[i<<2] | current_sprite_bit;
                                else q[i<<2] = current_sprite_bit;
                                if (q[(i<<2) + 1]) current_sprite_collision |= q[(i<<2) + 1] | current_sprite_bit;
                                else q[(i<<2) + 1] = current_sprite_bit;
                                if (q[(i<<2) + 2]) current_sprite_collision |= q[(i<<2) + 2] | current_sprite_bit;
                                else q[(i<<2) + 2] = current_sprite_bit;
                                if (q[(i<<2) + 3]) current_sprite_collision |= q[(i<<2) + 3] | current_sprite_bit;
                                else q[(i<<2) + 3] = current_sprite_bit;
							}
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 2;
						}
					}
					else
					{
                        /// SingleColor Sprite
                        for(int i=0;i<24;i++)
						{
                            if(sprite_sequencer_current_line[sprite_nr] & 0x800000)
							{
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (SingleColor Expand)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;
                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
							}
                            else sprite_buffer_line += 2;
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 1;
						}
					}
				}
				else
				{
                    /// Nicht Expandiertes Sprite
                    if((reg_mmc & current_sprite_bit) == current_sprite_bit)
					{
                        /// MultiColor Sprite
                        for(int i=0;i<12;i++)
						{
                            uint32_t t = sprite_sequencer_current_line[sprite_nr] & 0xC00000;
                            if ( t == 0x000000 )
                                sprite_buffer_line += 2;
                            else if ( t == 0x400000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm0;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm0;
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (MuliColor)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;

                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
                            }
                            else if ( t == 0x800000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mcolor[sprite_nr];
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (MuliColor)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;

                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;
                            }
                            else if ( t == 0xC00000 ) {
                                /// Sprite-Grafik Kollision
                                if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;
                                if(*(sprite_buffer_line+1) & 0x80) current_gfx_collision |= current_sprite_bit;

                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm1;
                                else sprite_buffer_line++;
                                if(!(*sprite_buffer_line&0x80))*sprite_buffer_line++ = reg_mm1;
                                else sprite_buffer_line++;

                                /// Sprite-Sprite Kollision (MuliColor)
                                if (q[i<<1]) current_sprite_collision |= q[i<<1] | current_sprite_bit;
                                else q[i<<1] = current_sprite_bit;

                                if (q[(i<<1) + 1]) current_sprite_collision |= q[(i<<1) + 1] | current_sprite_bit;
                                else q[(i<<1) + 1] = current_sprite_bit;

							}
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 2;
						}
					}
					else
					{
                        /// SingleColor Sprite
                        for(int i=0;i<24;i++)
						{
                            if(sprite_sequencer_current_line[sprite_nr] & 0x800000)
							{
                                if(!(*sprite_buffer_line&0x80))
                                {                                    
                                    /// Sprite-Grafik Kollision
                                    if(*sprite_buffer_line & 0x80) current_gfx_collision |= current_sprite_bit;

                                    *sprite_buffer_line++ = reg_mcolor[sprite_nr];

                                    /// Sprite-Sprite Kollision
                                    if (q[i]) current_sprite_collision |= q[i] | current_sprite_bit;
                                    else q[i] = current_sprite_bit;
                                }
                                else sprite_buffer_line++;
							}
                            else sprite_buffer_line++;
                            sprite_sequencer_current_line[sprite_nr] = sprite_sequencer_current_line[sprite_nr] << 1;
						}
					}
				}
			}
		}

        sprite_nr--;
        current_sprite_bit >>= 1;
    }

    //////////////////////// Sprite Collision ///////////////////////////

    // Prüfe sprite-sprite kollisions
    if(vic_config[VIC_SPR_SPR_COLL_ON])
    {
        if(reg_mm == 0)
        {
            // IRQ kann ausgelöst werden
            reg_mm |= current_sprite_collision;
            if(reg_mm != 0)
            {
                irq_flag |= 0x04;
                if (irq_mask & 0x04)
                {
                    irq_flag |= 0x80;
                    CpuTriggerInterrupt(VIC_IRQ);
                }
            }
        }
        else
        {
            // kein IRQ auslösen
            reg_mm |= current_sprite_collision;
        }
    }

    // Prüfe sprite-gfx kollision
    if(vic_config[VIC_SPR_BCK_COLL_ON])
    {
        if(reg_md == 0)
        {
            // IRQ kann ausgelöst werden
            reg_md |= current_gfx_collision;
            if(reg_md != 0)
            {
                irq_flag |= 0x02;
                if (irq_mask & 0x02)
                {
                    irq_flag |= 0x80;
                    CpuTriggerInterrupt(VIC_IRQ);
                }
            }
        }
        else
        {
            // kein IRQ auslösen
            reg_md |= current_gfx_collision;
        }
    }
}

inline void VICII::DrawBorder()
{
    if(!vic_config[VIC_BORDER_ON]) return;
    if(current_rasterline < first_display_line) return;
    border_buffer_line = video_buffer_line - 504;	/// Erstes Sichtbares Pixel

    for(int x=0;x<504;x++)
	{
        if(border_line[x] & 128) border_buffer_line++;
        else *border_buffer_line++ = border_line[x];
	}
}

void VICII::OneCycle()
{
    uint8_t sprite_bit = 0x01;

    current_x_coordinate = x_coordinates_table[current_cycle];

    // Prüfen ob Badlines zugelassen sind
    if(current_rasterline == 0x30) badline_enable = den;

    // Prüfen auf Badline zustand
    if((current_rasterline>=0x30) && (current_rasterline<=0xF7) && (reg_y_scroll == (current_rasterline&7)) && (badline_enable == true))
    {
        badline_status = true;
        display_status = true;
    }
    else badline_status = false;

    if ( current_cycle == 1 ) {
        if (current_rasterline == (total_rasterlines-1))
		{
            vertical_blanking = true;
            current_rasterline = 0;
            RefreshProc(video_buffer);
            draw_line_counter = 0;
            vc_base = 0;
		}
		else 
		{

            current_rasterline++;

			// Prüfen auf Raster IRQ
            //if (current_rasterline == reg_irq_raster) RasterIRQ();
            draw_this_line = (current_rasterline >= first_display_line && current_rasterline <= last_display_line);
		}

        if(draw_this_line)
        {
            video_buffer_line = &video_buffer[draw_line_counter++*MAX_XW];		// Zeiger für Aktuelle Zeile setzen
            border_line_pos = 0;
        }

		// Sprite 2 //
        if((sprite_dma & 0x18) == 0) *ba = true;

		// Sprite 3 //
        pAccess(3);
        if(!*ba) sAccess(3);

        DrawGraphicsPseudo();
    } else if ( current_cycle == 2 ) {
        if (vertical_blanking)
        {
            lp_triggered = vertical_blanking = false;
            //if (reg_irq_raster == 0) RasterIRQ();
        }

        // Prüfen auf Raster IRQ
        if (current_rasterline == reg_irq_raster) RasterIRQ();

		/// Sprite 5 ///
        if(sprite_dma & 0x20)  SetBALow();

        DrawGraphicsPseudo();
    } else if ( current_cycle == 3 ) {
		// Sprite 3 //
        if((sprite_dma & 0x30) == 0) *ba = true;

		// Sprite 4 //
        pAccess(4);
        if(!*ba) sAccess(4);

        DrawGraphicsPseudo();
    } else if ( current_cycle == 4 ) {
		/// Sprite 6 ///
        if(sprite_dma & 0x40)  SetBALow();

        DrawGraphicsPseudo();
    } else if ( current_cycle == 5 ) {
        // Sprite 4 //
        if((sprite_dma & 0x60) == 0) *ba = true;

		// Sprite 5 //
        pAccess(5);
        if(!*ba) sAccess(5);

        DrawGraphicsPseudo();
    } else if ( current_cycle == 6 ) {
		/// Sprite 7 ///
        if(sprite_dma & 0x80)  SetBALow();

        DrawGraphicsPseudo();
    } else if ( current_cycle == 7 ) {
        // Sprite 5 //
        if((sprite_dma & 0xC0) == 0) *ba = true;

		// Sprite 6 //
        pAccess(6);
        if(!*ba) sAccess(6);

        DrawGraphicsPseudo();
    } else if ( current_cycle == 8 ) {
        DrawGraphicsPseudo();
    } else if ( current_cycle == 9 ) {
		// Sprite 6 //
        if((sprite_dma & 0x80) == 0) *ba = true;

		// Sprite 7 //
        pAccess(7);
        if(!*ba) sAccess(7);

        vmli = 0;

        DrawGraphicsPseudo();
    } else if ( current_cycle == 10 ) {
        vmli = 0;

        DrawGraphicsPseudo();
    } else if ( current_cycle == 11 ) {
		// Sprite 7 //
        *ba = true;

        vc = vc_base;

        DrawGraphics();
    } else if ( current_cycle == 12 ) {
        vc = vc_base;

        if(badline_status) SetBALow();
        DrawGraphics();
    } else if ( current_cycle == 13 ) {
        vc = vc_base;

        if(badline_status) SetBALow();
        DrawGraphics();
    } else if ( current_cycle == 14 ) {
        if(badline_status) SetBALow();
        if((badline_status == true) && (display_status == true)) rc = 0;

        /// Sprite MC + 3
        for(int i=0;i<8;i++,sprite_bit<<=1)
        {
            if(sprite_dma & sprite_bit) mc[i] += 3;
        }

        DrawGraphics();
    } else if ( current_cycle == 15 ) {
        if (badline_status) SetBALow();
        DrawGraphics();
    } else if ( current_cycle == 16 ) {
        sprite_view_current_line = sprite_view;
        sprite_sequencer_current_line[0] = sprite_sequenzer[0];
        sprite_sequencer_current_line[1] = sprite_sequenzer[1];
        sprite_sequencer_current_line[2] = sprite_sequenzer[2];
        sprite_sequencer_current_line[3] = sprite_sequenzer[3];
        sprite_sequencer_current_line[4] = sprite_sequenzer[4];
        sprite_sequencer_current_line[5] = sprite_sequenzer[5];
        sprite_sequencer_current_line[6] = sprite_sequenzer[6];
        sprite_sequencer_current_line[7] = sprite_sequenzer[7];

        /// Sprite ///
        for(int i=0;i<8;i++,sprite_bit<<=1)
        {
            if((sprite_y_exp_flip_flop & sprite_bit) == sprite_bit) mc_base[i] += 2;
        }

        gAccess();
        if (badline_status) SetBALow();

        DrawGraphics();
    } else if ( current_cycle == 17 ) {
        /// Sprite ///
        for(int i=0;i<8;i++,sprite_bit<<=1)
        {
            if((sprite_y_exp_flip_flop & sprite_bit) == sprite_bit) mc_base[i] ++;
            if(mc_base[i] >= 63)
            {
                sprite_dma  &= ~sprite_bit;
                sprite_view &= ~sprite_bit;
            }
        }

        gAccess();
        if (badline_status) SetBALow();

        DrawGraphics();
    } else if ( current_cycle >= 18 && current_cycle <= 54 ) {
        gAccess();
        if (badline_status) SetBALow();

        DrawGraphics();
    } else if ( current_cycle == 55 ) {
        *ba = true;

		/// Sprite ///
		// In der ersten Phase von Zyklus 55 wird das Expansions-Flipflop
		// invertiert, wenn das MxYE-Bit gesetzt ist.

        for (int i=0; i<8; i++, sprite_bit<<=1) if (reg_mye & sprite_bit) sprite_y_exp_flip_flop ^= sprite_bit;

		// In der ersten Phasen von Zyklus 55 wird für Sprite 0-3 geprüft, 
		// ob das entsprechende MxE-Bit in Register $d015 gesetzt und die Y-Koordinate 
		// des Sprites (ungerade Register $d001-$d00f) gleich den unteren 8 Bits 
		// von RASTER ist. Ist dies der Fall und der DMA für das Sprite noch 
		// ausgeschaltet, wird der DMA angeschaltet, MCBASE gelöscht und, 
		// wenn das MxYE-Bit gesetzt ist, das Expansions-Flipflop gelöscht.
        sprite_bit = 0x01;
        for (int i=0; i<4; i++, sprite_bit<<=1)
		{
            if ((reg_me & sprite_bit) && ((current_rasterline & 0xff) == reg_my[i]))
			{
                if((sprite_dma & sprite_bit) == 0)
				{
                    sprite_dma |= sprite_bit;
                    mc_base[i] = 0;
                    if (reg_mye & sprite_bit) sprite_y_exp_flip_flop &= ~sprite_bit;
				}
			}
		}

		/// Sprite 0 ///
        if(sprite_dma & 0x01)  SetBALow();

        gAccess();
        DrawGraphics();
    } else if ( current_cycle == 56 ) {
		/// Sprite ///
		// In der ersten Phasen von Zyklus 56 wird für Sprite 4-7 geprüft, 
		// ob das entsprechende MxE-Bit in Register $d015 gesetzt und die Y-Koordinate 
		// des Sprites (ungerade Register $d001-$d00f) gleich den unteren 8 Bits 
		// von RASTER ist. Ist dies der Fall und der DMA für das Sprite noch 
		// ausgeschaltet, wird der DMA angeschaltet, MCBASE gelöscht und, 
		// wenn das MxYE-Bit gesetzt ist, das Expansions-Flipflop gelöscht.
        sprite_bit = 0x10;
        for (int i=4; i<8; i++, sprite_bit<<=1)
		{
            if ((reg_me & sprite_bit) && (current_rasterline & 0xff) == reg_my[i])
			{
                if((sprite_dma & sprite_bit) == 0)
				{
                    sprite_dma |= sprite_bit;
                    mc_base[i] = 0;
                    if (reg_mye & sprite_bit) sprite_y_exp_flip_flop &= ~sprite_bit;
				}
			}
		}	

        DrawGraphics();
    } else if ( current_cycle == 57 ) {
		/// Sprite 1 ///
        if(sprite_dma & 0x02)  SetBALow();

        DrawGraphics();
    } else if ( current_cycle == 58 ) {
        if(display_status == false)
        {
            vc_base = vc;
        }
        else if(badline_status)
        {
            if(rc == 7) vc_base = vc;
            rc = (rc + 1) & 7;
        }
        else
        {
            if(rc == 7)
            {
                vc_base = vc;
                display_status = false;
            }
            if(rc < 7) rc = (rc + 1) & 7;
        }

        /// Sprite ///
		// In der ersten Phase von Zyklus 58 wird für jedes Sprite MC mit MCBASE
		// geladen (MCBASE->MC) und geprüft, ob der DMA für das Sprite angeschaltet
		// und die Y-Koordinate des Sprites gleich den unteren 8 Bits von RASTER ist.
		// Ist dies der Fall, wird die Darstellung des Sprites angeschaltet.
        for(int i=0;i<8;i++, sprite_bit<<=1)
		{
            mc[i] = mc_base[i];
            //if(((sprite_dma & bitc) == bitc) && ((current_rasterline&0xFF) == reg_my[i])) SpriteView |= bitc;
            if(((sprite_dma & sprite_bit) == sprite_bit)) sprite_view |= sprite_bit;
		}

		// Sprite 0 //
        pAccess(0);
        if(!*ba) sAccess(0);

        DrawGraphics();
    } else if ( current_cycle == 59 ) {
		/// Sprite 2 ///
        if(sprite_dma & 0x04)  SetBALow();

        DrawGraphics();
    } else if ( current_cycle == 60 ) {
		// Sprite 0 //
        if((sprite_dma & 0x06) == 0) *ba = true;

		// Sprite 1 //
        pAccess(1);
        if(!*ba) sAccess(1);

        DrawGraphics();
    } else if ( current_cycle == 61 ) {
		/// Sprite 3 ///
        if(sprite_dma & 0x08)  SetBALow();

        DrawGraphics();
    } else if ( current_cycle == 62 ) {
		// Sprite 1 //
        if((sprite_dma & 0x0C) == 0) *ba = true;

		// Sprite 2 //
        pAccess(2);
        if(!*ba) sAccess(2);

        DrawGraphicsPseudo();
    } else if ( current_cycle == 63 ) {
		/// Rahmen ///
        if(rsel == 0)
		{
            if(current_rasterline == BORDER_RSEL0_YSTOP) border_flip_flop1 = true;
            if((current_rasterline == BORDER_RSEL0_YSTART) && (den == true)) border_flip_flop1 = false;
		}
		else
		{
            if(current_rasterline == BORDER_RSEL1_YSTOP) border_flip_flop1 = true;
            if((current_rasterline == BORDER_RSEL1_YSTART) && (den == true)) border_flip_flop1 = false;
		}

		/// Sprite 4 ///
        if(sprite_dma & 0x10)  SetBALow();

        DrawGraphicsPseudo();
        DrawSprites();
        DrawBorder();

        if(total_cycles_per_rasterline == 63) current_cycle = 0;

    } else if ( current_cycle == 64 ) {
        if(total_cycles_per_rasterline == 64) current_cycle = 0;

        DrawGraphicsPseudo();
    } else if ( current_cycle == 65 ) {
        current_cycle = 0;

        DrawGraphicsPseudo();
	}

    cAccess();     // cZugriffe immer im 2.Teil eines Zyklus wenn BA Low ist

    // Rahmen
    if(current_cycle == h_border_compare_right[csel])
    {
        border_flip_flop0 = true;
    }

    if(current_cycle == h_border_compare_left[csel])
    {
        if(current_rasterline == v_border_compare_bottom[rsel]) border_flip_flop1 = true;
        if(current_rasterline == v_border_compare_top[rsel]  && (den == true)) border_flip_flop1 = false;
        if(border_flip_flop1 == false) border_flip_flop0 = false;
    }

    // Ist ein Spritesequenzer aktiv ?
    // NEU
    //DrawSprites();

    is_write_reg_ec = false;
    is_write_reg_b0c = false;

    ///   Auf nächsten Zyklus stellen   ///
    /// Aktueller Zyklus ist nun zuende ///

    current_cycle++;
/**
    if(current_cycle == 1)
    {
        if(breakpoints[current_rasterline+1] & 256)
        {
                *break_status |= 256;
                break_values[8] = current_rasterline+1;
        }
    }
    else
    {
        if(breakpoints[current_rasterline] & 256)
        {
                *break_status |= 256;
                break_values[8] = current_rasterline;
        }
    }

    if(breakpoints[current_cycle] & 512)
	{
        *break_status |= 512;
        break_values[9] = current_cycle;
    }
    */
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/*
#define fw(var) fwrite(&var,1,sizeof(var),File)
#define fw1(var,size) fwrite(var,1,size,File)
bool VICII::SaveFreez(FILE* File)
{

	int Offset;
    Offset = video_buffer_line - video_buffer;
	fw(Offset);
	
    fw(draw_line_counter);
    fw(draw_this_line);
	fw(bitc);
    fw1(reg_mx,8);
    fw1(reg_my,8);
    fw(reg_mx8);
    fw(reg_ctrl_1);
    fw(reg_ctrl_2);
    fw(reg_y_scroll);
    fw(reg_x_scroll);
    fw(reg_vbase);
    fw(reg_irq_raster);
    fw(reg_lpx);
    fw(reg_lpy);
    fw(reg_me);
    fw(reg_mye);
    fw(reg_mdp);
    fw(reg_mmc);
    fw(reg_mxe);
	fw(MM);
    fw(reg_md);
	fw(EC);
    fw(reg_b0c);
    fw(reg_b1c);
    fw(reg_b2c);
    fw(reg_b3c);
    fw(reg_mm0);
    fw(reg_mm1);
    fw1(reg_mcolor,8);
    fw(current_cycle);
    fw(current_rasterline);
	fw(AktXKoordinate);
    fw(graphic_mode);
    fw(irq_flag);
    fw(irq_mask);
	fw(DEN);
	fw(VBlanking);
	fw(FirstBAZyklus);
    fw(badline_enable);
    fw(badline_status);
    fw(display_status);
    fw(lp_triggered);
	fw(RC);
    fw(vc);
    fw(vc_base);
    fw(matrix_base);
    fw(char_base);
    fw(bitmap_base);
    fw(vmli);
    fw(c_address);
    fw(g_address);
    fw1(c_value_buffer_8bit,64);
    fw1(c_value_buffer_4bit,64);
    fw(gfx_data);
    fw(char_data);
    fw(color_data);
	fw(GreyDotEnable);
    fw(is_write_reg_ec);
    fw(is_write_reg_b0c);

	//////////////////////////////// Sprites //////////////////////////////////

    fw(sprite_y_exp_flip_flop);
	fw1(MC,8);
    fw1(mc_base,8);
    fw(sprite_dma);
	fw(SpriteView);

	///////////////////////////////////////////////////////////////////////////

	////////////////////////////// Rahmenstufe ////////////////////////////////

    fw(csel);
    fw(rsel);
	fw(BorderCmpYo);
	fw(BorderCmpYu);
    fw(border_flip_flop0);
    fw(border_flip_flop1);
	fw(BorderCMP_Re);
	fw(BorderCMP_Li);
	fw(BorderCMP_Ob);
	fw(BorderCMP_Un);
	return true;
}

#define fr(var) fread(&var,1,sizeof(var),File)
#define fr1(var,size) fread(var,1,size,File)
bool VICII::LoadFreez(FILE *File,unsigned short Version)
{
	int Offset;

	switch(Version)
	{
	case 0x0100:
	case 0x0101:

		fr(Offset);
        video_buffer_line = Offset + video_buffer;
		
        fr(draw_line_counter);
        fr(draw_this_line);
		fr(bitc);
        fr1(reg_mx,8);
        fr1(reg_my,8);
        fr(reg_mx8);
        fr(reg_ctrl_1);
        fr(reg_ctrl_2);
        fr(reg_y_scroll);
        fr(reg_x_scroll);
        fr(reg_vbase);
        fr(reg_irq_raster);
        fr(reg_lpx);
        fr(reg_lpy);
        fr(reg_me);
        fr(reg_mye);
        fr(reg_mdp);
        fr(reg_mmc);
        fr(reg_mxe);
		fr(MM);
        fr(reg_md);
		fr(EC);
        fr(reg_b0c);
        fr(reg_b1c);
        fr(reg_b2c);
        fr(reg_b3c);
        fr(reg_mm0);
        fr(reg_mm1);
        fr1(reg_mcolor,8);
        fr(current_cycle);
        fr(current_rasterline);
		fr(AktXKoordinate);
        fr(graphic_mode);
        fr(irq_flag);
        fr(irq_mask);
		fr(DEN);
		fr(VBlanking);
		fr(FirstBAZyklus);
        fr(badline_enable);
        fr(badline_status);
        fr(display_status);
        fr(lp_triggered);
		fr(RC);
        fr(vc);
        fr(vc_base);
        fr(matrix_base);
        fr(char_base);
        fr(bitmap_base);
        fr(vmli);
        fr(c_address);
        fr(g_address);
        fr1(c_value_buffer_8bit,64);
        fr1(c_value_buffer_4bit,64);
        fr(gfx_data);
        fr(char_data);
        fr(color_data);
		fr(GreyDotEnable);
        fr(is_write_reg_ec);
        fr(is_write_reg_b0c);

		//////////////////////////////// Sprites //////////////////////////////////

        fr(sprite_y_exp_flip_flop);
		fr1(MC,8);
        fr1(mc_base,8);
        fr(sprite_dma);
		fr(SpriteView);

		///////////////////////////////////////////////////////////////////////////

		////////////////////////////// Rahmenstufe ////////////////////////////////

        fr(csel);
        fr(rsel);
		fr(BorderCmpYo);
		fr(BorderCmpYu);
        fr(border_flip_flop0);
        fr(border_flip_flop1);
		fr(BorderCMP_Re);
		fr(BorderCMP_Li);
		fr(BorderCMP_Ob);
		fr(BorderCMP_Un);
		break;
	}
	return true;
}
*/

void VICII::WriteIO(uint16_t address, uint8_t value)
{
    address &= 63;
    if (address==0x00 || address==0x02 || address==0x04 || address==0x06 || address==0x08 || address==0x0A || address==0x0C || address==0x0E)
		/// Sprite X Position
            reg_mx[address >> 1] = (reg_mx[address >> 1] & 0xFF00) | value;
    else if (address==0x01 || address==0x03 || address==0x05 || address==0x07 || address==0x09 || address==0x0B || address==0x0D || address==0x0F)
		/// Sprite Y Position
            reg_my[address >> 1] = value;
	else if (address==0x10) {
		/// Sprite X Position MSB
			int i, j;
            reg_mx8 = value;
			for (i=0, j=1; i<8; i++, j<<=1)
			{
                if (reg_mx8 & j) reg_mx[i] |= 0x100;
                else reg_mx[i] &= 0xFF;
			}
    }
	else if (address==0x11) {
		// Control Register 1
                        reg_ctrl_1 = value;
                        reg_y_scroll = value & 7;

                        new_irq_raster = static_cast<uint16_t>((reg_irq_raster & 0xFF) | ((value & 0x80) << 1));
                        if (reg_irq_raster != new_irq_raster && current_rasterline == new_irq_raster) RasterIRQ();
                        reg_irq_raster = new_irq_raster;
			
                        graphic_mode = ((reg_ctrl_1 & 0x60) | (reg_ctrl_2 & 0x10)) >> 4;
		
			// Prüfen ob Badlines zugelassen sind
                        if ((current_rasterline == 0x30) && (value & 0x10)) badline_enable = true;

			// Prüfen auf Badline zustand
                        if((current_rasterline>=0x30) && (current_rasterline<=0xF7) && (reg_y_scroll == (current_rasterline&7)) && (badline_enable == true))
			{
                                badline_status = true;
			}
            else badline_status = false;

			// RSEL
            rsel = (value & 0x08)>>3;

			// DEN
            den = (value & 0x10)>>4;
            write_reg_0x11 = true;
    }
	else if (address==0x12) {
		/// Rasterzähler
        new_irq_raster = (reg_irq_raster & 0xFF00) | value;
        if (reg_irq_raster != new_irq_raster && current_rasterline == new_irq_raster) RasterIRQ();
        reg_irq_raster = new_irq_raster;
    }
	else if (address==0x15) {
		/// Sprite Enable
            reg_me = value;
    }
	else if (address==0x16) {
		// Control Register 2
            reg_ctrl_2 = value;
            reg_x_scroll = value & 7;
            graphic_mode = ((reg_ctrl_1 & 0x60) | (reg_ctrl_2 & 0x10)) >> 4;
			
            // csel
            csel = (value & 0x08)>>3;
    }
	else if (address==0x17) {
		/// Sprite Y-Expansion
                        reg_mye = value;
                        sprite_y_exp_flip_flop |= ~value;
    }
	else if (address==0x18) {
		/// Speicher Pointer
                        reg_vbase = value;
                        matrix_base = static_cast<uint16_t>((value & 0xf0) << 6);
                        char_base = static_cast<uint16_t>((value & 0x0e) << 10);
                        bitmap_base = static_cast<uint16_t>((value & 0x08) << 10);
    }
	else if (address==0x19) {
		/// IRQ Flags
                        irq_flag = irq_flag & (~value & 0x0F);
                        if (irq_flag & irq_mask) irq_flag |= 0x80;
                        else CpuClearInterrupt(VIC_IRQ);
    }
	else if (address==0x1A) {
		/// IRQ Mask
                        irq_mask = value & 0x0F;
                        if (irq_flag & irq_mask)
			{
                                irq_flag |= 0x80;
                                CpuTriggerInterrupt(VIC_IRQ);
			} else {
                                irq_flag &= 0x7F;
                                CpuClearInterrupt(VIC_IRQ);
			}
    }
	else if (address==0x1B) {
		/// Sprite Daten Priorität
                        reg_mdp = value;
    }
	else if (address==0x1C) {
		/// Sprite Multicolor
                        reg_mmc = value;
    }
	else if (address==0x1D) {
		/// Sprite X-Expansion
                        reg_mxe = value;
    }
	else if (address==0x20) {
		/// Rahmenfarbe
                        reg_ec = value&15;
                        if(vic_config[VIC_GREY_DOTS_ON]) is_write_reg_ec = true;
    }
	else if (address==0x21) {
		/// Hintergrundfarbe 0
                        reg_b0c = value&15;
                        if(vic_config[VIC_GREY_DOTS_ON]) is_write_reg_b0c = true;
    }
	else if (address==0x22) {
		/// Hintergrundfarbe 1
        reg_b1c	= value&15;
                        //isWriteColorReg = true;
    }
	else if (address==0x23) {
		/// Hintergrundfarbe 2
        reg_b2c	= value&15;
                        //isWriteColorReg = true;
    }
	else if (address==0x24) {
		/// Hintergrundfarbe 3
        reg_b3c	= value&15;
                        //isWriteColorReg = true;
    }
	else if (address==0x25) {
		/// Sprite Multicolor 0
        reg_mm0  = value&15;
                        //isWriteColorReg = true;
    }
	else if (address==0x26)
		/// Sprite Multicolor 1
        reg_mm1  = value & 15;
                        //isWriteColorReg = true;
    else if (address==0x27 || address==0x28 || address==0x29 || address==0x2A || address==0x2B || address==0x2C || address==0x2D || address==0x2E)
		/// Sprite Farbe
        reg_mcolor[address - 0x27] = value & 15;
}

uint8_t VICII::ReadIO(uint16_t address)
{
    uint8_t ret;
    address &= 63;

    if (address==0x00 || address==0x02 || address==0x04 || address==0x06 || address==0x08 || address==0x0A || address==0x0C || address==0x0E)
		/// Sprite X Position
            return static_cast<uint8_t>(reg_mx[address >> 1]);
    if (address==0x01 || address==0x03 || address==0x05 || address==0x07 || address==0x09 || address==0x0B || address==0x0D || address==0x0F)
		/// Sprite Y Position
            return reg_my[address >> 1];
	if (address==0x10)
		/// Sprite X Position MSB
            return reg_mx8;
		/// Control Register 1
	if (address==0x11)
            if(write_reg_0x11) return (reg_ctrl_1 & 0x7F) | ((current_rasterline & 0x100) >> 1);
            else return 0;
		/// Rasterzähler
	if (address==0x12)
            return static_cast<uint8_t>(current_rasterline);
		/// Ligthpen X
	if (address==0x13)
            return reg_lpx;
		/// Ligthpen Y
	if (address==0x14)
            return reg_lpy;
	if (address==0x15) // Sprite enable
            return reg_me;
		// Control Register 2
	if (address==0x16)
            return reg_ctrl_2 | 0xC0;
		/// Sprite Y-Expansion
	if (address==0x17)
            return reg_mye;
		/// Speicher Pointer
	if (address==0x18)
            return reg_vbase | 0x01;
		/// IRQ Flags
	if (address==0x19)
            return irq_flag | 0x70;
		/// IRQ Mask
	if (address==0x1A)
            return irq_mask | 0xF0;
		/// Sprite Daten Priorität
	if (address==0x1B)
            return reg_mdp;
		/// Sprite Multicolor
	if (address==0x1C)
            return reg_mmc;
		/// Sprite X-Expansion
	if (address==0x1D)
            return reg_mxe;
		/// Sprite - Sprite Kollision
	if (address==0x1E) {
            ret = reg_mm;
            reg_mm = 0;
            return ret;
    }
		/// Sprite - Daten Kollision
	if (address==0x1F) {
            ret = reg_md;
            reg_md = 0;
            return ret;
    }
    if (address==0x20) return reg_ec | 0xf0;
    if (address==0x21) return reg_b0c | 0xf0;
    if (address==0x22) return reg_b1c | 0xf0;
    if (address==0x23) return reg_b2c | 0xf0;
    if (address==0x24) return reg_b3c | 0xf0;
    if (address==0x25) return reg_mm0 | 0xf0;
    if (address==0x26) return reg_mm1 | 0xf0;
    if (address==0x27 || address==0x28 || address==0x29 || address==0x2A || address==0x2B || address==0x2C || address==0x2D || address==0x2E)
            return reg_mcolor[address - 0x27] | 0xf0;
    return 0xFF;
}

void VICII::TriggerLightpen()
{
    if(!lp_triggered)
	{
        lp_triggered = true;

        reg_lpx = static_cast<uint8_t>(current_x_coordinate >> 1);
        reg_lpy = static_cast<uint8_t>(current_rasterline);

        irq_flag |= 0x08;
        if (irq_mask & 0x08)
		{
            irq_flag |= 0x80;
            CpuTriggerInterrupt(VIC_IRQ);
		}
	}
}
