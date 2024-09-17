#include "m-os-api.h"
#undef switch 
#include "structs.h"
#include "mmu.h"
#include "mos6510.h"
#include "m-os-api-sdtfn.h"

static uint32_t cycle_counter;
static bool cpu_states[5] = { false }; // true = Feetch / false = no Feetch ::: Index 0=C64 Cpu, 1-4 Floppy 1-4
static MMU* mmu;
static MOS6510* cpu;

static void NextSystemCycle(void) {
    cycle_counter++;
    // PHI0
    cpu_states[0] = MOS6510_OneZyklus(cpu);
    // PHI1
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

    bool reset_wire = false;        // Reset Leitung -> Für Alle Module mit Reset Eingang
    bool game_wire = true;         // Leitung Expansionsport --> MMU;
    bool exrom_wire = true;        // Leitung Expansionsport --> MMU;
    bool hi_ram_wire = false;       // Leitung Expansionsport --> MMU;
    bool lo_ram_wire = false;       // Leitung Expansionsport --> MMU;
    bool rdy_ba_wire = true;       // Leitung CPU <-- VIC
    bool c64_reset_ready = false;

    cmd_ctx_t* ctx = get_cmd_ctx();

    MOS6510_PORT* port = (MOS6510_PORT*)calloc(1, sizeof(MOS6510_PORT));
    MOS6510_PORT_MOS6510_PORT(port);

    mmu = (MMU*)calloc(1, sizeof(MMU));
    MMU_MMU(mmu, port);
    mmu->GAME = &game_wire;
    mmu->EXROM = &exrom_wire;
    mmu->RAM_H = &hi_ram_wire;
    mmu->RAM_L = &lo_ram_wire;

    cpu = (MOS6510*)calloc(1, sizeof(MOS6510));
    MOS6510_MOS6510(cpu);
    cpu->RDY = &rdy_ba_wire;
    cpu->RESET = &reset_wire;
    cpu->ResetReady = &c64_reset_ready;
    cpu->ResetReadyAdr = 0xE5CD;

    MMU_Reset(mmu);

// TODO: ???
    cpu->SP = 0xFD;
    cpu->PC = MOS6510_Read(cpu, 0xFFFC) | (MOS6510_Read(cpu, 0xFFFD) << 8);

    cycle_counter = 0;
    xTaskCreate(MainThread, "C64Main", 1024/*x4=4096*/, cpu, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(WarpThread, "C64Warp", 1024/*x4=4096*/, cpu, configMAX_PRIORITIES - 1, NULL);

    while (!marked_to_exit) { // TODO: ???
        sleep_ms(100);
    }
//
    free(cpu);
    free(mmu);
    free(port);
    return 0;
}

#include "mmu.c"
#include "roms.c"
#include "mos6510.c"
