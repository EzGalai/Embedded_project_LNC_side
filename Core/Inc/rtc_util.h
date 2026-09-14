#ifndef INC_RTC_UTIL_H_
#define INC_RTC_UTIL_H_
#include <stdint.h>

/**
 * @brief Converts a Unix timestamp (seconds since 1970-01-01 UTC) into
 * calendar date/time and writes it directly to the STM32's internal RTC.
 * Called by CommRx_HandleSetRtc so Log's date-based filenames and all
 * reported timestamps stay synchronized with Central Computer's time sync.
 * @param unixTime Seconds since the Unix epoch.
 */
void RtcUtil_SetFromUnixTime(uint32_t unixTime);

/**
 * @brief Formats the RTC's current date as "YYYY-MM-DD".
 * @param outBuf Destination buffer, must be at least 11 bytes (10 chars + null).
 */
void RtcUtil_GetTodayString(char *outBuf);


/**
 * @brief Reads the STM32's internal RTC and converts its current calendar
 * date/time back into a Unix timestamp (seconds since 1970-01-01 UTC) — the
 * inverse of RtcUtil_SetFromUnixTime. This is the single source of truth
 * for timestamps used in measurement/event records and GET_TIME_REQ replies.
 * @return Current time as a Unix timestamp.
 */
uint32_t RtcUtil_GetUnixTime(void);


#endif
