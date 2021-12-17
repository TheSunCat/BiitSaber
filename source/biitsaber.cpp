#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "Util.h"
#include "Wiimote.h"

// Resources
#include "Cube_tpl.h"
#include "Cube.h"

#include "Saber_tpl.h"
#include "Saber.h"

#define DEFAULT_FIFO_SIZE    (256*1024)

// two fb's for double buffering
static void *frameBuffer[2] = { NULL, NULL};
static GXRModeObj *rmode = NULL;

GXTexObj cubeTexture, saberTexture;

void *redCube, *blueCube;
void *redSaber, *blueSaber;

u32 cubeDispListSize;
void buildLists();
u32 makeCube(void** theCube, guVector color);

static GXColor lightColor[] = {
    {0x80,0x80,0x80,0xFF}, // Light color
    {0x80,0x80,0x80,0xFF}, // Ambient color
    {0x80,0x80,0x80,0xFF}  // Mat color
};
void setLight(Mtx mtx);

void draw(Mtx& model, Mtx& view, void** dispList, u32 dispListSize)
{
    Mtx modelView;

    guMtxConcat(model, view, modelView);
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);
    GX_CallDispList(*dispList, dispListSize);
}

void draw(guVector pos, Mtx& view, void** dispList, u32 dispListSize)
{
    Mtx model;

    guMtxIdentity(model);
    guMtxTransApply(model,model, pos.x, pos.y, pos.z);

    draw(model, view, dispList, dispListSize);
}

void draw(guVector pos, guVector scale, Mtx& view, void** dispList, u32 dispListSize)
{
    Mtx model;

    guMtxIdentity(model);
    guMtxScaleApply(model,model, scale.x, scale.y, scale.z);
    guMtxTransApply(model,model, pos.x, pos.y, pos.z);

    draw(model, view, dispList, dispListSize);
}

void draw(guVector pos, guVector scale, guVector rotation, Mtx& view, void** dispList, u32 dispListSize)
{
    Mtx model;
    guMtxIdentity(model);
    guMtxScaleApply(model,model, scale.x, scale.y, scale.z);

    Mtx rot;
    guMtxRotDeg(rot, 'z', rotation.z);
    guMtxConcat(rot, model, model);
    guMtxRotDeg(rot, 'y', rotation.y);
    guMtxConcat(rot, model, model);
    guMtxRotDeg(rot, 'x', rotation.x);
    guMtxConcat(rot, model, model);

    guMtxTransApply(model,model, pos.x, pos.y, pos.z);

    // TODO use quaternions?

    draw(model, view, dispList, dispListSize);
}

int main(int argc, char **argv) {
    f32 yscale;
    u32 xfbHeight;
    u32 fb = 0;
    bool first_frame = true;
    Mtx view; // view and perspective matrices
    Mtx44 perspective;
    void *gpfifo = NULL;
    GXColor background = {0x00, 0x00, 0x00, 0xFF};
    guVector cam = {0.0F, 0.0F, 0.0F},
              up = {0.0F, 1.0F, 0.0F},
            look = {0.0F, 0.0F, -1.0F};

    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    // allocate the fifo buffer
    gpfifo = memalign(32,DEFAULT_FIFO_SIZE);
    memset(gpfifo,0,DEFAULT_FIFO_SIZE);

    // allocate 2 framebuffers for double buffering
    frameBuffer[0] = SYS_AllocateFramebuffer(rmode);
    frameBuffer[1] = SYS_AllocateFramebuffer(rmode);

    // Initialize the console, required for printf
    //console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    // configure video
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    fb ^= 1;

    // init the flipper
    GX_Init(gpfifo,DEFAULT_FIFO_SIZE);

    // clears the bg to color and clears the z buffer
    GX_SetCopyClear(background,0x00FFFFFF);

    // other gx setup
    GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
    yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
    GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

    if (rmode->aa) {
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    } else {
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    }

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(frameBuffer[fb],GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // setup the vertex attribute table
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    // set number of rasterized color channels
    GX_SetNumChans(1);

    //set number of textures to generate
    GX_SetNumTexGens(1);

    GX_InvVtxCache();
    GX_InvalidateTexAll();


    // setup our camera at the origin
    // looking down the -z axis with y up
    guLookAt(view, &cam, &up, &look);

    // setup our projection matrix
    // this creates a perspective matrix with a view angle of 90,
    // and aspect ratio based on the display resolution
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
    guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

    buildLists();

    WPAD_SetDataFormat(-1, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetMotionPlus(-1, 1);

    Wiimote blueMote(0);
    Wiimote redMote(1);

    guVector blueRot, redRot;
    guVector bluePos, redPos;

    Mtx33 blueMtx, redMtx;

    guVector blueActualAccel, redActualAccel;

    unsigned long lastLoop = millis();

    while(1) {
        unsigned long now = millis();
        double dt = ((double)(now - lastLoop)) * 0.001; // deltaTime in seconds
        lastLoop = now;

        WPAD_ScanPads();
        blueMote.update(dt);
        redMote.update(dt);

        // TODO use Wiimote API here
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) exit(0);

        //blueRot.x = blueMote.orient.y; // pitch
        //blueRot.y = blueMote.orient.x; // roll

        redRot.x = redMote.orient.y;   // pitch
        redRot.y = redMote.orient.x;   // roll
        redRot.z = redMote.orient.z;

        //blueActualAccel = makeGuVector(wd0->accel.x, wd0->accel.y, wd0->accel.z ) - blueAccCalibration;
        //redActualAccel = makeGuVector(wd1->accel.x, wd1->accel.y, wd1->accel.z ) - redAccCalibration;



        //bluePos.x += (wd0->accel.x - blueAccCalibration.x) / 100000.f;
        //bluePos.y += (wd0->accel.y - blueAccCalibration.x) / 10000.f;
        //bluePos.z += (wd0->accel.z - blueAccCalibration.x) / 100000.f;


        if(first_frame) {
            first_frame = false;
            VIDEO_SetBlack(FALSE);
        }

        setLight(view); // Setup the light

        // ----------------
        // draw things here
        // ----------------


        // draw the cubes
        // TODO it's probably bad to copy the texture into GX_TEXMAP0 every time
        GX_LoadTexObj(&cubeTexture, GX_TEXMAP0);
        draw({5, -2, -20}, view, &redCube, cubeDispListSize);
        draw({-5, -2, -20}, view, &blueCube, cubeDispListSize);

        GX_LoadTexObj(&saberTexture, GX_TEXMAP0);
        draw({4 + bluePos.x, 0 + bluePos.y, -13 + bluePos.z}, {0.5, 4, 0.5}, blueRot, view, &blueSaber, cubeDispListSize);
        draw({-4 + redPos.x, 0 + redPos.y, -13 + redPos.z}, {0.5, 4, 0.5}, redRot, view, &redSaber, cubeDispListSize);


        // done drawing
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(frameBuffer[fb],GX_TRUE);

        GX_DrawDone();

        VIDEO_SetNextFramebuffer(frameBuffer[fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
        fb ^= 1;
    }

    return 0;
}

void buildLists() {
    // cube
    TPLFile cubeTPL;

    TPL_OpenTPLFromMemory(&cubeTPL, (void *)Cube_tpl,Cube_tpl_size);
    TPL_GetTexture(&cubeTPL, cube, &cubeTexture);

    cubeDispListSize = makeCube(&redCube, {1, 0, 0});
    makeCube(&blueCube, {0, 0, 1});

    // sabers
    TPLFile saberTPL;

    TPL_OpenTPLFromMemory(&saberTPL, (void *)Saber_tpl,Saber_tpl_size);
    TPL_GetTexture(&saberTPL, saber, &saberTexture);
    makeCube(&redSaber, {1, 0, 0});
    makeCube(&blueSaber, {0, 0, 1});

    // make textures render better
    Mtx mv, mr;
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    guLightPerspective(mv, 45, (f32)w/h, 1.05f, 1.0f, 0.0f, 0.0f);
    guMtxTrans(mr, 0.0f, 0.0f, -1.0f);
    guMtxConcat(mv, mr, mv);
    GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);
    GX_InvalidateTexAll();

    // Set up TEV to paint the textures properly.
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
}

u32 makeCube(void** theCube, guVector color)
{
    *theCube = memalign(32, 896);
    memset(*theCube, 0, 896);
    DCInvalidateRange(*theCube, 896);
    GX_BeginDispList(*theCube, 896);

    GX_Begin(GX_QUADS,GX_VTXFMT0,24); // Start drawing
        // Bottom face
        GX_Position3f32(-1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,1.0f); // Top right
        GX_Position3f32( 1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,1.0f); // Top left
        GX_Position3f32( 1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,0.0f); // Bottom left
        GX_Position3f32(-1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,0.0f); // Bottom right
        // Front face
        GX_Position3f32(-1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,0.0f); // Bottom left
        GX_Position3f32( 1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,0.0f); // Bottom right
        GX_Position3f32( 1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,1.0f); // Top right
        GX_Position3f32(-1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,1.0f); // Top left
        // Back face
        GX_Position3f32(-1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,0.0f); // Bottom right
        GX_Position3f32(-1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,1.0f); // Top right
        GX_Position3f32( 1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,1.0f); // Top left
        GX_Position3f32( 1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,0.0f); // Bottom left
        // Right face
        GX_Position3f32( 1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,0.0f); // Bottom right
        GX_Position3f32( 1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,1.0f); // Top right
        GX_Position3f32( 1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,1.0f); // Top left
        GX_Position3f32( 1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,0.0f); // Bottom left
        // Left face
        GX_Position3f32(-1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,0.0f); // Bottom right
        GX_Position3f32(-1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,0.0f); // Top right
        GX_Position3f32(-1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,1.0f); // Top left
        GX_Position3f32(-1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,1.0f); // Bottom left
        // Top face
        GX_Position3f32(-1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,1.0f); // Top left
        GX_Position3f32(-1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(0.0f,0.0f); // Bottom left
        GX_Position3f32( 1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,0.0f); // Bottom rught
        GX_Position3f32( 1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
        GX_Color3f32(color.x,color.y,color.z);
        GX_TexCoord2f32(1.0f,1.0f); // Top right
    GX_End();         // Done drawing quads

    // GX_EndDispList() returns the size of the display list, so store that value and use it with GX_CallDispList().
    return GX_EndDispList(); // Done building the box list
}

void setLight(Mtx view)
{
    guVector lpos;
    GXLightObj lobj;

    lpos.x = 0;
    lpos.y = 0;
    lpos.z = 2.0f;

    guVecMultiply(view,&lpos,&lpos);

    GX_InitLightPos(&lobj,lpos.x,lpos.y,lpos.z);
    GX_InitLightColor(&lobj,lightColor[0]);
    GX_LoadLightObj(&lobj,GX_LIGHT0);

    // set number of rasterized color channels
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0,GX_ENABLE,GX_SRC_VTX,GX_SRC_VTX,GX_LIGHT0,GX_DF_CLAMP,GX_AF_NONE);
    GX_SetChanAmbColor(GX_COLOR0A0,lightColor[1]);
    GX_SetChanMatColor(GX_COLOR0A0,lightColor[2]);
}
