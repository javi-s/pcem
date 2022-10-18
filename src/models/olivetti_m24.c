#include "ibm.h"
#include "io.h"
#include "olivetti_m24.h"
#include "fdd.h"

uint8_t olivetti_m24_read(uint16_t port, void *priv) {
        uint8_t dip_switch = 0x00; 

        switch (port) {
        case 0x66:
        /*   Group 0 DIP-Switches                       */
        /*   8 7 6 5 4 3 2 1                            */
        /*                 1  --  0x01 128 KB           */
        /*               1    --  0x02 256 KB           */
        /*               1 1  --  0x03 384 KB (2 banks) */
        /*           1        --  0x08 512 KB           */
        /*           1     1  --  0x09 640 KB (2 banks) */
        /*         1          --  0x10 8087 installed   */
        /*   1                --  0x80 RAM bank 0 and 1 */
                switch (mem_size) {
                case 128:
                        dip_switch |= 0x01;
                        break;
                case 256:
                        dip_switch |= 0x02;
                        break;
                case 384:
                        dip_switch |= 0x03 | 0x80;
                        break;
                case 512:
                        dip_switch |= 0x08;
                        break;
                case 640:
                        dip_switch |= 0x09 | 0x80;
                        break;
                default:
                        break;
                }

                if (hasfpu)
                        dip_switch |= 0x10;

                return dip_switch;
        case 0x67:
        /*   Group 1 DIP-Switches                       */
        /*   8 7 6 5 4 3 2 1                            */
        /*                 1  --  0x01 720 KB FD        */
        /*               1    --  0x02 Fast startup     */
        /*             1      --  0x04 Internal HD off  */
        /*           1        --  0x08 Slow scroll      */
        /*       1            --  0x20 80x25 color      */
        /*     1              --  0x40 Two MFD enabled  */

                /* Identify if drive B is enabled and flag two drives */
                if (fdd_get_type(1))
                        dip_switch |= 0x40;

                dip_switch |= 0x02 | 0x04 | 0x20;

                return dip_switch;
        }
        return 0xff;
}

void olivetti_m24_init() { io_sethandler(0x0066, 0x0002, olivetti_m24_read, NULL, NULL, NULL, NULL, NULL, NULL); }
