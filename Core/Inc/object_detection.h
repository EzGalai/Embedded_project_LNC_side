#ifndef INC_OBJECT_DETECTION_H_
#define INC_OBJECT_DETECTION_H_
#include <stdbool.h>

/**
 * @brief Called from the IR receiver's EXTI callback (ISR context) — marks
 * that activity was seen since the last poll. Safe to call many times
 * rapidly during one remote-control burst; idempotent.
 */
void ObjectDetection_NotifyActivity(void);

/**
 * @brief Called every 200ms by vObjectDetectionTask. Checks whether activity
 * was seen since the last call, clears the flag, and compares against the
 * previous poll to determine whether the detection state actually changed.
 * @param outDetected Set to the current detection state (true = detected).
 * @return true if the state changed since the previous poll.
 */
bool ObjectDetection_Poll(bool *outDetected);

#endif
