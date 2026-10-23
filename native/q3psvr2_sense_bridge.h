#ifndef Q3PSVR2_SENSE_BRIDGE_H
#define Q3PSVR2_SENSE_BRIDGE_H
/* License: GPL-2.0-or-later, see repository COPYING.  This is an
 * engine-facing boundary, NOT a 7.51 native controller driver.
 * Native Tracker2 platform adapter must publish verified poses into it. */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { Q3PSVR2_SENSE_LEFT = 0, Q3PSVR2_SENSE_RIGHT = 1 };

typedef struct {
    uint64_t timestamp_us;
    float position_m[3];      /* platform-space meters; conversion to Quake world required */
    float orientation_xyzw[4];/* normalized quaternion, right-hand conversion required */
    float trigger;            /* 0..1 if available */
    float grip;               /* 0..1 if available */
    float stick_x, stick_y;   /* -1..1 if available */
    uint32_t buttons;        /* platform adapter documents its button map */
    uint8_t pose_valid;
    uint8_t input_valid;
} q3psvr2_sense_sample;

typedef struct {
    q3psvr2_sense_sample hand[2];
    uint8_t active;
} q3psvr2_sense_snapshot;

void q3psvr2_sense_reset(void);
void q3psvr2_sense_publish(int hand, const q3psvr2_sense_sample *sample);
/* Returns 1 only for valid, fresh pose. Returns 0 (caller uses DualSense/game aim) otherwise. */
int q3psvr2_sense_get_pose(int hand, uint64_t now_us, uint64_t max_age_us,
                           q3psvr2_sense_sample *out);
void q3psvr2_sense_get_snapshot(q3psvr2_sense_snapshot *out);

#ifdef __cplusplus
}
#endif
#endif
