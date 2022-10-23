#include <stdio.h>
#include "ibm.h"
#include "io.h"
#include "nvr.h"
#include "nvr_mm58174.h"
#include "rtc_mm58174.h"
#include "pic.h"
#include "timer.h"
#include "rtc.h"
#include "paths.h"
#include "config.h"
#include "nmi.h"

static pc_timer_t nvr_onesec_timer;
static int nvr_onesec_cnt = 0;

static void mm58174_onesec(void *p) {
        nvr_onesec_cnt++;
        if (nvr_onesec_cnt >= 100) {
                mm58174_tick();
                mm58174_get(nvrram);
                nvr_onesec_cnt = 0;
        }
        timer_advance_u64(&nvr_onesec_timer, 10000 * TIMER_USEC);
}

void write_mm58174(uint16_t addr, uint8_t val, void *priv) {
        uint8_t old;

        addr &= 0x0F;
        val &= 0x0F;
        
        old = nvrram[addr];
        nvrram[addr] = val;

        /* pclog("MM58174 updating register %d: [%d => %d]", addr, old, val); */

        /* Update non-read-only changed values if not synchronizing time to host */
        if ((addr != MM58174_TENTHS) && (addr != MM58174_SECOND1) && (addr != MM58174_SECOND10)) {
                if ((old != val) && !enable_sync) {
                        mm58174_time_update(nvrram, addr);
                        nvr_dosave = 1;
                        /* pclog(" [Changed]"); */
                }
        }

        /* pclog("\n"); */

        if ((addr == MM58174_RESET) && (nvrram[addr] & 0x01)) {
                /* When timer starts, MM58174 sets seconds and tenths of second to 0 */
                nvrram[MM58174_TENTHS] = 0;
                mm58174_time_update(nvrram, MM58174_TENTHS);
                if (!enable_sync) {
                        /* Only set seconds to 0 if not synchronizing time to host clock*/
                        nvrram[MM58174_SECOND1] = 0;
                        mm58174_time_update(nvrram, MM58174_SECOND1);
                        nvrram[MM58174_SECOND10] = 0;
                        mm58174_time_update(nvrram, MM58174_SECOND10);
                        nvr_dosave = 1;
                }
        }
}

uint8_t read_mm58174(uint16_t addr, void *priv) {
        addr &= 0x0F;
        if (addr == 0x0F) {
                nvrram[addr] &= 0x07; /* Set IRQ control bit to 0 upon read */
        }

        return nvrram[addr];
}

void mm58174_loadnvr() {
        FILE *f;

        nvrmask = 63;
        oldromset = romset;
        switch (romset) {
        case ROM_OLIM24:
                f = nvrfopen("m24.nvr", "rb");
                break;
        default:
                return;
        }
        if (!f) {
                memset(nvrram, 0xFF, 64);
                nvrram[MM58174_TEST] = 0; /* Test register */
                if (!enable_sync) {
                        memset(nvrram, 0, 16);
                }
                return;
        }
        fread(nvrram, 64, 1, f);
        if (enable_sync)
                mm58174_time_internal_sync(nvrram);
        else
                mm58174_time_internal_set_nvrram(nvrram); /* Update the internal clock state based on the NVR registers */
        fclose(f);
}

void mm58174_savenvr() {
        FILE *f;
        switch (oldromset) {
        case ROM_OLIM24:
                f = nvrfopen("m24.nvr", "wb");
                break;
        default:
                return;
        }
        fwrite(nvrram, 64, 1, f);
        fclose(f);
}

void nvr_mm58174_init() {
        io_sethandler(0x0070, 0x0010, read_mm58174, NULL, NULL, write_mm58174, NULL, NULL, NULL);
        timer_add(&nvr_onesec_timer, mm58174_onesec, NULL, 1);
}
