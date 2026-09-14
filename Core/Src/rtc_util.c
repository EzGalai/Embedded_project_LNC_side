#include "rtc_util.h"
#include "main.h" /* for hrtc */
#include <stdio.h>

/**
 * @brief Converts days-since-1970-01-01 into a proleptic-Gregorian
 * (year, month, day). Howard Hinnant's public-domain civil_from_days
 * algorithm — small, well-tested, and correct for every year this project
 * will ever run in, without needing a full date/time library.
 */
static void CivilFromDays(int32_t z, int *year, uint8_t *month, uint8_t *day)
{
    z += 719468; /* shift epoch from 1970-01-01 to 0000-03-01 */
    int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);                        /* [0, 146096] */
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; /* [0, 399] */
    int32_t y = (int32_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);             /* [0, 365] */
    uint32_t mp = (5 * doy + 2) / 153;                                  /* [0, 11] */
    uint32_t d = doy - (153 * mp + 2) / 5 + 1;                          /* [1, 31] */
    uint32_t m = mp + (mp < 10 ? 3 : (uint32_t)-9);                     /* [1, 12] */

    *year = (int)(y + (m <= 2 ? 1 : 0));
    *month = (uint8_t)m;
    *day = (uint8_t)d;
}

/**
 * @brief Converts a proleptic-Gregorian (year, month, day) into
 * days-since-1970-01-01. Howard Hinnant's public-domain days_from_civil
 * algorithm — the exact inverse of CivilFromDays above.
 */
static int32_t DaysFromCivil(int year, uint8_t month, uint8_t day)
{
    int32_t y = year - (month <= 2 ? 1 : 0);
    int32_t era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);                                          /* [0, 399] */
    uint32_t doy = (153u * (uint32_t)((int)month + (month > 2 ? -3 : 9)) + 2u) / 5u + day - 1u; /* [0, 365] */
    uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;                            /* [0, 146096] */
    return era * 146097 + (int32_t)doe - 719468;
}

uint32_t RtcUtil_GetUnixTime(void)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate; /* must read Time immediately before Date — see note above */
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    int32_t days = DaysFromCivil(2000 + sDate.Year, sDate.Month, sDate.Date);
    uint32_t secondsOfDay = (uint32_t)sTime.Hours * 3600u + (uint32_t)sTime.Minutes * 60u + sTime.Seconds;

    return (uint32_t)days * 86400u + secondsOfDay;
}

void RtcUtil_FormatDate(uint32_t unixTime, char *outBuf)
{
    int32_t days = (int32_t)(unixTime / 86400u);
    int year;
    uint8_t month, day;
    CivilFromDays(days, &year, &month, &day);
    sprintf(outBuf, "%04d-%02u-%02u", year, month, day);
}

void RtcUtil_SetFromUnixTime(uint32_t unixTime)
{
    int32_t days = (int32_t)(unixTime / 86400u);
    uint32_t secondsOfDay = unixTime % 86400u;

    int year;
    uint8_t month, day;
    CivilFromDays(days, &year, &month, &day);

    /* 1970-01-01 was a Thursday (HAL's RTC_WEEKDAY_THURSDAY == 4) */
    uint8_t weekday = (uint8_t)(((days + 3) % 7 + 7) % 7 + 1);

    RTC_TimeTypeDef sTime = {0};
    sTime.Hours   = (uint8_t)(secondsOfDay / 3600u);
    sTime.Minutes = (uint8_t)((secondsOfDay % 3600u) / 60u);
    sTime.Seconds = (uint8_t)(secondsOfDay % 60u);
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    RTC_DateTypeDef sDate = {0};
    sDate.Year    = (uint8_t)(year - 2000); /* RTC_DateTypeDef.Year is 0-99, offset from 2000 */
    sDate.Month   = month;
    sDate.Date    = day;
    sDate.WeekDay = weekday;
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void RtcUtil_GetTodayString(char *outBuf)
{
	RtcUtil_FormatDate(RtcUtil_GetUnixTime(), outBuf);
}
