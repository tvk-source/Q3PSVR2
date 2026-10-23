/* PS5 OpenGL -> Hmd2 eye-buffer bridge.
 *
 * Quake renders both eyes into one RGBA8 OpenGL texture, left/right side by
 * side. A compute pass copies that texture into persistently mapped GPU memory
 * in the 64 KiB tiled layout Hmd2 reads. This follows the hardware-verified
 * approach used by PS5 VR2 homebrew rather than assuming a Vulkan image can be
 * handed to Hmd2 directly.
 */
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ps5_q3vr_gl.h"

extern void say(const char *fmt, ...);

static int s_eye = 2048, s_w, s_h;
static GLuint s_tex, s_fbo, s_depth, s_program, s_ssbo;
static size_t s_stride, s_align_offset;
static uint8_t *s_mapped;
static ps5_q3vr_targets_t s_targets;

/* CPU copy of the compute shader's tile-address function. */
static uint32_t q3vr_tiled_offset_cpu(uint32_t x,uint32_t y){
    uint32_t o =
        ((y << 4) & 0x70u) ^
        ((y << 5) & 0xf00u) ^
        ((y << 9) & 0x1000u) ^
        ((y << 8) & 0x4000u) ^
        ((x << 2) & 0xcu) ^
        ((x << 5) & 0x380u) ^
        ((x << 4) & 0x400u) ^
        ((x << 6) & 0x800u) ^
        ((x << 9) & 0xa000u);

    uint32_t bpr=((uint32_t)s_w+127u)>>7;
    return (((y>>7)*bpr+(x>>7))<<16)+o;
}

/* Q3PSVR2_DIAG_PPM_FUNCTION_GUARD_V1 */
#if defined(Q3PSVR2_DEBUG_PPM_CAPTURE) && (Q3PSVR2_DEBUG_PPM_CAPTURE)
static void q3vr_capture_ppm(int b){
    static int captured;
    const int step=2;

    if(captured || !s_fbo || !s_mapped ||
       b<0 || b>=PS5_Q3VR_BUFFERS)
        return;

    captured=1;

    const int ow=s_w/step;
    const int oh=s_h/step;
    const size_t bytes=(size_t)s_w*(size_t)s_h*4u;

    uint8_t *linear=(uint8_t*)malloc(bytes);

    if(linear){
        GLint oldReadFbo=0;
        GLint oldReadBuffer=0;

        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&oldReadFbo);
        glGetIntegerv(GL_READ_BUFFER,&oldReadBuffer);

        glBindFramebuffer(GL_READ_FRAMEBUFFER,s_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,s_w,s_h,GL_RGBA,GL_UNSIGNED_BYTE,linear);

        GLenum err=glGetError();

        FILE *f=NULL;
        if(err==GL_NO_ERROR)
            f=fopen("/download0/q3vr-fbo.ppm","wb");

        if(f){
            fprintf(f,"P6\n%d %d\n255\n",ow,oh);

            /*
             * glReadPixels is bottom-up. Write top-down so the file is
             * directly viewable.
             */
            for(int y=0;y<s_h;y+=step){
                int sy=s_h-1-y;

                for(int x=0;x<s_w;x+=step){
                    const uint8_t *px=
                        linear+(((size_t)sy*s_w+x)*4u);
                    fwrite(px,1,3,f);
                }
            }

            fclose(f);
            say("q3vr: captured /download0/q3vr-fbo.ppm");
        }else{
            say("q3vr: FBO capture failed GL 0x%x",(unsigned)err);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER,(GLuint)oldReadFbo);
        glReadBuffer((GLenum)oldReadBuffer);
        free(linear);
    }

    /*
     * Now decode exactly the tiled memory Hmd2 receives.
     */
    const uint8_t *tp=
        s_mapped+s_align_offset+(size_t)b*s_stride;

    FILE *f=fopen("/download0/q3vr-tiled.ppm","wb");

    if(f){
        fprintf(f,"P6\n%d %d\n255\n",ow,oh);

        for(int y=0;y<s_h;y+=step){
            for(int x=0;x<s_w;x+=step){
                const uint8_t *px=
                    tp+q3vr_tiled_offset_cpu(
                        (uint32_t)x,(uint32_t)y);
                fwrite(px,1,3,f);
            }
        }

        fclose(f);
        say("q3vr: captured /download0/q3vr-tiled.ppm");
    }
}
#endif /* Q3PSVR2_DEBUG_PPM_CAPTURE */

static const char *kCopyCS =
"#version 460 core\n"
"layout(local_size_x=8,local_size_y=8) in;\n"
"layout(binding=0) uniform sampler2D srcTex;\n"
"layout(std430,binding=1) writeonly buffer Dst { uint px[]; };\n"
"uniform int W,H;\n"
"uint off(uint x,uint y){\n"
" uint o=((y<<4)&0x70u)^((y<<5)&0xf00u)^((y<<9)&0x1000u)^((y<<8)&0x4000u)^"
"((x<<2)&0xcu)^((x<<5)&0x380u)^((x<<4)&0x400u)^((x<<6)&0x800u)^((x<<9)&0xa000u);\n"
" uint bpr=(uint(W)+127u)>>7; return ((((y>>7)*bpr)+(x>>7))<<16)+o; }\n"
"void main(){ ivec2 p=ivec2(gl_GlobalInvocationID.xy); if(p.x>=W||p.y>=H)return;"
" px[off(uint(p.x),uint(p.y))>>2]=packUnorm4x8(texelFetch(srcTex,ivec2(p.x,H-1-p.y),0)); }\n";

static GLuint compile_compute(void) {
    GLuint sh=glCreateShader(GL_COMPUTE_SHADER), p;
    GLint ok=0; char log[1024]; GLsizei n=0;
    glShaderSource(sh,1,&kCopyCS,NULL); glCompileShader(sh);
    glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){ glGetShaderInfoLog(sh,sizeof log,&n,log); say("q3vr: compute compile: %s",log); glDeleteShader(sh); return 0; }
    p=glCreateProgram(); glAttachShader(p,sh); glLinkProgram(p); glDeleteShader(sh);
    glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ glGetProgramInfoLog(p,sizeof log,&n,log); say("q3vr: compute link: %s",log); glDeleteProgram(p); return 0; }
    return p;
}

static int make_targets(int eye) {
    const GLbitfield flags=GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT;
    size_t total; uintptr_t base;
    s_eye=eye; s_w=eye*2; s_h=eye;
    glCreateTextures(GL_TEXTURE_2D,1,&s_tex);
    glTextureStorage2D(s_tex,1,GL_RGBA8,s_w,s_h);
    glCreateFramebuffers(1,&s_fbo);
    glNamedFramebufferTexture(s_fbo,GL_COLOR_ATTACHMENT0,s_tex,0);
    glCreateRenderbuffers(1,&s_depth);
    glNamedRenderbufferStorage(s_depth,GL_DEPTH24_STENCIL8,s_w,s_h);
    glNamedFramebufferRenderbuffer(s_fbo,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,s_depth);
    if(glCheckNamedFramebufferStatus(s_fbo,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) return 0;
    s_stride=(size_t)((s_w+127)/128)*((s_h+127)/128)*0x10000u;
    total=s_stride*PS5_Q3VR_BUFFERS+0x10000u;
    glCreateBuffers(1,&s_ssbo); glNamedBufferStorage(s_ssbo,total,NULL,flags);
    s_mapped=(uint8_t*)glMapNamedBufferRange(s_ssbo,0,total,flags);
    if(!s_mapped) return 0;
    base=((uintptr_t)s_mapped+0xffffu)&~(uintptr_t)0xffffu;
    s_align_offset=(size_t)(base-(uintptr_t)s_mapped);
    memset(&s_targets,0,sizeof s_targets);
    for(int i=0;i<PS5_Q3VR_BUFFERS;i++) s_targets.address[i]=(uint64_t)(base+(uintptr_t)i*s_stride);
    s_targets.width=s_w; s_targets.height=s_h; s_targets.pitch=s_w; s_targets.tiled=1;
    say("q3vr: eye buffers %dx%d at 0x%llx",s_w,s_h,(unsigned long long)s_targets.address[0]);
    return 1;
}

int ps5_q3vr_gl_init(void) {
    GLint maxtex=0, maxrb=0;
    int eye=2048;

    if(s_fbo) return 1;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxtex);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&maxrb);

    say("q3vr: GL limits texture=%d renderbuffer=%d",maxtex,maxrb);

    /*
     * Our current bridge uses one side-by-side texture.
     * A 2048px eye therefore requires a 4096px-wide texture.
     * Stay inside the limits reported by the compatibility context for
     * this diagnostic build.
     */
    if(maxtex < eye*2 || maxrb < eye*2) {
        eye=1024;
        say("q3vr: using diagnostic %dx%d eyes, SBS %dx%d",
            eye,eye,eye*2,eye);
    }

    s_program=compile_compute();
    if(!s_program) return 0;

    if(make_targets(eye)) return 1;

    ps5_q3vr_gl_shutdown();
    s_program=compile_compute();

    return s_program && make_targets(1024);
}
void ps5_q3vr_gl_shutdown(void){
    if(s_ssbo) glDeleteBuffers(1,&s_ssbo); if(s_depth) glDeleteRenderbuffers(1,&s_depth); if(s_fbo) glDeleteFramebuffers(1,&s_fbo);
    if(s_tex) glDeleteTextures(1,&s_tex); if(s_program) glDeleteProgram(s_program);
    s_ssbo=s_depth=s_fbo=s_tex=s_program=0; s_mapped=NULL; memset(&s_targets,0,sizeof s_targets);
}
unsigned ps5_q3vr_gl_fbo(void){ return s_fbo; }
unsigned ps5_q3vr_gl_texture(void){ return s_tex; }
int ps5_q3vr_gl_eye_size(void){ return s_eye; }
const ps5_q3vr_targets_t *ps5_q3vr_gl_targets(void){ return &s_targets; }
void ps5_q3vr_gl_fill(int b){
    if(b<0||b>=PS5_Q3VR_BUFFERS||!s_program) return;

/* Save Quake's OpenGL state before the VR compute pass. */
GLint oldProgram = 0;
GLint oldActiveTexture = 0;
GLint oldTexture0 = 0;

glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);

glActiveTexture(GL_TEXTURE0);
glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture0);
glActiveTexture((GLenum)oldActiveTexture);

/* Run the VR framebuffer copy. */
glUseProgram(s_program);
glBindTextureUnit(0, s_tex);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER,1,s_ssbo,s_align_offset+(size_t)b*s_stride,s_stride);
    glUniform1i(glGetUniformLocation(s_program,"W"),s_w); glUniform1i(glGetUniformLocation(s_program,"H"),s_h);
    glDispatchCompute((GLuint)((s_w+7)/8),(GLuint)((s_h+7)/8),1);
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
    glFinish();
/* Restore Quake's original texture and shader program. */
glBindTextureUnit(0, (GLuint)oldTexture0);
glUseProgram((GLuint)oldProgram);
    /*
     * Capture one settled frame after about five seconds at 60 Hz.
     * This tells us whether corruption exists before or after the
     * OpenGL -> tiled-Hmd2 copy.
     */
    {
        static unsigned captureFrame;
        captureFrame++;
        if(captureFrame==1200)
            /* Q3PSVR2_DIAG_PPM_CALL_GUARD_V1 */
            do {
#if defined(Q3PSVR2_DEBUG_PPM_CAPTURE) && (Q3PSVR2_DEBUG_PPM_CAPTURE)
                q3vr_capture_ppm(b);
#endif /* Q3PSVR2_DEBUG_PPM_CAPTURE */
            } while (0);
    }
}
