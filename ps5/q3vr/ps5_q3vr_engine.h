#pragma once

/* Narrow bridge used by the Quake3e renderer. It intentionally knows nothing
 * about Quake renderer structs, so the engine patch stays small. */
#ifdef __cplusplus
extern "C" {
#endif

int  PS5Q3VR_IsActive(void);
int  PS5Q3VR_BeginEye(int stereo_frame); /* 1 left, 2 right in Quake3e */
void PS5Q3VR_EndFrame(void);
void PS5Q3VR_Recenter(void);
void PS5Q3VR_SetBackendEye(int eye);
int PS5Q3VR_Get2DViewport(int *x,int *y,int *w,int *h);

/* Supplies the current eye transform. base_axis is Quake's player/body view.
 * out_origin is Quake units. out_axis is a Quake 3x3 view axis.
 * out_proj is an OpenGL column-major projection matrix. */
int PS5Q3VR_GetView(int eye, const float base_origin[3], const float base_axis[3][3],
                    float out_origin[3], float out_axis[3][3], float out_proj[16],
                    int *viewport_x, int *viewport_y, int *viewport_w, int *viewport_h);

#ifdef __cplusplus
}
#endif
