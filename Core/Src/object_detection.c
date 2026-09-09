#include "object_detection.h"

static volatile bool g_activity = false;
static bool g_previousDetected = false;

void ObjectDetection_NotifyActivity(void)
{
    g_activity = true;
}

bool ObjectDetection_Poll(bool *outDetected)
{
    bool detected = g_activity;
    g_activity = false;

    bool changed = (detected != g_previousDetected);
    g_previousDetected = detected;

    *outDetected = detected;
    return changed;
}
