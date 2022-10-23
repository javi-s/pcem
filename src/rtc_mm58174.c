/* Emulation of:
   National Semiconductor MM58174 Real Time Clock */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pcem/logging.h>
#include "rtc_mm58174.h"

#define peek2(a) (nvrram[(a##1)] + 10 * nvrram[(a##10)])

struct {
        int sec;
        int min;
        int hour;
        int mday;
        int mon;
        int year;
} internal_clock;

/* Table for days in each month */
static int rtc_days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* Called to determine whether the year is leap or not */
static int rtc_is_leap(int org_year) {
        if (org_year % 400 == 0)
                return 1;
        if (org_year % 100 == 0)
                return 0;
        if (org_year % 4 == 0)
                return 1;
        return 0;
}

/* Called to determine the days in the current month */
static int rtc_get_days(int org_month, int org_year) {
        if (org_month != 2)
                return rtc_days_in_month[org_month - 1];
        else
                return rtc_is_leap(org_year) ? 29 : 28;
}

/* Called when the internal clock gets updated */
static void mm58174_recalc() {
        if (internal_clock.sec == 60) {
                internal_clock.sec = 0;
                internal_clock.min++;
        }
        if (internal_clock.min == 60) {
                internal_clock.min = 0;
                internal_clock.hour++;
        }
        if (internal_clock.hour == 24) {
                internal_clock.hour = 0;
                internal_clock.mday++;
        }
        if (internal_clock.mday == (rtc_get_days(internal_clock.mon, internal_clock.year) + 1)) {
                internal_clock.mday = 1;
                internal_clock.mon++;
        }
        if (internal_clock.mon == 13) {
                internal_clock.mon = 1;
                internal_clock.year++;
        }
        /* pclog ("MM58174 - Tick: %02d:%02d:%02d %d-%d-%d\n",
               internal_clock.hour,
               internal_clock.min,
               internal_clock.sec,
               internal_clock.mday,
               internal_clock.mon,
               internal_clock.year); */
}

/* Called when ticking the second */
void mm58174_tick() {
        internal_clock.sec++;
        mm58174_recalc();
}

/* Called when modifying the NVR registers */
void mm58174_time_update(uint8_t *nvrram, int reg) {
        switch (reg) {
        case MM58174_SECOND1:
        case MM58174_SECOND10:
                internal_clock.sec = peek2(MM58174_SECOND);
                break;
        case MM58174_MINUTE1:
        case MM58174_MINUTE10:
                internal_clock.min = peek2(MM58174_MINUTE);
                break;
        case MM58174_HOUR1:
        case MM58174_HOUR10:
                /* MM58174 counts in 24-hour mode */
                internal_clock.hour = peek2(MM58174_HOUR);
                break;
        case MM58174_DAY1:
        case MM58174_DAY10:
                internal_clock.mday = peek2(MM58174_DAY);
                break;
        case MM58174_MONTH1:
        case MM58174_MONTH10:
                internal_clock.mon = peek2(MM58174_MONTH);
                break;
        case MM58174_IRQ:
                /* MM87174 does not store the year, M24 uses the IRQ register to count 8 years from leap year */
                internal_clock.year = 1984 + (nvrram[MM58174_IRQ] & 0x07);
                break;
        }
}

/* Called to obtain the current day of the week based on the internal clock */
static int time_week_day() {
        int day_of_month = internal_clock.mday;
        int month2 = internal_clock.mon;
        int year2 = internal_clock.year % 100;
        int century = ((internal_clock.year - year2) / 100) % 4;
        int sum = day_of_month + month2 + year2 + century;
        /* (Sum mod 7) gives 0 for Saturday, we need it for Sunday, so +6 for Saturday to get 6 and Sunday 0 */
        int raw_wd = ((sum + 6) % 7);
        return raw_wd;
}

/* Called to get time stored into the internal clock */
static void mm58174_time_internal_get(struct tm *time_var) {
        time_var->tm_sec = internal_clock.sec;
        time_var->tm_min = internal_clock.min;
        time_var->tm_hour = internal_clock.hour;
        time_var->tm_wday = time_week_day();
        time_var->tm_mday = internal_clock.mday;
        time_var->tm_mon = internal_clock.mon - 1;
        time_var->tm_year = internal_clock.year - 1900;
}

static void mm58174_time_internal_set(struct tm *time_var) {
        internal_clock.sec = time_var->tm_sec;
        internal_clock.min = time_var->tm_min;
        internal_clock.hour = time_var->tm_hour;
        internal_clock.mday = time_var->tm_mday;
        internal_clock.mon = time_var->tm_mon + 1;
        /* MM87174 does not store the year, M24 uses the IRQ register to count 8 years from base leap year */
        internal_clock.year = 1984 + ((time_var->tm_year + 1900) % 8);
}

static void mm58174_time_set_nvrram(uint8_t *nvrram, struct tm *cur_time_tm) {
        /* pclog ("MM58174 - Curtime: %02d:%02d:%02d %d-%d-%d\n",
               cur_time_tm->tm_hour,
               cur_time_tm->tm_min,
               cur_time_tm->tm_sec,
               cur_time_tm->tm_mday,
               cur_time_tm->tm_mon + 1,
               cur_time_tm->tm_year); */
        nvrram[MM58174_SECOND1] = cur_time_tm->tm_sec % 10;
        nvrram[MM58174_SECOND10] = cur_time_tm->tm_sec / 10;
        nvrram[MM58174_MINUTE1] = cur_time_tm->tm_min % 10;
        nvrram[MM58174_MINUTE10] = cur_time_tm->tm_min / 10;
        nvrram[MM58174_HOUR1] = cur_time_tm->tm_hour % 10;
        nvrram[MM58174_HOUR10] = cur_time_tm->tm_hour / 10;
        nvrram[MM58174_WEEKDAY] = cur_time_tm->tm_wday + 1;
        nvrram[MM58174_DAY1] = cur_time_tm->tm_mday % 10;
        nvrram[MM58174_DAY10] = cur_time_tm->tm_mday / 10;
        nvrram[MM58174_MONTH1] = (cur_time_tm->tm_mon + 1) % 10;
        nvrram[MM58174_MONTH10] = (cur_time_tm->tm_mon + 1) / 10;
        /* MM87174 does not store the year, M24 uses the IRQ register to count 8 years from leap year */
        nvrram[MM58174_IRQ] = (cur_time_tm->tm_year + 1900) % 8;
        nvrram[MM58174_LEAPYEAR] = 8 >> ((nvrram[MM58174_IRQ] & 0x07) & 0x03);
        /* pclog ("MM58174 - Saved: %02d:%02d:%02d %d-%d-%d - IRQ %d\n",
               peek2(MM58174_HOUR),
               peek2(MM58174_MINUTE),
               peek2(MM58174_SECOND),
               peek2(MM58174_DAY),
               peek2(MM58174_MONTH),
               1984 + (nvrram[MM58174_IRQ] & 0x07),
               nvrram[MM58174_IRQ]); */
}

void mm58174_time_internal_set_nvrram(uint8_t *nvrram) {
        /* Load the entire internal clock state from the NVR */
        internal_clock.sec = peek2(MM58174_SECOND);
        internal_clock.min = peek2(MM58174_MINUTE);
        internal_clock.hour = peek2(MM58174_HOUR);
        internal_clock.mday = peek2(MM58174_DAY);
        internal_clock.mon = peek2(MM58174_MONTH);
        /* MM87174 does not store the year, M24 uses the IRQ register to count 8 years from leap year */
        internal_clock.year = 1984 + (nvrram[MM58174_IRQ] & 0x07);
}

void mm58174_time_internal_sync(uint8_t *nvrram) {
        struct tm *cur_time_tm;
        time_t cur_time;

        time(&cur_time);
        cur_time_tm = localtime(&cur_time);

        mm58174_time_internal_set(cur_time_tm);

        mm58174_time_set_nvrram(nvrram, cur_time_tm);
}

void mm58174_get(uint8_t *nvrram) {
        struct tm cur_time_tm;

        mm58174_time_internal_get(&cur_time_tm);

        mm58174_time_set_nvrram(nvrram, &cur_time_tm);
}
