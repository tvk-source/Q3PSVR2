#pragma once
#include "ps5_q3vr.h"

#ifdef __cplusplus
extern "C" {
#endif

int ps5_q3vr_gl_init(void);
void ps5_q3vr_gl_shutdown(void);
unsigned ps5_q3vr_gl_fbo(void);
unsigned ps5_q3vr_gl_texture(void);
int ps5_q3vr_gl_eye_size(void);
const ps5_q3vr_targets_t *ps5_q3vr_gl_targets(void);
void ps5_q3vr_gl_fill(int buffer_index);

#ifdef __cplusplus
}
#endif
