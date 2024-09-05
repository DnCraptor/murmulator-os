#include "m-os-api.h"
#include "m-os-api-sdtfn.h"
#include "m-os-api-timer.h"

#define RGBVAL8(r,g,b) (0xC0 | (((r & 0xFF) / 42) << 4) | (((g & 0xFF) / 42) << 2) | ((b & 0xFF) / 42))
#define RGBVAL16(r,g,b) ((RGBVAL8(r, g, b) << 8) | RGBVAL8(r, g, b))

inline static unsigned long millis() { 
  return timer_hw->timerawl / 1000;
}

#include "emuapi.h"

// TODO: ensure values
#define CRYSTAL       	17734475.0f
#define CLOCKSPEED      ( CRYSTAL / 18.0f) // 985248,61 Hz
#define LINECNT         312 //Rasterlines
#define CYCLESPERRASTERLINE 63
#define WRITE_ATN_CLK_DATA(value) {}
#define READ_CLK_DATA() (0)
#define LINEFREQ      			(CLOCKSPEED / CYCLESPERRASTERLINE) //Hz
#define EXACTTIMINGDURATION 600ul //ms exact timing after IEC-BUS activity

static int skip = 0;

#include "pla.h"

inline static void c64_Init(void)
{
  disableEventResponder();
  resetPLA();
  resetCia1();
  resetCia2();
  resetVic();
  cpu_reset();
#ifdef HAS_SND  
  playSID.begin();
  emu_sndInit();
#endif  
}

int main() {
    skip = 0;
	marked_to_exit = false;
    cmd_ctx_t* ctx = get_cmd_ctx();
//    if (ctx->argc != 2) {
//        fprintf(ctx->std_err, "Pls. do not miss to specify ROM filename...\n");
//        return -1;
//    }
    emu_init();
    emu_start();
    emu_Init(ctx->argc > 1 ? ctx->argv[1] : NULL);
    while (!marked_to_exit) {
        emu_Step();   
    }
    return 0;
}

static unsigned char  palette8[PALETTE_SIZE];
static unsigned short palette16[PALETTE_SIZE];

void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index)
{
    if (index<PALETTE_SIZE) {
        palette8[index]  = RGBVAL8(r,g,b);
        palette16[index]  = RGBVAL16(r,g,b);        
    }
}

void emu_DrawVsync(void)
{
    skip += 1;
    skip &= VID_FRAME_SKIP;
#ifdef USE_VGA   
//    tft.waitSync();                   
#else                      
//    volatile bool vb=vbl; 
//    while (vbl==vb) {};
#endif
}


void emu_DrawLine(unsigned char * VBuf, int width, int height, int line) 
{
    if (skip == 0) {
#ifdef USE_VGA                        
///         tft.writeLine(width,height,line, VBuf, palette8);
#else
         tft.writeLine(width,height,line, VBuf, palette16);
#endif      
    }  
}  

void emu_DrawLine8(unsigned char * VBuf, int width, int height, int line) 
{
    if (skip == 0) {
#ifdef USE_VGA                        
      ///tft.writeLine(width,height,line, VBuf);      
#endif      
    }
} 

void emu_DrawLine16(unsigned short * VBuf, int width, int height, int line) 
{
    if (skip == 0) {
#ifdef USE_VGA        
        ///tft.writeLine16(width,height,line, VBuf);
#else
        tft.writeLine(width,height,line, VBuf);
#endif        
    }
}  

void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride) 
{
    if (skip == 0) {
#ifdef USE_VGA                
        ///tft.writeScreen(width,height-TFT_VBUFFER_YCROP,stride, VBuf+(TFT_VBUFFER_YCROP/2)*stride, palette8);
#else
        tft.writeScreen(width,height-TFT_VBUFFER_YCROP,stride, VBuf+(TFT_VBUFFER_YCROP/2)*stride, palette16);
#endif
    }
}

int emu_FrameSkip(void)
{
    return skip;
}

void * emu_LineBuffer(int line)
{
    // TODO: ensure
    return get_buffer() + line * 320;///(void*)tft.getLineBuffer(line);    
}

#ifdef HAS_SND
#include "AudioPlaySystem.h"
AudioPlaySystem mymixer;

void emu_sndInit() {
  tft.begin_audio(256, mymixer.snd_Mixer);
  mymixer.start();    
}

void emu_sndPlaySound(int chan, int volume, int freq)
{
  if (chan < 6) {
    mymixer.sound(chan, freq, volume); 
  }
}

void emu_sndPlayBuzz(int size, int val) {
  mymixer.buzz(size,val); 
}

#endif

struct {
  union {
    uint32_t kv;
    struct {
      uint8_t ke,   //Extratasten SHIFT, STRG, ALT...
              kdummy,
              k,    //Erste gedrückte Taste
              k2;   //Zweite gedrückte Taste
    };
  };
  uint32_t lastkv;
  uint8_t shiftLock;
} kbdData = {0, 0, 0};

#include "cpu.h"
extern struct tcpu cpu;

uint8_t cia1PORTA(void) {

  uint8_t v;

  v = ~cpu.cia1.R[0x02] | (cpu.cia1.R[0x00] & cpu.cia1.R[0x02]);
  int keys = emu_GetPad();
#ifndef PICOMPUTER
  /*
  if (oskbActive) keys = 0;
  */
#endif  
  if (!cpu.swapJoysticks) {
    if (keys & MASK_JOY2_BTN) v &= 0xEF;
    if (keys & MASK_JOY2_UP) v &= 0xFE;
    if (keys & MASK_JOY2_DOWN) v &= 0xFD;
    if (keys & MASK_JOY2_RIGHT) v &= 0xFB;
    if (keys & MASK_JOY2_LEFT) v &= 0xF7;
  } else {
    if (keys & MASK_JOY1_BTN) v &= 0xEF;
    if (keys & MASK_JOY1_UP) v &= 0xFE;
    if (keys & MASK_JOY1_DOWN) v &= 0xFD;
    if (keys & MASK_JOY1_RIGHT) v &= 0xFB;
    if (keys & MASK_JOY1_LEFT) v &= 0xF7;
  }	


  if (!kbdData.kv) return v; //Keine Taste gedrückt

  uint8_t filter = ~cpu.cia1.R[0x01] & cpu.cia1.R[0x03];
/***
  if (kbdData.k) {
    if ( keymatrixmap[1][kbdData.k] & filter)  v &= ~keymatrixmap[0][kbdData.k];
  }
  if (kbdData.ke) {
    if (kbdData.ke & 0x02) { //Shift-links
      if ( keymatrixmap[1][0xff] & filter) v &= ~keymatrixmap[0][0xff];
    }
    if (kbdData.ke & 0x20) { //Shift-rechts
      if ( keymatrixmap[1][0xfe] & filter) v &= ~keymatrixmap[0][0xfe];
    }
    if (kbdData.ke & 0x11) { //Control
      if ( keymatrixmap[1][0xfd] & filter) v &= ~keymatrixmap[0][0xfd];
    }
    if (kbdData.ke & 0x88) { //Windows (=> Commodore)
      if ( keymatrixmap[1][0xfc] & filter) v &= ~keymatrixmap[0][0xfc];
    }
  }
**/
  return v;
}

uint8_t cia1PORTB(void) {
  uint8_t v;
  v = ~cpu.cia1.R[0x03] | (cpu.cia1.R[0x00] & cpu.cia1.R[0x02]) ;
  int keys = emu_GetPad();
#ifndef PICOMPUTER
  /*
  if (oskbActive) keys = 0;
  */
#endif  
  if (!cpu.swapJoysticks) {
    if (keys & MASK_JOY1_BTN) v &= 0xEF;
    if (keys & MASK_JOY1_UP) v &= 0xFE;
    if (keys & MASK_JOY1_DOWN) v &= 0xFD;
    if (keys & MASK_JOY1_RIGHT) v &= 0xFB;
    if (keys & MASK_JOY1_LEFT) v &= 0xF7;
  } else {
    if (keys & MASK_JOY2_BTN) v &= 0xEF;
    if (keys & MASK_JOY2_UP) v &= 0xFE;
    if (keys & MASK_JOY2_DOWN) v &= 0xFD;
    if (keys & MASK_JOY2_RIGHT) v &= 0xFB;
    if (keys & MASK_JOY2_LEFT) v &= 0xF7;
  }

  if (!kbdData.kv) return v; //Keine Taste gedrückt

  uint8_t filter = ~cpu.cia1.R[0x00] & cpu.cia1.R[0x02];
/**
  if (kbdData.k) {
    if ( keymatrixmap[0][kbdData.k] & filter) v &= ~keymatrixmap[1][kbdData.k];
  }
  if (kbdData.ke) {
    if (kbdData.ke & 0x02) { //Shift-links
      if ( keymatrixmap[0][0xff] & filter) v &= ~keymatrixmap[1][0xff];
    }
    if (kbdData.ke & 0x20) { //Shift-rechts
      if ( keymatrixmap[0][0xfe] & filter) v &= ~keymatrixmap[1][0xfe];
    }
    if (kbdData.ke & 0x11) { //Control
      if ( keymatrixmap[0][0xfd] & filter) v &= ~keymatrixmap[1][0xfd];
    }
    if (kbdData.ke & 0x88) { //Windows (=> Commodore)
      if ( keymatrixmap[0][0xfc] & filter) v &= ~keymatrixmap[1][0xfc];
    }
  }
**/
  return v;
}

#include "pla.c"
#include "emuapi.c"

/* IRAM_ATTR */
static void oneRasterLine(void) {
  static unsigned short lc = 1;
  while (true) {
    cpu.lineStartTime = fbmicros(); //get_ccount();
    cpu.lineCycles = cpu.lineCyclesAbs = 0;
    if (!cpu.exactTiming) {
      vic_do();
    } else {
      vic_do_simple();
    }
    if (--lc == 0) {
      lc = LINEFREQ / 10; // 10Hz
      cia1_checkRTCAlarm();
      cia2_checkRTCAlarm();
    }
    //Switch "ExactTiming" Mode off after a while:
    if (!cpu.exactTiming) break;
    if ( (fbmicros() - cpu.exactTimingStartTime)*1000 >= EXACTTIMINGDURATION ) {
      cpu_disableExactTiming();
      break;
    }
  };

}

void c64_Start(char * filename) { }
void disableEventResponder() { }
void c64_Step(void) {
	oneRasterLine();
}
