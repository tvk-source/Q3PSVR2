/* OpenGL/PSVR2 GLimp backend for Quake3e. Replaces ps5/q3/ps5_glimp.c in the
 * VR build. The flat EGL surface exists only to own the GL context; once Hmd2
 * VR mode starts, Quake renders into the PSVR2 side-by-side FBO and does not
 * present its own TV frames (Hmd2 mirroring owns the TV). */
#include "client/client.h"
#include "platform.h"
#include "ps5_q3vr.h"
#include "ps5_q3vr_gl.h"
#include "ps5_q3vr_engine.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string.h>

void IN_Init(void); void IN_Shutdown(void);
int sceUserServiceGetInitialUser(int32_t *user);

static EGLDisplay dpy=EGL_NO_DISPLAY; static EGLSurface surf=EGL_NO_SURFACE; static EGLContext ctx=EGL_NO_CONTEXT;

void GLimp_InitGamma(glconfig_t *config){ config->deviceSupportsGamma=qfalse; }
void GLimp_SetGamma(unsigned char r[256],unsigned char g[256],unsigned char b[256]){(void)r;(void)g;(void)b;}

void GLimp_Init(glconfig_t *config){
    static const EGLint ca[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
    static const EGLint xa[]={EGL_CONTEXT_MAJOR_VERSION_KHR,4,EGL_CONTEXT_MINOR_VERSION_KHR,6,EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,EGL_NONE};
    EGLConfig ec=NULL;EGLint n=0,w=0,h=0;int32_t user=-1;
    dpy=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(dpy==EGL_NO_DISPLAY||!eglInitialize(dpy,NULL,NULL)||!eglBindAPI(EGL_OPENGL_API)||!eglChooseConfig(dpy,ca,&ec,1,&n)||n!=1){
        Com_Error(ERR_FATAL,"PS5 OpenGL: failed to initialise EGL");
        return;
    }
    surf=eglCreateWindowSurface(dpy,ec,(EGLNativeWindowType)0,NULL);ctx=eglCreateContext(dpy,ec,EGL_NO_CONTEXT,xa);
    if(surf==EGL_NO_SURFACE||ctx==EGL_NO_CONTEXT||!eglMakeCurrent(dpy,surf,surf,ctx)){
        Com_Error(ERR_FATAL,"PS5 OpenGL: failed to create/make current EGL context");
        return;
    }
    eglSwapInterval(dpy,1);eglQuerySurface(dpy,surf,EGL_WIDTH,&w);eglQuerySurface(dpy,surf,EGL_HEIGHT,&h);
    Cvar_Get("r_availableModes","1920x1080",CVAR_ROM);
    memset(config,0,sizeof *config);config->vidWidth=w;config->vidHeight=h;config->windowAspect=(float)w/(float)h;config->displayFrequency=60;config->isFullscreen=qtrue;config->colorBits=32;config->depthBits=24;config->stencilBits=8;config->driverType=GLDRV_ICD;config->hardwareType=GLHW_GENERIC;
    IN_Init();HandleEvents();Key_ClearStates();
    sceUserServiceGetInitialUser(&user);
    if(user>=0&&ps5_q3vr_init(user)&&ps5_q3vr_gl_init()&&ps5_q3vr_start(ps5_q3vr_gl_targets())){
        config->stereoEnabled=qtrue;config->vidWidth=ps5_q3vr_gl_eye_size();config->vidHeight=ps5_q3vr_gl_eye_size();config->windowAspect=1.0f;
        say("q3vr: OpenGL PSVR2 path active, fw %d",ps5_q3vr_firmware());
    }else{config->stereoEnabled=qfalse;say("q3vr: flat fallback: %s",ps5_q3vr_status());}
    return;
}
void GLimp_EndFrame(void){ if(PS5Q3VR_IsActive())PS5Q3VR_EndFrame();else eglSwapBuffers(dpy,surf); }
void GLimp_Shutdown(qboolean unloadDLL){(void)unloadDLL;ps5_q3vr_stop();ps5_q3vr_gl_shutdown();IN_Shutdown();if(dpy!=EGL_NO_DISPLAY){eglMakeCurrent(dpy,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(ctx!=EGL_NO_CONTEXT)eglDestroyContext(dpy,ctx);if(surf!=EGL_NO_SURFACE)eglDestroySurface(dpy,surf);eglTerminate(dpy);}dpy=EGL_NO_DISPLAY;surf=EGL_NO_SURFACE;ctx=EGL_NO_CONTEXT;}
void *GL_GetProcAddress(const char *name){ return (void*)eglGetProcAddress(name); }
