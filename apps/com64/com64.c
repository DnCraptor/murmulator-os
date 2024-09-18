#include "m-os-api.h"
#undef switch 
#include "structs.h"
#include "mmu.h"
#include "mos6510.h"
#include "mos6569.h"
#include "mos6526.h"
//#include "mos6581_8085.h"
#include "cartridge.h"
#include "reu.h"
#include "georam.h"
#include "tape1530.h"
#include "m-os-api-sdtfn.h"
#include "m-os-api-math.h"

#define C64Takt 985248  // 50,124542Hz (Original C64 PAL)
#define MAX_FLOPPY_NUM 4

static uint32_t cycle_counter;
static bool reset_wire = false;        // Reset Leitung -> Für Alle Module mit Reset Eingang
static bool game_wire = true;         // Leitung Expansionsport --> MMU;
static bool exrom_wire = true;        // Leitung Expansionsport --> MMU;
static bool hi_ram_wire = false;       // Leitung Expansionsport --> MMU;
static bool lo_ram_wire = false;       // Leitung Expansionsport --> MMU;
static bool rdy_ba_wire = true;       // Leitung CPU <-- VIC
static bool c64_reset_ready = false;
static bool enable_ext_wires;
static bool ext_rdy_wire;

static bool enable_stereo_sid;
static bool enable_stereo_sid_6channel_mode;
static uint16_t stereo_sid_address;

static uint8_t c64_iec_wire;       // Leitungen vom C64 zur Floppy Bit 4=ATN 6=CLK 7=DATA
static uint8_t floppy_iec_wire;    // Leitungen von Floppy zur c64 Bit 6=CLK 7=DATA

static int c64_frequency;   // Normaler PAL Takt ist 985248 Hz (985248 / (312 Rz * 63 Cycles) = 50,124542 Hz)
                            // 50 Hz Syncroner Takt ist 982800 Hz
static int c64_speed;

static bool cpu_states[MAX_FLOPPY_NUM + 1] = { false }; // true = Feetch / false = no Feetch ::: Index 0=C64 Cpu, 1-4 Floppy 1-4
static MMU* mmu;
static MOS6510* cpu;
static VICII* vic;
///static MOS6581_8085* sid1;
///static MOS6581_8085* sid2;
static MOS6526* cia1;
static MOS6526* cia2;
static Cartridge* crt;
static REU* reu;
static GEORAM* geo;
static TAPE1530* tape;

static void NextSystemCycle(void) {
    cycle_counter++;
    floppy_iec_wire = 0;
    for(int i = 0; i < MAX_FLOPPY_NUM; ++i)
    {
///        cpu_states[i+1] = floppy[i]->OneCycle();
///        floppy_iec_wire |= ~floppy[i]->FloppyIECLocal;
    }
    floppy_iec_wire = ~floppy_iec_wire;
    // PHI0
    if(enable_ext_wires) rdy_ba_wire = ext_rdy_wire;
    cpu_states[0] = MOS6510_OneZyklus(cpu);
    // PHI1
///    vic->OneCycle();
///    cia1->OneZyklus();
///    cia2->OneZyklus();
///    sid1->OneZyklus();
///    if(enable_stereo_sid) sid2->OneZyklus();
///    reu->OneZyklus();
///    tape->OneCycle();
    MOS6510_Phi1(cpu);
}

static void MainThread(void *userdat) {
    while (!marked_to_exit) { // TODO: ???
        sleep_ms(100);
    }
    vTaskDelete( NULL );
}

static void WarpThread(void *userdat) {
    while (!marked_to_exit) {
        NextSystemCycle();
    }
    vTaskDelete( NULL );
}

static void VicRefresh(void* ignore, unsigned char* vic_puffer) {
    // TODO:
}
/**
static void WriteSidIO(void* ignore, uint16_t address, uint8_t value)
{
    if(enable_stereo_sid)
    {
        if((address & 0xFFE0) == 0xD400) MOS6581_8085_WriteIO(sid1,address,value);
        if((address & 0xFFE0) == stereo_sid_address) MOS6581_8085_WriteIO(sid2,address,value);
    }
    else
    {
        MOS6581_8085_WriteIO(sid1,address,value);
    }
}
*/
int main() {
    marked_to_exit = false;

    ext_rdy_wire = false;
    enable_ext_wires = false;
    reset_wire = true;        // Reset Leitung -> Für Alle Module mit Reset Eingang
    game_wire = true;         // Leitung Expansionsport --> MMU;
    exrom_wire = true;        // Leitung Expansionsport --> MMU;
    hi_ram_wire = false;       // Leitung Expansionsport --> MMU;
    lo_ram_wire = false;       // Leitung Expansionsport --> MMU;
    rdy_ba_wire = true;       // Leitung CPU <-- VIC
    c64_reset_ready = false;
    enable_stereo_sid = false;
    enable_stereo_sid_6channel_mode = false;
    stereo_sid_address = 0xD420;
    c64_iec_wire = false;

    cmd_ctx_t* ctx = get_cmd_ctx();
    MOS6510_PORT* port = (MOS6510_PORT*)calloc(1, sizeof(MOS6510_PORT));
    MOS6510_PORT_MOS6510_PORT(port);

    mmu = (MMU*)calloc(1, sizeof(MMU));
    MMU_MMU(mmu, port);

    cpu = (MOS6510*)calloc(1, sizeof(MOS6510));
    MOS6510_MOS6510(cpu);

    vic = (VICII*)calloc(1, sizeof(VICII));
    VICII_VICII(vic);
/**
    sid2 = (MOS6581_8085*)calloc(1, sizeof(MOS6581_8085));
    MOS6581_8085_MOS6581_8085(sid1, 0, 44100, 1024);
    if (enable_stereo_sid) {
        sid2 = (MOS6581_8085*)calloc(1, sizeof(MOS6581_8085));
        MOS6581_8085_MOS6581_8085(sid1, 1, 44100, 1024);
    }
    else sid2 = 0;
*/
    cia1 = (MOS6526*)calloc(1, sizeof(MOS6526));
    MOS6526_MOS6526(cia1, 0);
    cia2 = (MOS6526*)calloc(1, sizeof(MOS6526));
    MOS6526_MOS6526(cia2, 1);

    crt = (Cartridge*)calloc(1, sizeof(Cartridge));
    Cartridge_Cartridge(crt);

    reu = (REU*)calloc(1, sizeof(REU));
    REU_REU(reu);

    geo = (GEORAM*)calloc(1, sizeof(GEORAM));
    GEORAM_GEORAM(geo);

    tape = (TAPE1530*)calloc(1, sizeof(TAPE1530));
///    TAPE1530_TAPE1530(tape, ...);
///    tape = new TAPE1530(audio_frequency,audio_spec_have.samples,C64Takt);

    cia2->floppy_iec_wire = &floppy_iec_wire;
    cia2->c64_iec_wire = &c64_iec_wire;

    int i;
    for(i = 0; i < MAX_FLOPPY_NUM; ++i) {} // TODO:

    c64_frequency = C64Takt;
    c64_speed = 100;
    /**
    cpu_pc_history_pos = 0;
    io_source = 0;
    c64_command_line_lenght = 0;
    c64_command_line_current_pos = 0;
    c64_command_line_status = false;
    c64_command_line_count_s = false;
    debug_mode = one_cycle = one_opcode = false;
    cycle_counter = 0;
	limit_cycles_counter = 0;
	hold_next_system_cycle = false;
    debug_animation = false;
    debug_logging = false;
    animation_speed_add = audio_spec_have.samples/audio_frequency;
    animation_speed_counter = 0;
    */

    cpu->ReadProcTbl = mmu->CPUReadProcTbl;
    cpu->WriteProcTbl = mmu->CPUWriteProcTbl;
    vic->ReadProcTbl = mmu->VICReadProcTbl;
    vic->RefreshProc.fn = VicRefresh;
    vic->RefreshProc.p = vic;
    reu->ReadProcTbl = mmu->CPUReadProcTbl;
    reu->WriteProcTbl = mmu->CPUWriteProcTbl;

    mmu->VicIOWriteProc.fn = VICII_WriteIO;
    mmu->VicIOWriteProc.p = vic;
    mmu->VicIOReadProc.fn = VICII_ReadIO;
    mmu->VicIOReadProc.p = vic;
///    mmu->SidIOWriteProc.fn = WriteSidIO;
///    mmu->SidIOWriteProc.p = sid1;
///    mmu->SidIOReadProc.fn = MOS6581_8085_ReadIO;
///    mmu->SidIOReadProc.p = sid1;
    //

    mmu->GAME = &game_wire;
    mmu->EXROM = &exrom_wire;
    mmu->RAM_H = &hi_ram_wire;
    mmu->RAM_L = &lo_ram_wire;
    // TODO:
    mmu->VicIOWriteProc;
    mmu->VicIOReadProc;

    mmu->SidIOWriteProc;
    mmu->SidIOReadProc;

    mmu->Cia1IOWriteProc;
    mmu->Cia2IOWriteProc;
    mmu->Cia1IOReadProc;
    mmu->Cia2IOReadProc;

    mmu->IO1WriteProc;
    mmu->IO2WriteProc;
    mmu->IO1ReadProc;
    mmu->IO2ReadProc;

    cpu->RDY = &rdy_ba_wire;
    cpu->RESET = &reset_wire;
    cpu->ResetReady = &c64_reset_ready;
    cpu->ResetReadyAdr = 0xE5CD;

    MMU_Reset(mmu);

    cycle_counter = 0;
    xTaskCreate(MainThread, "C64Main", 1024/*x4=4096*/, cpu, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(WarpThread, "C64Warp", 1024/*x4=4096*/, cpu, configMAX_PRIORITIES - 1, NULL);

    while (!marked_to_exit) { // TODO: ???
        sleep_ms(100);
    }
//
    free(tape);
    free(geo);
    free(crt->am29f040Hi);
    free(crt->am29f040Lo);
    free(crt);
    free(reu);
    free(cia2);
    free(cia1);
///    if(sid2) MOS6581_8085_MOS6581_8085_free(sid2);
///    MOS6581_8085_MOS6581_8085_free(sid1);
    free(vic);
    free(cpu);
    free(mmu);
    free(port);
    return 0;
}

#include "mmu.c"
#include "roms.c"
#include "mos6510.c"
#include "mos6569.c"
//#include "mos6581_8085.c"
