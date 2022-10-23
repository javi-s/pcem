
/* The MM58174 is a 4-bit RTC, so each memory location can only hold a single
 * BCD digit. Hence everything has 'ones' and 'tens' digits. */

#ifndef _RTC_MM58174_H_
#define _RTC_MM58174_H_
enum MM58174_ADDR {
        /* Registers */
        MM58174_TEST,       /* TEST register, write only */
        MM58174_TENTHS,     /* Tenths of second, read only */
        MM58174_SECOND1,    /* Units of seconds, read only */
        MM58174_SECOND10,   /* Tens of seconds, read only */
        MM58174_MINUTE1,
        MM58174_MINUTE10,
        MM58174_HOUR1,
        MM58174_HOUR10,
        MM58174_DAY1,
        MM58174_DAY10,
        MM58174_WEEKDAY,
        MM58174_MONTH1,
        MM58174_MONTH10,
        MM58174_LEAPYEAR,   /* Leap year status, write only */
        MM58174_RESET,      /* RESET register, write only */
        MM58174_IRQ         /* Interrupt register, read / write */
};

void mm58174_tick();
void mm58174_time_update(uint8_t *nvrram, int reg);
void mm58174_get(uint8_t *nvrram);
void mm58174_time_internal_set_nvrram(uint8_t *nvrram);
void mm58174_time_internal_sync(uint8_t *nvrram);

#endif /* _RTC_MM58174_H_ */
