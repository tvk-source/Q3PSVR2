#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PS5_Q3VR_BUFFERS 3

typedef struct ps5_q3vr_eye_s {
    float pos[3];      /* tracker-space metres */
    float quat[4];     /* x,y,z,w */
    float tan[4];      /* left,right,up,down tangents */
} ps5_q3vr_eye_t;

typedef struct ps5_q3vr_frame_s {
    ps5_q3vr_eye_t eye[2];
    int buffer;
    int tracked;
} ps5_q3vr_frame_t;

typedef struct ps5_q3vr_targets_s {
    uint64_t address[PS5_Q3VR_BUFFERS];
    int width, height, pitch;
    int tiled;
} ps5_q3vr_targets_t;

typedef struct ps5_q3vr_hand_s {
    int tracked, connected;
    float pos[3], quat[4];
    uint32_t buttons;
    float stick[2], trigger;
} ps5_q3vr_hand_t;

int  ps5_q3vr_init(int user_id);
int  ps5_q3vr_start(const ps5_q3vr_targets_t *targets);
void ps5_q3vr_stop(void);
int  ps5_q3vr_active(void);
void ps5_q3vr_wait_slot(void);
int  ps5_q3vr_begin(ps5_q3vr_frame_t *frame);
void ps5_q3vr_submit(void);
void ps5_q3vr_recenter(void);
void ps5_q3vr_origin(float pos[3], float yaw_q[4]);
int  ps5_q3vr_hand(int side, ps5_q3vr_hand_t *out);
void ps5_q3vr_vibrate(float strength);
const char *ps5_q3vr_status(void);
int  ps5_q3vr_firmware(void); /* matched Hmd2 table, e.g. 751/761/1360 */

#ifdef __cplusplus
}
#endif
