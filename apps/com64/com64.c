#include "m-os-api.h"
#undef switch 
#include "structs.h"
#include "mmu.h"
#include "mos6510.h"
#include "mos6569.h"
#include "m-os-api-sdtfn.h"
#include "m-os-api-math.h"

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

static bool cpu_states[5] = { false }; // true = Feetch / false = no Feetch ::: Index 0=C64 Cpu, 1-4 Floppy 1-4
static MMU* mmu;
static MOS6510* cpu;
static VICII* vic;

static void NextSystemCycle(void) {
    cycle_counter++;
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

    cmd_ctx_t* ctx = get_cmd_ctx();

    MOS6510_PORT* port = (MOS6510_PORT*)calloc(1, sizeof(MOS6510_PORT));
    MOS6510_PORT_MOS6510_PORT(port);

    mmu = (MMU*)calloc(1, sizeof(MMU));
    MMU_MMU(mmu, port);

    cpu = (MOS6510*)calloc(1, sizeof(MOS6510));
    MOS6510_MOS6510(cpu);

    vic = (VICII*)calloc(1, sizeof(VICII));
    VICII_VICII(vic);

    cpu->ReadProcTbl = mmu->CPUReadProcTbl;
    cpu->WriteProcTbl = mmu->CPUWriteProcTbl;
    vic->ReadProcTbl = mmu->VICReadProcTbl;
///    vic->RefreshProc.fn = VicRefresh;
    vic->RefreshProc.p = vic; // ???

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
