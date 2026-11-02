/*
 * SauerQuest VR bootstrap.
 *
 * This is a from-scratch JNI/lifecycle driver (not adapted from QuakeQuest)
 * built on top of TBXR_Common.c/OpenXrInput.c's OpenXR session/swapchain/
 * frame loop. Phase 1 proved the toolchain end to end with a standalone
 * colored clear and no engine linkage; Phase 4 (here) calls into the real
 * Sauerbraten engine every frame via androidbridge.h's C-linkage entry
 * points, mirroring QuakeQuest's AppThreadFunction pattern: boot the
 * engine once via android_sauer_main(), then drive per-frame
 * tick/draw/endframe from this thread's own loop instead of the engine
 * owning one itself (see src/engine/main.cpp's __ANDROID__ branch).
 *
 * Both eyes still render identically (monoscopic) using the engine's
 * existing single-camera state -- real per-eye stereo matrices are
 * Phase 5's job, not this one. The goal here is just to get real engine
 * pixels (the main menu, initially) on screen and prove Phase 3's GLES3
 * fixes actually work at runtime, not just that they compile.
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <pthread.h>

#include "VrCommon.h"
#include "androidbridge.h"
#include "VrPerf.h"
#include "VrTelemetry.h"
#include <time.h>

static double vrGlueWait = 0, vrGlueSubmit = 0, vrGlueTotal = 0; static int vrGlueFrames = 0;
static double nowms(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6; }
int android_vr_get_frametimes(float *waitms, float *submitms, float *totalms)
{
    int n = vrGlueFrames > 0 ? vrGlueFrames : 1;
    *waitms = (float)(vrGlueWait / n); *submitms = (float)(vrGlueSubmit / n); *totalms = (float)(vrGlueTotal / n);
    int frames = vrGlueFrames; vrGlueWait = vrGlueSubmit = vrGlueTotal = 0; vrGlueFrames = 0;
    return frames;
}

/* Same local definition TBXR_Common.c uses (DarkPlaces normally pulls
 * this in transitively via its own mathlib.h). */
#ifndef DEG2RAD
#define DEG2RAD(x) ((x) * (float)(M_PI / 180.0))
#endif

void jni_shutdown(void); /* defined below; VR_Shutdown() needs it forward-declared */

/* Sauerbraten's own logging (logoutf() -> stdout, see src/engine/server.cpp)
 * goes nowhere on Android unless something redirects stdout/stderr to
 * logcat -- there is no terminal for it to inherit. Standard NDK pattern:
 * pipe stdout/stderr into a background thread that forwards each line via
 * __android_log_write. Started once, before android_sauer_main(), so init
 * logging is visible from the very first line. */
static void *StdioLogThread(void *arg)
{
    int fd = *(int *)arg;
    free(arg);
    char buf[512];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        if (buf[n - 1] == '\n') n--;
        buf[n] = '\0';
        __android_log_write(ANDROID_LOG_INFO, "SauerQuest", buf);
    }
    return NULL;
}

static void RedirectStdioToLogcat(void)
{
    int pipefd[2];
    if (pipe(pipefd) != 0) return;

    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    int *readFd = (int *)malloc(sizeof(int));
    *readFd = pipefd[0];
    pthread_t thread;
    pthread_create(&thread, NULL, StdioLogThread, readFd);
    pthread_detach(thread);
}

/* ---- Required by TBXR_Common.c (referenced even on code paths we never
 * take, e.g. the flat "screen layer" branch of TBXR_submitFrame) ---- */
float playerYaw = 0.0f;
float playerHeight = 0.0f;
vec3_t hmdorientation = {0, 0, 0};

ovrInputStateTrackedRemote leftTrackedRemoteState_old;
ovrInputStateTrackedRemote leftTrackedRemoteState_new;
ovrTrackedController leftRemoteTracking_new;
ovrInputStateTrackedRemote rightTrackedRemoteState_old;
ovrInputStateTrackedRemote rightTrackedRemoteState_new;
ovrTrackedController rightRemoteTracking_new;

extern int runStatus; /* defined in TBXR_Common.c, -1 while running */

/* ---- Game-specific callbacks required by TBXR_Common.c ---- */

void VR_FrameSetup(void)
{
}

bool VR_UseScreenLayer(void)
{
    /* The main menu (menus.cpp's g3d_addgui() forces GUI_2D while
     * mainmenu is set, and main.cpp's renderbackground()) is rendered
     * entirely via hudmatrix.ortho() -- flat screen-space content with no
     * real per-eye perspective at all. Submitting that through the normal
     * stereo XrCompositionLayerProjection is wrong: the compositor maps
     * each eye's rendered image onto the display using that eye's own
     * declared asymmetric FOV (its distortion-mesh/reprojection metadata),
     * which only produces a correct result for content that was actually
     * rendered with a matching per-eye perspective. Flat ortho content
     * has none, so the two eyes' declared (and genuinely different, real
     * lens-driven asymmetric) FOVs each warp the *same* image
     * differently -- confirmed on-device as the actual cause of a
     * reported inability to binocularly fuse the main menu at all, even
     * with eye-to-eye camera *position* forced completely identical
     * (ruling out every other stereo-math candidate first). The
     * XrCompositionLayerCylinderKHR path below (TBXR_Common.c's
     * TBXR_submitFrame(), falling back to a flat Quad on runtimes without
     * that extension) instead submits it as a real curved panel
     * positioned in stage space, shown identically to both eyes
     * regardless of per-eye FOV asymmetry -- the standard, correct
     * technique for screen-locked 2D VR content. Real gameplay
     * (gl_drawframe(), real per-eye asymmetric frustums matching the
     * declared FOV) still needs the true stereo projection layer. */
    return android_sauer_is_mainmenu() != 0;
}

float VR_GetScreenLayerDistance(void)
{
    return SAUERQUEST_MENU_SCREEN_DISTANCE;
}

void VR_SetHMDOrientation(float pitch, float yaw, float roll)
{
    hmdorientation[0] = pitch;
    hmdorientation[1] = yaw;
    hmdorientation[2] = roll;
    playerYaw = yaw;
}

void VR_SetHMDPosition(float x, float y, float z)
{
    playerHeight = y;
}

/* The main menu screen's pose (see VrCommon.h's SauerQuest_GetMenuScreenPose()
 * declaration for why this is cached instead of read from live head
 * tracking every frame -- doing that made the screen visibly chase the
 * player's head/gaze instead of staying fixed in the room). Anchored
 * fresh, facing wherever the player was looking, each time the menu
 * transitions from closed to open. */
static XrPosef sMenuScreenPose;
static bool sMenuScreenAnchored = false;

static void SauerQuest_UpdateMenuScreenAnchor(void)
{
    if (!android_sauer_is_mainmenu()) {
        sMenuScreenAnchored = false; /* re-anchor fresh next time the menu opens */
        return;
    }
    if (sMenuScreenAnchored) return;

    const XrVector3f axis = {0.0f, 1.0f, 0.0f};
    sMenuScreenPose.position = gAppState.xfStageFromHead.position;
    sMenuScreenPose.orientation = XrQuaternionf_CreateFromVectorAngle(axis, DEG2RAD(playerYaw));
    sMenuScreenAnchored = true;
}

XrPosef SauerQuest_GetMenuScreenPose(void)
{
    return sMenuScreenPose;
}

/* Right-controller laser-pointer emulation for the main menu's curved
 * "virtual screen" (see VR_UseScreenLayer()). Sauerbraten's own 3D GUI
 * cursor (3dgui.cpp's cursorx/cursory) is desktop-mouse-driven --
 * relative deltas fed through g3d_movecursor() from SDL_MOUSEMOTION --
 * which never fires on Android (no SDL window/mouse). This instead
 * raycasts the right controller's aim pose against the screen and feeds
 * the hit point in as an absolute cursor position, and maps the trigger
 * to the same processkey(-1, isdown) call desktop's SDL_BUTTON_LEFT
 * handler makes.
 *
 * The screen may actually be a slightly curved XrCompositionLayerCylinderKHR
 * (TBXR_Common.c), but for SAUERQUEST_MENU_SCREEN_WIDTH's central angle
 * the curve's deviation from a flat plane at the same center/distance is
 * a few centimeters at most -- well under pointer precision needed for a
 * menu, so a flat-plane intersection test is used instead of solving the
 * actual cylinder surface, since it's far simpler and visually
 * indistinguishable.
 *
 * The in-game pause/options menu and scoreboard (opened via the left
 * Menu/Y buttons -- see VR_HandleControllerInput() below -- while
 * android_sauer_is_mainmenu() stays false, since a game is still
 * running) need no cursor-position math at all: they're real-3D
 * "floating panel" gui windows (3dgui.cpp's usegui2d cap on Android),
 * whose own hit-test already reads the controller's aim ray directly
 * (androidgetaim(), 3dgui.cpp) instead of head direction. Only the
 * trigger's click-forwarding needs to keep running for those, gated on
 * the more general android_sauer_is_menu_open() instead of the
 * boot-menu-only android_sauer_is_mainmenu(). */
static void SauerQuest_UpdateMenuPointer(void)
{
    static bool triggerWasDown = false;

    if (!android_sauer_is_menu_open()) {
        triggerWasDown = false; /* don't carry a stale click into gameplay */
        return;
    }

    if (!android_sauer_is_mainmenu()) {
        bool triggerDown = (rightTrackedRemoteState_new.Buttons & xrButton_Trigger) != 0;
        if (triggerDown != triggerWasDown) {
            android_sauer_click(triggerDown ? 1 : 0);
            triggerWasDown = triggerDown;
        }
        return;
    }
    if (!rightRemoteTracking_new.Active) return;

    XrPosef screenPose = SauerQuest_GetMenuScreenPose();
    XrVector3f screenForward = XrQuaternionf_Rotate(screenPose.orientation, (XrVector3f){0.0f, 0.0f, -1.0f});
    XrVector3f screenRight   = XrQuaternionf_Rotate(screenPose.orientation, (XrVector3f){1.0f, 0.0f, 0.0f});
    XrVector3f screenUp      = XrQuaternionf_Rotate(screenPose.orientation, (XrVector3f){0.0f, 1.0f, 0.0f});

    /* Cylinder/quad pose.position is the player/axis, not the visible
     * surface -- matches TBXR_submitFrame()'s own placement. */
    XrVector3f screenCenter = {
        screenPose.position.x + screenForward.x * SAUERQUEST_MENU_SCREEN_DISTANCE,
        screenPose.position.y + screenForward.y * SAUERQUEST_MENU_SCREEN_DISTANCE,
        screenPose.position.z + screenForward.z * SAUERQUEST_MENU_SCREEN_DISTANCE
    };

    XrVector3f rayOrigin = rightRemoteTracking_new.Pose.position;
    XrVector3f rayDir = XrQuaternionf_Rotate(rightRemoteTracking_new.Pose.orientation, (XrVector3f){0.0f, 0.0f, -1.0f});

    float denom = rayDir.x*screenForward.x + rayDir.y*screenForward.y + rayDir.z*screenForward.z;
    if (denom > 0.0001f) {
        XrVector3f toCenter = {
            screenCenter.x - rayOrigin.x, screenCenter.y - rayOrigin.y, screenCenter.z - rayOrigin.z
        };
        float t = (toCenter.x*screenForward.x + toCenter.y*screenForward.y + toCenter.z*screenForward.z) / denom;
        if (t > 0.0f) {
            XrVector3f hit = {
                rayOrigin.x + rayDir.x*t, rayOrigin.y + rayDir.y*t, rayOrigin.z + rayDir.z*t
            };
            XrVector3f toHit = { hit.x - screenCenter.x, hit.y - screenCenter.y, hit.z - screenCenter.z };
            float localRight = toHit.x*screenRight.x + toHit.y*screenRight.y + toHit.z*screenRight.z;
            float localUp    = toHit.x*screenUp.x    + toHit.y*screenUp.y    + toHit.z*screenUp.z;

            float u = 0.5f + localRight / SAUERQUEST_MENU_SCREEN_WIDTH;
            float v = 0.5f - localUp / SAUERQUEST_MENU_SCREEN_HEIGHT;
            if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
            if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;
            android_sauer_set_cursor(u, v);
        }
    }

    bool triggerDown = (rightTrackedRemoteState_new.Buttons & xrButton_Trigger) != 0;
    if (triggerDown != triggerWasDown) {
        android_sauer_click(triggerDown ? 1 : 0);
        triggerWasDown = triggerDown;
    }
}

/* Controller-aimed weapon (decoupled from head direction -- this port's
 * confirmed scope for aiming). Reports the right controller's pose,
 * head-relative like android_sauer_set_eye()'s own eye offset, so
 * androidgetaim() (rendergl.cpp, via iengine.h) can resolve it into
 * world space the exact same way: rotate by camera1's *current*
 * yaw/pitch/roll rather than baking in today's head orientation here,
 * so this keeps working once body-yaw/locomotion decouples camera1's
 * yaw from raw head yaw (not yet the case -- no locomotion exists yet). */
static void SauerQuest_UpdateWeaponAim(void)
{
    if (!rightRemoteTracking_new.Active) {
        android_sauer_set_aim(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
        return;
    }

    XrPosef xfHeadFromController = XrPosef_Multiply(
        XrPosef_Inverse(gAppState.xfStageFromHead),
        rightRemoteTracking_new.Pose);

    /* Same {-x,-z,y} -> Sauerbraten {lateral,forward,up} remap as the
     * eye offset (rendergl.cpp's android_sauer_set_eye() comment) --
     * NOT the {-z,-x,y} convention TBXR's own QuatToYawPitchRoll() uses,
     * which is Quake/DarkPlaces's axis order, not Cube2's. */
    float dx = -xfHeadFromController.position.x;
    float dy = -xfHeadFromController.position.z;
    float dz =  xfHeadFromController.position.y;

    vec3_t rotation = {0.0f, 0.0f, 0.0f};
    vec3_t aimAngles;
    QuatToYawPitchRoll(rightRemoteTracking_new.Pose.orientation, rotation, aimAngles);

    android_sauer_set_aim(dx, dy, dz, aimAngles[1], aimAngles[0], aimAngles[2], 1);
}

static void SauerQuest_UpdateWeaponFire(void)
{
    static bool triggerWasDown = false;

    /* Also gated on the pause/options menu and scoreboard now, not just
     * the boot main menu -- SauerQuest_UpdateMenuPointer() above claims
     * the trigger as a menu click whenever any of those are open, so
     * this needs to back off the same way to avoid also firing a shot. */
    if (android_sauer_is_menu_open()) {
        triggerWasDown = false; /* don't carry a stale fire-hold into the menu */
        return;
    }

    bool triggerDown = (rightTrackedRemoteState_new.Buttons & xrButton_Trigger) != 0;
    if (triggerDown != triggerWasDown) {
        android_sauer_fire(triggerDown ? 1 : 0);
        triggerWasDown = triggerDown;
    }
}

/* Smooth-stick movement (left thumbstick) + snap-turn (right thumbstick,
 * X axis) -- this port's locomotion, following the same left/right split
 * QuakeQuest's own reference (vr_glue/reference/QuakeQuest_OpenXR.c) uses.
 * android_sauer_set_move() itself no-ops on the main menu (main.cpp), so
 * only snap-turn needs its own explicit menu gate here -- rotating
 * camera1->yaw while the menu's cylinder/quad panel is on-screen would be
 * pointless (that panel is positioned from a cached head-anchor, not
 * camera1->yaw) and could only cause a stray jump once gameplay resumes. */
static void SauerQuest_UpdateLocomotion(void)
{
    android_sauer_set_move(leftTrackedRemoteState_new.Joystick.x,
                            leftTrackedRemoteState_new.Joystick.y);

    if (android_sauer_is_mainmenu()) return;

    /* Snap-turn fires once per crossing of the +/-0.7 threshold, not
     * continuously while held past it -- same edge-detection pattern
     * QuakeQuest's own reference uses for its snap-turn (and for this
     * project's own trigger/click edge detection above). 45 degrees is a
     * common, comfort-oriented snap-turn increment; positive turns the
     * same direction increasing yaw already does (turn left, per
     * Sauerbraten's own convention -- physics.cpp's vecfromyawpitch()). */
    const float threshold = 0.7f;
    const float snapdegrees = 45.0f;
    static int snapState = 0; /* -1 = past left threshold, 1 = past right, 0 = neither */

    // Confirmed on-device: the naive direct mapping (push right ->
    // negative/turn-right) was left/right-reversed, same as sideways
    // movement above -- flipped here too.
    float turnx = rightTrackedRemoteState_new.Joystick.x;
    if (turnx > threshold) {
        if (snapState != 1) { android_sauer_snap_turn(snapdegrees); snapState = 1; }
    } else if (turnx < -threshold) {
        if (snapState != -1) { android_sauer_snap_turn(-snapdegrees); snapState = -1; }
    } else {
        snapState = 0;
    }

    /* Right stick Y axis -> weapon cycle, same threshold-crossing
     * edge-detection as snap-turn above (one step per crossing, not
     * continuous while held past it). Up = next weapon, down = previous,
     * matching desktop's mouse-wheel-up/-down (delta_do 1/-1). */
    static int weaponCycleState = 0; /* -1 = past down threshold, 1 = past up, 0 = neither */
    float turny = rightTrackedRemoteState_new.Joystick.y;
    if (turny > threshold) {
        if (weaponCycleState != 1) { android_sauer_weapon_cycle(1); weaponCycleState = 1; }
    } else if (turny < -threshold) {
        if (weaponCycleState != -1) { android_sauer_weapon_cycle(-1); weaponCycleState = -1; }
    } else {
        weaponCycleState = 0;
    }
}

/* Left controller: index trigger -> jump, Menu button -> ESCAPE
 * (togglemainmenu), Y button -> TAB (showscores) -- same edge-detection
 * pattern as the right trigger's fire/click handling above, each routed
 * through android_sauer_jump()/menu()/tab() (main.cpp), which themselves
 * go through processkey() so they pick up whatever those keys are bound
 * to rather than hardcoding the bound command here. Unlike fire, jump is
 * gated off in the main menu the same way (so a trigger-happy player
 * sitting in the menu doesn't queue up a jump for when gameplay resumes);
 * ESCAPE and TAB are left ungated since ESCAPE is exactly how the menu
 * itself gets toggled and a TAB press while already in the menu is
 * harmless (showscores just no-ops outside a game). */
static void SauerQuest_UpdateLeftButtons(void)
{
    static bool jumpWasDown = false;
    static bool menuWasDown = false;
    static bool tabWasDown = false;

    bool jumpDown = !android_sauer_is_mainmenu() &&
        (leftTrackedRemoteState_new.Buttons & xrButton_Trigger) != 0;
    if (jumpDown != jumpWasDown) {
        android_sauer_jump(jumpDown ? 1 : 0);
        jumpWasDown = jumpDown;
    }

    bool menuDown = (leftTrackedRemoteState_new.Buttons & xrButton_Enter) != 0;
    if (menuDown != menuWasDown) {
        android_sauer_menu(menuDown ? 1 : 0);
        menuWasDown = menuDown;
    }

    bool tabDown = (leftTrackedRemoteState_new.Buttons & xrButton_Y) != 0;
    if (tabDown != tabWasDown) {
        android_sauer_tab(tabDown ? 1 : 0);
        tabWasDown = tabDown;
    }
}

/* Right controller: B button -> "go back" one menu level
 * (android_sauer_menu_back(), main.cpp -- cleargui(1)), console-style,
 * distinct from the left Menu button's ESCAPE/togglemainmenu (which can
 * also OPEN the menu from nothing). Edge-detected like every other
 * button here; cleargui(1) itself safely no-ops with no menu open, so
 * this is left ungated rather than checking android_sauer_is_menu_open()
 * first. */
static void SauerQuest_UpdateRightButtons(void)
{
    static bool backWasDown = false;

    bool backDown = (rightTrackedRemoteState_new.Buttons & xrButton_B) != 0;
    if (backDown != backWasDown) {
        android_sauer_menu_back(backDown ? 1 : 0);
        backWasDown = backDown;
    }
}

void VR_HandleControllerInput(void)
{
    SauerQuest_UpdateMenuScreenAnchor();
    TBXR_UpdateControllers();
    SauerQuest_UpdateMenuPointer();
    SauerQuest_UpdateWeaponAim();
    SauerQuest_UpdateWeaponFire();
    SauerQuest_UpdateLocomotion();
    SauerQuest_UpdateLeftButtons();
    SauerQuest_UpdateRightButtons();
}

void VR_Shutdown(void)
{
    jni_shutdown();
}

/* Called by src/engine/main.cpp's setupscreen() (via androidbridge.h) once
 * TBXR_InitRenderer() below has already sized the eye buffers. */
void android_get_eye_size(int *w, int *h)
{
    *w = (int)gAppState.Width;
    *h = (int)gAppState.Height;
}

int android_vr_get_metrics(android_vr_metrics *m)
{
    VrPerfMetrics p;
    int ok = VrPerf_Query(&p);
    m->appCpuMs = p.appCpuMs; m->appGpuMs = p.appGpuMs;
    m->compCpuMs = p.compCpuMs; m->compGpuMs = p.compGpuMs; m->latencyMs = p.latencyMs;
    m->cpuUtilAvg = p.cpuUtilAvg; m->cpuUtilWorst = p.cpuUtilWorst; m->gpuUtil = p.gpuUtil;
    m->droppedFrames = p.droppedFrames; m->foveationLevel = p.foveationLevel;
    m->foveationDynamic = p.foveationDynamic; m->refreshHz = p.refreshHz;
    return ok;
}

void android_vr_set_foveation(int level, int dynamic)
{
    VrPerf_SetFoveation(level, dynamic);
}

void android_vr_telemetry(const char *line)
{
    VrTelemetry_Send(line);
}

void *AppThreadFunction(void *parm)
{
    gAppThread = (ovrAppThread *)parm;

    java.Vm = gAppThread->JavaVm;
    (*java.Vm)->AttachCurrentThread(java.Vm, &java.Env, NULL);
    java.ActivityObject = gAppThread->ActivityObject;

    prctl(PR_SET_NAME, (long)"SauerQuestVRThread", 0, 0, 0);

    RedirectStdioToLogcat();

    gAppState.MainThreadTid = gettid();

    TBXR_InitialiseOpenXR();
    TBXR_EnterVR();
    TBXR_InitRenderer();
    TBXR_InitActions();
    TBXR_WaitForSessionActive();

    ALOGV("SauerQuest VR bootstrap: booting engine (%dx%d per eye)",
          (int)gAppState.Width, (int)gAppState.Height);

    /* Sauerbraten's own file I/O (src/shared/stream.cpp's openfile()) uses
     * plain fopen() with paths relative to the process's cwd -- it has no
     * concept of APK assets. SauerQuestActivity.onCreate() already
     * extracted the bundled data/packages assets to internal storage
     * (getFilesDir(), same path SDL_AndroidGetInternalStoragePath()
     * returns); chdir() there now so every relative fopen() the engine is
     * about to do just works. Declared extern rather than pulling in
     * SDL.h here (this file otherwise has no SDL dependency at all). */
    {
        extern const char *SDL_AndroidGetInternalStoragePath(void);
        const char *storagePath = SDL_AndroidGetInternalStoragePath();
        if (storagePath) chdir(storagePath);
        else ALOGV("SauerQuest VR bootstrap: SDL_AndroidGetInternalStoragePath() failed");
    }

    {
        int argc = 1;
        char *argv[] = { (char *)"sauerquest" };
        android_sauer_main(argc, argv);
    }

    ALOGV("SauerQuest VR bootstrap: engine booted, entering frame loop");

    double vrFrameStart = nowms();
    while (runStatus == -1) {
        double t0 = nowms();
        TBXR_FrameSetup();
        double t1 = nowms(); vrGlueWait += t1 - t0;

        android_sauer_tick();

        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            TBXR_prepareEyeBuffer(eye);
            if (gAppState.FrameState.shouldRender) {
                /* gAppState.Projections[eye].pose is the eye relative to
                 * HeadSpace -- a small IPD-driven offset only, NOT the
                 * player's real position in the room (eyes don't rotate
                 * independently of the head either, so only .position is
                 * used here). android_sauer_set_eye()'s contract
                 * (androidbridge.h) documents dx/dy/dz as exactly this:
                 * "eye position offset from the head". Composing in
                 * gAppState.xfStageFromHead (the player's real physical
                 * position/height in the guardian-defined stage space, via
                 * XrPosef_Multiply as TBXR_submitFrame() does for the
                 * actual compositor submission) does NOT belong here --
                 * this app has no room-scale/positional-tracking movement
                 * system yet, so the player's *real* height and stage
                 * position (easily 1.5+ real meters) would get added
                 * straight onto camera1->o after being scaled by
                 * vrworldscale, dwarfing the true ~3cm IPD offset and
                 * placing the render camera somewhere arbitrary relative
                 * to world geometry -- confirmed on-device as the actual
                 * cause of a reported inability to binocularly fuse the
                 * view (severe near-field parallax against whatever
                 * geometry the camera ended up next to/inside, not a
                 * subtle stereo-math error). */
                XrPosef xfHeadFromEye = gAppState.Projections[eye].pose;
                XrFovf fov = gAppState.Projections[eye].fov;

                /* OpenXR is Y-up/right-handed (+X right, +Y up, -Z
                 * forward). QuatToYawPitchRoll() (TBXR_Common.c) remaps
                 * its forward/right vectors as {-z,-x,y} too, but that
                 * convention is Quake/DarkPlaces's own (X=forward,
                 * Y=lateral) -- it's only used there to extract a scalar
                 * yaw/pitch/roll *angle*, which works out the same
                 * regardless of which axis is nominally "forward", so it
                 * never surfaced as a bug. Sauerbraten/Cube2's actual
                 * world axes are the opposite: X=lateral (strafe),
                 * Y=forward (verified against vecfromyawpitch() in
                 * physics.cpp -- move contributes to .y, strafe to .x).
                 * A *position* offset can't ignore that difference the
                 * way an angle can: naively reusing {-z,-x,y} here put
                 * the real ~3cm IPD offset (from OpenXR position.x, the
                 * lateral axis) onto Sauerbraten's Y (forward/back)
                 * instead of X (lateral) -- pushing one eye's camera
                 * toward the scene and the other away from it along the
                 * view axis instead of side-to-side. Confirmed on-device
                 * as the actual cause of a reported inability to
                 * binocularly fuse the view at all ("everything,
                 * uniformly" doubled -- consistent with a wrong-axis
                 * global camera-offset bug, not per-object depth error).
                 * dx now carries the lateral component onto Sauerbraten's
                 * X, dy the (near-zero, eyes don't offset forward/back)
                 * depth component onto Sauerbraten's Y. */
                float dx = -xfHeadFromEye.position.x;
                float dy = -xfHeadFromEye.position.z;
                float dz =  xfHeadFromEye.position.y;

                android_sauer_set_eye_index(eye);
                android_sauer_set_eye(dx, dy, dz,
                    hmdorientation[1], hmdorientation[0], hmdorientation[2],
                    tanf(fov.angleLeft), tanf(fov.angleRight), tanf(fov.angleUp), tanf(fov.angleDown));
                android_sauer_drawframe();
            }
            TBXR_finishEyeBuffer(eye);
        }

        android_sauer_endframe();

        double t2 = nowms();
        TBXR_submitFrame();
        double t3 = nowms(); vrGlueSubmit += t3 - t2; vrGlueTotal += t3 - vrFrameStart; vrFrameStart = t3; vrGlueFrames++;
    }

    TBXR_LeaveVR();
    VR_Shutdown();
    exit(0);

    return NULL;
}

/* ---- JNI lifecycle glue ---- */

static jmethodID android_shutdown;
static JavaVM *jVM;
static jobject jniCallbackObj = 0;

void jni_shutdown(void)
{
    JNIEnv *env;
    if (((*jVM)->GetEnv(jVM, (void **)&env, JNI_VERSION_1_4)) < 0) {
        (*jVM)->AttachCurrentThread(jVM, &env, NULL);
    }
    (*env)->CallVoidMethod(env, jniCallbackObj, android_shutdown);
}

/* No JNI_OnLoad of our own: SDL2's SDL_android.c already defines one (it
 * registers native methods for org/libsdl/app/SDLActivity classes we don't
 * have -- those registrations harmlessly fail-and-log, per its own
 * register_methods() error handling -- and captures SDL's internal
 * mJavaVM, which its audio backend needs). Two JNI_OnLoad definitions in
 * the same .so is a link error either way, and SDL's is the one that must
 * win, so jVM is captured here from onCreate's env instead. */

/* SDL_android.c's nativeSetupJNI() (mangled per its SDL_JAVA_INTERFACE
 * macro to Java_org_libsdl_app_SDLActivity_nativeSetupJNI) sets its global
 * mActivityClass -- required by SDL_AndroidGetInternalStoragePath() and
 * friends -- from whichever class calls it. Upstream, that's Java calling
 * into org.libsdl.app.SDLActivity's own native method declaration; this
 * port has no such class, so it's called directly as a plain C function
 * below instead, passing our real Activity's class (see
 * SauerQuestActivity.getContext(), the one static method on it SDL's
 * lookups actually need). */
extern void Java_org_libsdl_app_SDLActivity_nativeSetupJNI(JNIEnv *env, jclass cls);

JNIEXPORT jlong JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onCreate(JNIEnv *env, jclass activityClass, jobject activity)
{
    ALOGV("SauerQuestJNILib::onCreate()");

    (*env)->GetJavaVM(env, &jVM);

    jclass realActivityClass = (*env)->GetObjectClass(env, activity);
    Java_org_libsdl_app_SDLActivity_nativeSetupJNI(env, realActivityClass);

    ovrAppThread *appThread = (ovrAppThread *)malloc(sizeof(ovrAppThread));
    ovrAppThread_Create(appThread, env, activity, activityClass);

    surfaceMessageQueue_Enable(&appThread->MessageQueue, true);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_CREATE, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);

    return (jlong)((size_t)appThread);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onStart(JNIEnv *env, jobject obj, jlong handle, jobject obj1)
{
    ALOGV("SauerQuestJNILib::onStart()");

    jniCallbackObj = (jobject)((*env)->NewGlobalRef(env, obj1));
    jclass callbackClass = (*env)->GetObjectClass(env, jniCallbackObj);
    android_shutdown = (*env)->GetMethodID(env, callbackClass, "shutdown", "()V");

    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_START, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onResume(JNIEnv *env, jobject obj, jlong handle)
{
    ALOGV("SauerQuestJNILib::onResume()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_RESUME, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onPause(JNIEnv *env, jobject obj, jlong handle)
{
    ALOGV("SauerQuestJNILib::onPause()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_PAUSE, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onStop(JNIEnv *env, jobject obj, jlong handle)
{
    ALOGV("SauerQuestJNILib::onStop()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_STOP, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onDestroy(JNIEnv *env, jobject obj, jlong handle)
{
    ALOGV("SauerQuestJNILib::onDestroy()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_DESTROY, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
    surfaceMessageQueue_Enable(&appThread->MessageQueue, false);

    ovrAppThread_Destroy(appThread, env);
    free(appThread);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onSurfaceCreated(JNIEnv *env, jobject obj, jlong handle, jobject surface)
{
    ALOGV("SauerQuestJNILib::onSurfaceCreated()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);

    ANativeWindow *newNativeWindow = ANativeWindow_fromSurface(env, surface);
    appThread->NativeWindow = newNativeWindow;
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED);
    surfaceMessage_SetPointerParm(&message, 0, appThread->NativeWindow);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onSurfaceChanged(JNIEnv *env, jobject obj, jlong handle, jobject surface)
{
    ALOGV("SauerQuestJNILib::onSurfaceChanged()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);

    ANativeWindow *newNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (newNativeWindow != appThread->NativeWindow) {
        if (appThread->NativeWindow != NULL) {
            srufaceMessage message;
            surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED);
            surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
            ANativeWindow_release(appThread->NativeWindow);
            appThread->NativeWindow = NULL;
        }
        if (newNativeWindow != NULL) {
            appThread->NativeWindow = newNativeWindow;
            srufaceMessage message;
            surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED);
            surfaceMessage_SetPointerParm(&message, 0, appThread->NativeWindow);
            surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
        }
    } else if (newNativeWindow != NULL) {
        ANativeWindow_release(newNativeWindow);
    }
}

JNIEXPORT void JNICALL
Java_org_sauerquest_vr_SauerQuestJNILib_onSurfaceDestroyed(JNIEnv *env, jobject obj, jlong handle)
{
    ALOGV("SauerQuestJNILib::onSurfaceDestroyed()");
    ovrAppThread *appThread = (ovrAppThread *)((size_t)handle);
    srufaceMessage message;
    surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED);
    surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
    ANativeWindow_release(appThread->NativeWindow);
    appThread->NativeWindow = NULL;
}
