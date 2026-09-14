#ifndef INC_RTC_UTIL_H_
#define INC_RTC_UTIL_H_
#include <stdint.h>

/**
 * @brief Converts a Unix timestamp (seconds since 1970-01-01 UTC) into
 * calendar date/time and writes it directly to the STM32's internal RTC.
 * Called by CommRx_HandleSetRtc so Log's date-based filenames stay
 * synchronized with Central Computer's time sync — independent of
 * g_lncClock, which is used only for the wire protocol's TIMESTAMP fields
 * and never advances on its own.
 * @param unixTime Seconds since the Unix epoch.
 */
void RtcUtil_SetFromUnixTime(uint32_t unixTime);

/**
 * @brief Formats the RTC's current date as "YYYY-MM-DD".
 * @param outBuf Destination buffer, must be at least 11 bytes (10 chars + null).
 */
void RtcUtil_GetTodayString(char *outBuf);

#endif
