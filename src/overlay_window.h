#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H

#include <stddef.h>

typedef struct {
    int mon_id;
    
    float x; // normalized 0..1 inside monitor
    float y; // normalized 0..1 inside monitor
} OverlayPoint;


typedef struct {
    OverlayPoint points[16];
    size_t count;
    
    int cancelled;
} OverlayResult;

/*
 * Start point selection mode.
 *
 * max_points:
 *     number of points required before automatically finishing.
 *
 * Returns:
 *     0 success
 *    -1 cancelled
 */
int overlay_capture(
    size_t max_points,
    OverlayResult *result
);

#endif // OVERLAY_WINDOW_H
