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
    cmd_ctx_t* ctx = get_cmd_ctx();

    MOS6510_PORT* port = (MOS6510_PORT*)calloc(1, sizeof(MOS6510_PORT));
    MOS6510_PORT_MOS6510_PORT(port);

    mmu = (MMU*)calloc(1, sizeof(MMU));
    MMU_MMU(mmu, port);

    cpu = (MOS6510*)calloc(1, sizeof(MOS6510));
    MOS6510_MOS6510(cpu);

    MMU_Reset(mmu);

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
