#include "q3psvr2_sense_bridge.h"
#include <string.h>
/* NOT thread-safe: call publish and read on one game/main thread, or wrap
 * externally with native locks. No SDK imports until a 7.51 adapter is proven. */
static q3psvr2_sense_snapshot state;

void q3psvr2_sense_reset(void) { memset(&state, 0, sizeof(state)); }

void q3psvr2_sense_publish(int hand, const q3psvr2_sense_sample *sample) {
    if (hand < 0 || hand > 1 || !sample) return;
    state.hand[hand] = *sample;
    state.active = state.hand[0].pose_valid || state.hand[1].pose_valid;
}

int q3psvr2_sense_get_pose(int hand, uint64_t now_us, uint64_t max_age_us,
                           q3psvr2_sense_sample *out) {
    const q3psvr2_sense_sample *p;
    if (!out) return 0;
    memset(out, 0, sizeof(*out));
    if (hand < 0 || hand > 1 || max_age_us == 0) return 0;
    p=&state.hand[hand];
    if (!p->pose_valid || !p->timestamp_us || now_us < p->timestamp_us ||
        now_us-p->timestamp_us > max_age_us) return 0;
    *out=*p;
    return 1;
}

void q3psvr2_sense_get_snapshot(q3psvr2_sense_snapshot *out) {
    if (out) *out = state;
}
