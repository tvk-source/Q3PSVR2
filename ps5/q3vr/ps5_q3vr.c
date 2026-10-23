/* Native PS VR2 runtime for Quake3e on PS5.
 *
 * This is a game-oriented extraction of the Hmd2/VrTracker2 path proven by
 * PS5VR. It keeps the multi-firmware anchor tables from the user-supplied
 * PS5VR source and only exposes what a Quake-style game needs: predicted eye
 * poses, Sense-controller poses, 120 Hz reprojection with 60 Hz rendering,
 * recentering and a side-by-side RGB eye surface.
 *
 * GPL-3.0-or-later. The resulting combined build is therefore GPL-3.0-or-later.
 */
#include "ps5_q3vr.h"
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern void say(const char *fmt, ...);

int sceSysmoduleLoadModule(uint32_t id);
const char *sceKernelGetFsSandboxRandomWord(void);
int sceKernelLoadStartModule(const char *path, size_t argc, const void *argv, uint32_t flags, void *opt, int *res);
int sceKernelDlsym(int handle, const char *name, void **addr);
int sceKernelGetModuleInfo(int handle, void *info);
int sceKernelAllocateDirectMemory(int64_t start, int64_t end, size_t len, size_t align, int type, int64_t *offset);
int sceKernelMapDirectMemory(void **addr, size_t len, int prot, int flags, int64_t offset, size_t align);
int sceKernelUsleep(unsigned usec);
uint64_t sceKernelGetProcessTime(void);
int sceVideoOutOpen(int user_id, int bus_type, int index, const void *param);
int sceVideoOutClose(int handle);
void sceVideoOutSetBufferAttribute2(void *attr, uint64_t format, uint32_t tiling, uint32_t width,
                                    uint32_t height, uint64_t option, uint32_t reserved0, uint64_t reserved1);
int sceVideoOutRegisterBuffers2(int handle, int set_index, int start_index, void *buffers, int count,
                                void *attr, int category, void *option);
int sceVideoOutUnregisterBuffers(int handle, int set_index);
int sceVideoOutSubmitFlip(int handle, int index, int flip_mode, int64_t flip_arg);
int sceVideoOutConfigureOutput(int handle, uint64_t mode, const void *options, const void *r0, uint64_t r1);
int scePadOpen(int user, int type, int index, const void *param);
int scePadReadState(int handle, void *data);

typedef struct { uint64_t size, align; } SizeAlign;
#define HMD2_FUNCS(X) \
 X(int,sceHmd2Initialize,(const void *p)) \
 X(int,sceHmd2Open,(int user,int type,int index,const void *p)) \
 X(int,sceHmd2GetDeviceInformation,(void *info)) \
 X(int,sceHmd2GetFieldOfViewWithoutHandle,(void *fov)) \
 X(SizeAlign,sceHmd2ReprojectionQueryBufferSizeAlign,(void)) \
 X(SizeAlign,sceHmd2ReprojectionQueryDisplayBufferSizeAlign,(void)) \
 X(int,sceHmd2ReprojectionInitialize,(const void *p,const void *r)) \
 X(int,sceHmd2ReprojectionEnableVrMode,(uint64_t mode)) \
 X(int,sceHmd2ReprojectionDisableVrMode,(void)) \
 X(int,sceHmd2ReprojectionBeginFrame,(uint64_t frame)) \
 X(int,sceHmd2ReprojectionGetPredictedDisplayTime,(uint64_t frame,uint64_t *time)) \
 X(int,sceHmd2ReprojectionSetParam,(uint32_t buffer,uint64_t frame,const void *p,const void *trace)) \
 X(int,sceHmd2ReprojectionSetRenderConfig,(uint64_t half,uint64_t r0,uint64_t r1)) \
 X(SizeAlign,sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign,(void)) \
 X(SizeAlign,sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign,(void)) \
 X(int,sceHmd2ReprojectionSetMirroringOption,(const void *opt)) \
 X(int,sceHmd2ReprojectionEnableMirroring,(const void *p)) \
 X(int,sceHmd2ReprojectionDisableMirroring,(void)) \
 X(int,sceHmd2ReprojectionTerminate,(void)) \
 X(int,sceHmd2Close,(int h)) \
 X(int,sceHmd2Terminate,(void))
#define HMD2_OPT_FUNCS(X) \
 X(int,sceHmd2SetVibration,(int h,int strength))
#define TRACKER_FUNCS(X) \
 X(int,sceVrTracker2QueryMemory,(const void *p,void *out)) \
 X(int,sceVrTracker2Initialize,(const void *p)) \
 X(int,sceVrTracker2RegisterDevice,(int type,int handle)) \
 X(int,sceVrTracker2GetResult,(const void *p,void *out)) \
 X(int,sceVrTracker2UnregisterDevice,(int handle)) \
 X(int,sceVrTracker2Finalize,(void))
#define DECL(ret,name,args) static ret (*name) args;
HMD2_FUNCS(DECL) HMD2_OPT_FUNCS(DECL) TRACKER_FUNCS(DECL)

typedef struct { const char *name; uint32_t offset; } export_sig_t;
typedef struct { uint32_t va; const char *text; } export_anchor_t;
typedef struct { int fw; const export_sig_t *exports; size_t count; const export_anchor_t *anchor; } export_table_t;
#define COUNT(a) (sizeof(a)/sizeof((a)[0]))
#include "exports/vr_exports.h"

typedef struct { const char *name; void **slot; int optional; } slot_t;
#define SLOT(ret,name,args) {#name,(void**)&name,0},
#define OSLOT(ret,name,args) {#name,(void**)&name,1},
static slot_t hmd_slots[]={ HMD2_FUNCS(SLOT) HMD2_OPT_FUNCS(OSLOT) };
static slot_t tracker_slots[]={ TRACKER_FUNCS(SLOT) };

typedef struct {
    uint64_t size; char name[256];
    struct { uint64_t addr; uint32_t size; int32_t prot; } seg[4];
    uint32_t nseg; uint8_t fingerprint[20];
} module_info_t;

typedef struct { void *layers; uint32_t count,pad; } layer_set_t;
typedef struct { void *data,*metadata,*r0,*r1; } vo_buffer_t;

enum { SYSMODULE_HMD2=0x12a, SYSMODULE_TRACKER2=0x12b };
#define VR_MODE_120HZ 0x2000full
#define VR_QUEUE_FULL ((int)0x8A72001A)
#define VO_BUSY ((int)0x80290009)
#define OVERSCAN 1.15f
#define IPD_METRES 0.064f
#define FRAME_US 16667ull

static struct {
    int loaded,failed,active,frame_open,user,hmd,vh,buf;
    int fw_hmd,fw_tracker;
    float fov[4];
    ps5_q3vr_targets_t targets;
    uint8_t layers[PS5_Q3VR_BUFFERS][0x130] __attribute__((aligned(16)));
    layer_set_t sets[PS5_Q3VR_BUFFERS];
    uint64_t frame,time,next_begin;
    float device_pos[3],device_q[4];
    float origin_pos[3],origin_yaw[4]; int have_origin,tracked_frames;
    int sense[2],sense_registered[2],sense_opened;
    void *mirror_work,*mirror_disp; int mirroring;
    unsigned submits,errors;
    char status[192];
} g={.hmd=-1,.vh=-1,.origin_yaw={0,0,0,1},.sense={-1,-1}};

static void statusf(const char *fmt,...){ va_list ap; va_start(ap,fmt); vsnprintf(g.status,sizeof g.status,fmt,ap); va_end(ap); say("q3vr: %s",g.status); }
const char *ps5_q3vr_status(void){ return g.status; }
int ps5_q3vr_firmware(void){ return g.fw_hmd; }

static int load_module(uint32_t id,const char *name){
    int rc=sceSysmoduleLoadModule(id),res=0; char p[160]; const char *w=sceKernelGetFsSandboxRandomWord();
    snprintf(p,sizeof p,"/%s/common/lib/%s.sprx",w?w:"system",name);
    int h=sceKernelLoadStartModule(p,0,NULL,0,NULL,&res); say("q3vr: module %s sys 0x%08x handle 0x%x",name,rc,h); return h;
}
static uintptr_t module_base(int h,const char *file,module_info_t *mi){
    memset(mi,0,sizeof *mi); mi->size=sizeof *mi;
    if(sceKernelGetModuleInfo(h,mi)!=0||strcmp(mi->name,file)) return 0;
    uintptr_t base=0; for(unsigned i=0;i<mi->nseg&&i<4;i++) if(!base||mi->seg[i].addr<base) base=mi->seg[i].addr; return base;
}
static int anchor_match(const module_info_t *mi,uintptr_t base,const export_anchor_t *a){
    uintptr_t p=base+a->va; size_t n=strlen(a->text)+1;
    for(unsigned i=0;i<mi->nseg&&i<4;i++){ uintptr_t lo=mi->seg[i].addr,hi=lo+mi->seg[i].size;
        if(p>=lo&&p+n<=hi&&(mi->seg[i].prot&1)) return memcmp((void*)p,a->text,n)==0; }
    return 0;
}
static const export_table_t *pick_table(int h,const char *file,const export_table_t *tables,size_t n,uintptr_t *base){
    module_info_t mi; *base=module_base(h,file,&mi); if(!*base) return NULL;
    for(size_t i=0;i<n;i++) if(anchor_match(&mi,*base,tables[i].anchor)) return &tables[i];
    return NULL;
}
static int resolve(int h,const char *file,slot_t *slots,size_t n,const export_table_t *tables,size_t nt,int *fw){
    uintptr_t base=0; const export_table_t *t=NULL; int table_tried=0,ok=1;
    for(size_t i=0;i<n;i++){
        if(sceKernelDlsym(h,slots[i].name,slots[i].slot)==0&&*slots[i].slot) continue;
        if(!table_tried){ t=pick_table(h,file,tables,nt,&base); table_tried=1; if(t){*fw=t->fw; say("q3vr: %s table %d.%02d base %p",file,t->fw/100,t->fw%100,(void*)base);} }
        *slots[i].slot=NULL;
        for(size_t j=0;t&&j<t->count;j++) if(!strcmp(t->exports[j].name,slots[i].name)){*slots[i].slot=(void*)(base+t->exports[j].offset);break;}
        if(!*slots[i].slot&&!slots[i].optional){ say("q3vr: missing %s",slots[i].name); ok=0; }
    }
    return ok?0:-1;
}
static void *dalloc(size_t size,size_t align){
    int64_t off=0; void *p=NULL; size=(size+0xffff)&~(size_t)0xffff;
    if(sceKernelAllocateDirectMemory(0,(int64_t)16<<30,size,align,12,&off)||sceKernelMapDirectMemory(&p,size,0x33,0,off,align)) return NULL;
    memset(p,0,size); return p;
}
static int tracker_pose(uint64_t t,int handle,float p[3],float q[4],float lq[4],float rq[4]){
    uint8_t gp[0x20]={0}; static uint8_t res[0x400]; *(uint32_t*)gp=0x20; *(int32_t*)(gp+4)=handle; *(uint64_t*)(gp+0x10)=t;
    memset(res,0,sizeof res); if(sceVrTracker2GetResult(gp,res)!=0) return 0;
    memcpy(p,res+0x60,12); memcpy(q,res+0x70,16); if(lq)memcpy(lq,res+0xb0,16); if(rq)memcpy(rq,res+0xf0,16); return *(uint32_t*)(res+4)!=0;
}
static void qrotate(const float q[4],const float v[3],float o[3]){
    const float x=q[0],y=q[1],z=q[2],w=q[3]; float tx=2*(y*v[2]-z*v[1]),ty=2*(z*v[0]-x*v[2]),tz=2*(x*v[1]-y*v[0]);
    o[0]=v[0]+w*tx+(y*tz-z*ty); o[1]=v[1]+w*ty+(z*tx-x*tz); o[2]=v[2]+w*tz+(x*ty-y*tx);
}
static float heading(const float q[4]){ const float x=q[0],y=q[1],z=q[2],w=q[3]; float fx=-2*(x*z+y*w),fz=-(1-2*(x*x+y*y)); return atan2f(-fx,-fz); }

int ps5_q3vr_init(int user){
    if(g.loaded) return 1; if(g.failed) return 0; g.failed=1; g.user=user;
    int h=load_module(SYSMODULE_HMD2,"libSceHmd2");
    if(h<0||resolve(h,"libSceHmd2.sprx",hmd_slots,COUNT(hmd_slots),klibSceHmd2Tables,COUNT(klibSceHmd2Tables),&g.fw_hmd)){statusf("Hmd2 unavailable");return 0;}
    h=load_module(SYSMODULE_TRACKER2,"libSceVrTracker2");
    if(h<0||resolve(h,"libSceVrTracker2.sprx",tracker_slots,COUNT(tracker_slots),klibSceVrTracker2Tables,COUNT(klibSceVrTracker2Tables),&g.fw_tracker)){statusf("VrTracker2 unavailable");return 0;}
    uint8_t hp[16]={0}; int rc=sceHmd2Initialize(hp); if(rc){statusf("Hmd2 init 0x%08x",rc);return 0;}
    float f[16]={0}; rc=sceHmd2GetFieldOfViewWithoutHandle(f); memcpy(g.fov,f,sizeof g.fov);
    if(rc||f[0]<=0||f[2]<=0){statusf("no headset/FOV rc 0x%08x",rc);return 0;}
    g.hmd=sceHmd2Open(user,0,0,NULL); if(g.hmd<0){statusf("Hmd2Open 0x%08x",g.hmd);return 0;}
    uint32_t qp[8]={0x20,0},qo[8]={0}; if(sceVrTracker2QueryMemory(qp,qo)){statusf("tracker memory query failed");return 0;}
    void *tw=dalloc(qo[1],qo[2]>0x10000?qo[2]:0x10000); if(!tw){statusf("tracker direct memory failed");return 0;}
    uint8_t ip[0x48]={0}; *(uint32_t*)ip=0x48; *(int32_t*)(ip+8)=700; *(int32_t*)(ip+0xc)=700; *(uint64_t*)(ip+0x10)=0x1f00; *(uint64_t*)(ip+0x18)=0x1f00; *(void**)(ip+0x20)=tw; *(uint32_t*)(ip+0x28)=qo[1]; *(uint32_t*)(ip+0x2c)=qo[2];
    rc=sceVrTracker2Initialize(ip); int rc2=!rc?sceVrTracker2RegisterDevice(0,g.hmd):-1; if(rc||rc2){statusf("tracker init %08x reg %08x",rc,rc2);return 0;}
    SizeAlign wb=sceHmd2ReprojectionQueryBufferSizeAlign(),db=sceHmd2ReprojectionQueryDisplayBufferSizeAlign(); void *work=dalloc(wb.size,0x10000),*disp=dalloc(db.size,0x10000); if(!work||!disp){statusf("reprojection memory failed");return 0;}
    uint8_t rp[72]={0}; *(void**)rp=work; *(void**)(rp+8)=disp; *(uint32_t*)(rp+0x10)=256; *(int32_t*)(rp+0x14)=0x1f00; *(uint32_t*)(rp+0x18)=1;
    rc=sceHmd2ReprojectionInitialize(rp,NULL); if(rc){statusf("reprojection init %08x",rc);return 0;}
    g.loaded=1; g.failed=0; statusf("ready fw Hmd2 %d.%02d Tracker %d.%02d",g.fw_hmd/100,g.fw_hmd%100,g.fw_tracker/100,g.fw_tracker%100); return 1;
}

static int tsharp(uint32_t out[8],uint64_t addr,uint32_t w,uint32_t h,uint32_t pitch,int tiled){
    enum{FMT=56,SX=4,SY=5,SZ=6,SW=7,IMG2D=9,SW64=27}; if(!addr||(addr&(tiled?0xffffu:0xffu))||!w||!h||pitch<w)return 0;
    memset(out,0,32); uint32_t wm1=w-1,pm1=pitch-1; out[0]=(uint32_t)(addr>>8); out[1]=(uint32_t)(addr>>40)|(FMT<<20)|((wm1&3u)<<30); out[2]=((wm1>>2)&0xfffu)|((h-1)<<14); out[3]=SX|(SY<<3)|(SZ<<6)|(SW<<9)|(IMG2D<<28); if(tiled)out[3]|=SW64<<20; else if(pitch!=w)out[4]=(pm1&0x1fffu)|(((pm1>>13)&1u)<<13); out[5]=7u<<20; return 1;
}
int ps5_q3vr_start(const ps5_q3vr_targets_t *t){
    if(g.active)return 1; if(!g.loaded||!t)return 0; g.targets=*t; uint32_t ts[PS5_Q3VR_BUFFERS][8];
    for(int b=0;b<PS5_Q3VR_BUFFERS;b++)if(!tsharp(ts[b],t->address[b],t->width,t->height,t->pitch,t->tiled)){statusf("bad eye buffer %d",b);return 0;}
    int rc=sceHmd2ReprojectionEnableVrMode(VR_MODE_120HZ); g.vh=sceVideoOutOpen(0xff,32,0,NULL); if(g.vh<0){sceHmd2ReprojectionDisableVrMode();statusf("VR port failed %08x",g.vh);return 0;}
    vo_buffer_t bufs[PS5_Q3VR_BUFFERS]; memset(bufs,0,sizeof bufs); memset(g.layers,0,sizeof g.layers);
    for(int b=0;b<PS5_Q3VR_BUFFERS;b++){*(uint32_t*)g.layers[b]=0x10;memcpy(g.layers[b]+4,ts[b],32);memcpy(g.layers[b]+0x24,ts[b],32);g.sets[b].layers=g.layers[b];g.sets[b].count=1;bufs[b].data=&g.sets[b];}
    uint8_t attr[80]={0}; sceVideoOutSetBufferAttribute2(attr,0x8000000000000000ull,0,t->width,t->height,0,0,0); rc=sceVideoOutRegisterBuffers2(g.vh,0,0,bufs,PS5_Q3VR_BUFFERS,attr,0x10,NULL);
    if(rc){sceVideoOutClose(g.vh);g.vh=-1;sceHmd2ReprojectionDisableVrMode();statusf("RegisterBuffers2 %08x",rc);return 0;}
    sceVideoOutConfigureOutput(g.vh,VR_MODE_120HZ,NULL,NULL,0); sceHmd2ReprojectionSetRenderConfig(1,0,0);
    if(!g.mirror_work){SizeAlign w=sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign(),d=sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();g.mirror_work=dalloc(w.size,w.align>0x10000?w.align:0x10000);g.mirror_disp=dalloc(d.size,d.align>0x10000?d.align:0x10000);} if(g.mirror_work&&g.mirror_disp){uint32_t opt[8]={1};void *mp[2]={g.mirror_work,g.mirror_disp};sceHmd2ReprojectionSetMirroringOption(opt);g.mirroring=sceHmd2ReprojectionEnableMirroring(mp)==0;}
    g.buf=0;g.frame_open=0;g.next_begin=0;g.active=1;statusf("VR active %dx%d",t->width,t->height);return 1;
}
void ps5_q3vr_stop(void){
    if(!g.active)return; g.active=0;g.frame_open=0;if(g.mirroring)sceHmd2ReprojectionDisableMirroring();g.mirroring=0;sceHmd2ReprojectionDisableVrMode();int rc=0;for(int i=0;i<50;i++){rc=sceVideoOutUnregisterBuffers(g.vh,0);if(rc!=VO_BUSY)break;sceKernelUsleep(10000);}sceVideoOutClose(g.vh);g.vh=-1;statusf("VR stopped after %u submits",g.submits);
}
int ps5_q3vr_active(void){return g.active;}
void ps5_q3vr_wait_slot(void){if(!g.active)return;uint64_t now=sceKernelGetProcessTime();if(g.next_begin>now&&g.next_begin-now<=FRAME_US){sceKernelUsleep((unsigned)(g.next_begin-now));now=g.next_begin;}g.next_begin=g.next_begin&&now<g.next_begin+FRAME_US?g.next_begin+FRAME_US:now+FRAME_US;}
void ps5_q3vr_recenter(void){float p[3],q[4],l[4],r[4];if(!g.loaded||!tracker_pose(0,g.hmd,p,q,l,r))return;float a=(heading(l)+heading(r))*0.5f;g.origin_yaw[0]=g.origin_yaw[2]=0;g.origin_yaw[1]=sinf(a*.5f);g.origin_yaw[3]=cosf(a*.5f);memcpy(g.origin_pos,p,12);g.have_origin=1;say("q3vr: recentered heading %.1f",a*57.29578f);}
void ps5_q3vr_origin(float p[3],float q[4]){memcpy(p,g.origin_pos,12);memcpy(q,g.origin_yaw,16);}
int ps5_q3vr_begin(ps5_q3vr_frame_t *f){
    if(!g.active||!f)return 0;if(!g.frame_open){uint64_t fr=g.frame+1;int rc=sceHmd2ReprojectionBeginFrame(fr);for(int i=0;rc==VR_QUEUE_FULL&&i<40;i++){sceKernelUsleep(1000);rc=sceHmd2ReprojectionBeginFrame(fr);}if(rc)return 0;g.frame=fr;g.time=0;sceHmd2ReprojectionGetPredictedDisplayTime(fr,&g.time);g.frame_open=1;}
    float lq[4]={0,0,0,1},rq[4]={0,0,0,1};g.device_q[0]=g.device_q[1]=g.device_q[2]=0;g.device_q[3]=1;memset(g.device_pos,0,12);f->tracked=tracker_pose(g.time,g.hmd,g.device_pos,g.device_q,lq,rq);g.tracked_frames=f->tracked?g.tracked_frames+1:0;if(!g.have_origin&&g.tracked_frames==180)ps5_q3vr_recenter();
    for(int e=0;e<2;e++){float side[3]={e?IPD_METRES*.5f:-IPD_METRES*.5f,0,0},off[3];qrotate(g.device_q,side,off);for(int k=0;k<3;k++)f->eye[e].pos[k]=g.device_pos[k]+off[k];memcpy(f->eye[e].quat,e?rq:lq,16);}float tl[4]={g.fov[0]*OVERSCAN,g.fov[1]*OVERSCAN,g.fov[2]*OVERSCAN,g.fov[3]*OVERSCAN},tr[4]={tl[1],tl[0],tl[2],tl[3]};memcpy(f->eye[0].tan,tl,16);memcpy(f->eye[1].tan,tr,16);f->buffer=g.buf;return 1;
}
void ps5_q3vr_submit(void){
    if(!g.frame_open)return;g.frame_open=0;static uint8_t lp[0x148] __attribute__((aligned(16)));struct{void*layers;int32_t count,pad;void*ext;}p={lp,1,0,NULL};uint32_t tw=g.targets.width,th=g.targets.height,ew=tw/2;float tl[4]={g.fov[0]*OVERSCAN,g.fov[1]*OVERSCAN,g.fov[2]*OVERSCAN,g.fov[3]*OVERSCAN},tr[4]={tl[1],tl[0],tl[2],tl[3]};memset(lp,0,sizeof lp);uint32_t *u=(uint32_t*)lp;u[0]=0x10;u[2]=tw;u[3]=th;u[4]=tw;u[5]=th;u[6]=0x92;u[7]=0x00fff000;u[8]=0x05500000;u[0x28/4]=ew;u[0x2c/4]=th;u[0x38/4]=ew;u[0x3c/4]=th;u[0x40/4]=ew;memcpy(lp+0x48,tl,16);memcpy(lp+0x58,tr,16);memcpy(lp+0x68,g.device_pos,12);memcpy(lp+0x74,g.device_q,16);*(uint64_t*)(lp+0x88)=g.time;*(uint64_t*)(lp+0x90)=g.frame;u[0x98/4]=100;u[0xa0/4]=ew;u[0xa4/4]=th;u[0xa8/4]=ew;u[0xac/4]=th;int r1=sceHmd2ReprojectionSetParam(g.buf,g.frame,&p,NULL),r2=sceVideoOutSubmitFlip(g.vh,g.buf,2,(int64_t)g.frame);if((r1||r2)&&g.errors++<20)say("q3vr: submit %08x flip %08x",r1,r2);g.submits++;g.buf=(g.buf+1)%PS5_Q3VR_BUFFERS;
}
static void open_sense(void){if(g.sense_opened)return;g.sense_opened=1;for(int s=0;s<2;s++){g.sense[s]=scePadOpen(g.user,s?4:3,0,NULL);int rc=g.sense[s]>=0?sceVrTracker2RegisterDevice(s?1:2,g.sense[s]):-1;g.sense_registered[s]=rc==0;say("q3vr: sense %d handle %x reg %08x",s,g.sense[s],rc);}}
int ps5_q3vr_hand(int side,ps5_q3vr_hand_t *o){
    if(!o||side<0||side>1||!g.active)return 0;memset(o,0,sizeof *o);o->quat[3]=1;open_sense();if(!g.sense_registered[side])return 0;uint8_t st[0x100]={0};o->connected=scePadReadState(g.sense[side],st)==0;if(o->connected){memcpy(&o->buttons,st,4);int k=side&&!(st[6]==0&&st[7]==0)?6:4;o->stick[0]=(st[k]-127.5f)/127.5f;o->stick[1]=(st[k+1]-127.5f)/127.5f;o->trigger=st[side?9:8]/255.0f;}float p[3],q[4];o->tracked=tracker_pose(g.time,g.sense[side],p,q,NULL,NULL);if(o->tracked){memcpy(o->pos,p,12);memcpy(o->quat,q,16);}return 1;
}
void ps5_q3vr_vibrate(float strength){static int last=-1;if(!sceHmd2SetVibration||g.hmd<0)return;if(strength<0)strength=0;if(strength>1)strength=1;int n=(int)lroundf(strength*25);if(n!=last){sceHmd2SetVibration(g.hmd,n);last=n;}}
