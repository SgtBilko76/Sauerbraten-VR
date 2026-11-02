// Android-only C-linkage entry points bridging the OpenXR native thread
// (android/app/src/main/cpp/vr_glue/, plain C) and the C++ engine. Phase 4
// of the Quest VR port: replaces main.cpp's own for(;;) loop on Android
// with per-frame calls driven by the OpenXR frame loop instead.
#ifndef ANDROID_BRIDGE_H
#define ANDROID_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Implemented in main.cpp == desktop's main(), called once by the VR
// native thread (matching QuakeQuest's AppThreadFunction pattern) after
// TBXR_InitRenderer() has an EGL context already current on that thread.
// On Android, main() runs one-time init only and returns instead of
// entering its own loop (see main()'s __ANDROID__ branch) -- everything
// afterward is driven per-frame by the three functions below.
int android_sauer_main(int argc, char **argv);

// Called once per XR frame, before the per-eye render loop: game-logic
// tick only (input/physics/world update), no rendering.
void android_sauer_tick(void);

// Called once per eye, after the OpenXR swapchain image for that eye is
// bound as the current framebuffer: renders the main menu or the game
// world (whichever is active) using the engine's existing camera/proj
// state. Both eyes render identically for now (monoscopic in the
// headset) until Phase 5 wires real per-eye stereo matrices.
void android_sauer_drawframe(void);

// Called once per XR frame, after both eyes are drawn: engine-side
// end-of-frame bookkeeping (desktop's swapbuffers(), which already
// no-ops its actual SDL_GL_SwapWindow call on __ANDROID__ -- OpenXR's
// swapchain release, done separately by the native thread, is the real
// present).
void android_sauer_endframe(void);

// Implemented in vr_glue/sauerquest_vr_bootstrap.c: reports the OpenXR
// per-eye swapchain image size. Already known by the time
// android_sauer_main() runs, since TBXR_InitRenderer() completes before
// it's called.
void android_get_eye_size(int *w, int *h);

/* Métricas del runtime (XR_META_performance_metrics) y foveado fijo, para el
 * panel vrstats de rendergl.cpp. Implementadas en vr_glue/sauerquest_vr_bootstrap.c
 * sobre vr_glue/VrPerf.c. Valores -1 = no disponible. */
typedef struct {
    float appCpuMs, appGpuMs, compCpuMs, compGpuMs, latencyMs;
    float cpuUtilAvg, cpuUtilWorst, gpuUtil;
    int droppedFrames, foveationLevel, foveationDynamic;
    float refreshHz;
} android_vr_metrics;
int android_vr_get_metrics(android_vr_metrics *m);
void android_vr_set_foveation(int level, int dynamic);
/* Telemetría: una línea de texto por UDP broadcast al PC (vr_glue/VrTelemetry.c). */
void android_vr_telemetry(const char *line);
/* Ojo que se está dibujando (0 izquierdo, 1 derecho); lo pone el bucle VR antes de
 * cada android_sauer_drawframe(). Lo usan los pases que se comparten entre ojos. */
void android_sauer_set_eye_index(int eye);
/* Multiview: los dos ojos de golpe (offsets cabeza→ojo en ejes de Sauerbraten,
 * orientación común y tan de cada frustum). El motor dibuja una vez. */
void android_sauer_set_eyes(float dx0, float dy0, float dz0, float dx1, float dy1, float dz1,
                            float yaw, float pitch, float roll,
                            float tl0, float tr0, float tu0, float td0,
                            float tl1, float tr1, float tu1, float td1);
/* 1 si el glue creó la swapchain en array y espera shaders multiview. */
int android_vr_multiview(void);
/* Lee la capa 0 del framebuffer del ojo (lo que ve el ojo izquierdo), lo reduce
 * y lo manda al PC por UDP en trozos (tools/apk/capture.mjs lo guarda en PNG). */
void android_vr_capture(int scale); /* 3 = un tercio de resolución; 2 para inspeccionar detalle */
int android_vr_unicast(void); /* 1 si la telemetría va directa al PC */
const char *android_vr_build(void); /* versionName de la APK (p. ej. 0.1.4403-7973186) */
int android_vr_status(char *buf, int len); /* estado de la telemetría para el HUD */
int android_vr_srgbcontrol(void); /* 1 si el glue desactivó la codificación sRGB al escribir */
/* Tiempos medios por frame (ms) del bucle VR desde la última llamada: espera en
 * xrWaitFrame (TBXR_FrameSetup), xrEndFrame (TBXR_submitFrame) y frame completo.
 * Devuelve el número de frames acumulados y pone los acumuladores a cero. */
int android_vr_get_frametimes(float *waitms, float *submitms, float *totalms);

// Implemented in rendergl.cpp; called once per eye, immediately before
// android_sauer_drawframe(), to report that eye's real OpenXR pose/FOV
// for this frame. setcammatrix()/gl_drawframe() read these (instead of
// camera1's mouse-driven yaw/pitch/roll and the fov cvar) on Android.
//   dx/dy/dz: eye position offset from the head, in Sauerbraten's own
//     "quake style" world axes (already remapped from OpenXR's Y-up axes
//     by the caller, matching hmdorientation's convention) and in real
//     *meters*, not world units -- scaled by the vrworldscale cvar on
//     this side so it's runtime-tunable without a rebuild.
//   yaw/pitch/roll: degrees, Sauerbraten's convention (this is head
//     orientation, shared by both eyes -- eyes don't rotate
//     independently of the head in this rig, only position differs).
//   tanLeft/tanRight/tanUp/tanDown: tan() of the per-eye asymmetric FOV
//     angles from xrLocateViews, i.e. already in the form
//     matrix4::frustum() needs once multiplied by the near clip plane.
void android_sauer_set_eye(float dx, float dy, float dz,
                            float yaw, float pitch, float roll,
                            float tanLeft, float tanRight, float tanUp, float tanDown);

// Implemented in main.cpp; reports whether the engine is currently showing
// the (fully 2D/ortho -- see menus.cpp's g3d_addgui() GUI_2D forcing while
// mainmenu is set, and main.cpp's renderbackground(), also pure
// hudmatrix.ortho()) main menu rather than real 3D gameplay. The native VR
// frame loop uses this to switch TBXR's submission between a flat
// XrCompositionLayerQuad (correct for this ortho-only content -- a
// Projection layer's per-eye asymmetric FOV drives the compositor's
// distortion-mesh mapping of the rendered image, which doesn't apply
// correctly to content that was never rendered with any real per-eye
// perspective in the first place, confirmed on-device as the actual cause
// of a reported inability to binocularly fuse the main menu at all) and
// the normal stereo Projection layer used for real gameplay.
int android_sauer_is_mainmenu(void);

// Implemented in main.cpp; true whenever ANY gui window is showing --
// the boot main menu (same as android_sauer_is_mainmenu() above) OR an
// in-game one like the pause/options menu (ESCAPE) or scoreboard (TAB),
// both of which render via the engine's own real-3D "floating panel" gui
// path on Android (3dgui.cpp's usegui2d cap) rather than the boot menu's
// separate mono composition layer. Used to decide whether the right
// controller's trigger should click a menu instead of firing the weapon.
int android_sauer_is_menu_open(void);

// Implemented in 3dgui.cpp/main.cpp; let the right controller's raycast
// against the main-menu "virtual screen" (see VR_UseScreenLayer()) drive
// the existing 3D GUI cursor and click state, in place of desktop's
// SDL-mouse-driven g3d_movecursor()/processkey(-1,...) path (never fires
// on Android -- no SDL window/mouse). x/y are normalized [0,1] with
// (0,0) at the top-left, matching g3d_addgui()'s GUI_2D ortho space.
void android_sauer_set_cursor(float x, float y);
void android_sauer_click(int isdown);

// Implemented in rendergl.cpp; reports the right controller's current
// aim pose, decoupling weapon aiming/shooting from head direction (the
// confirmed scope for this port -- see androidgetaim() in iengine.h,
// which is what actually reads this to drive the shoot raycast and
// viewmodel orientation). Same conventions as android_sauer_set_eye():
// dx/dy/dz is the controller's position offset from the head in real
// meters (Sauerbraten world axes), yaw/pitch/roll in degrees. active=0
// means the controller isn't currently tracked.
void android_sauer_set_aim(float dx, float dy, float dz,
                            float yaw, float pitch, float roll, int active);

// Implemented in main.cpp; maps the right controller's trigger to the
// same fire input desktop's left mouse button gives (physics.cpp's
// `attack` ICOMMAND -> game::doattack()). Only meaningful during actual
// gameplay -- the native frame loop uses android_sauer_is_mainmenu() to
// decide whether a trigger press should be this or a GUI click instead.
void android_sauer_fire(int isdown);

// Implemented in main.cpp; sets player1's held movement direction every
// frame from the left thumbstick (this port's smooth-stick locomotion),
// the same role desktop's held forward/back/strafe keys play. sideways/
// forward are already deadzone-filtered, roughly [-1,1], but only their
// sign is used -- Sauerbraten's own move/strafe fields (schar, ents.h)
// are discrete on desktop too (a key is either held or not); the
// "smooth" part of smooth-stick locomotion is the continuous per-tick
// world-space motion those discrete flags drive, not multi-speed analog
// throttling. Positive forward = move forward; sideways's sign is
// flipped internally (main.cpp) from the naive direct joystick.x
// mapping -- confirmed on-device as backwards otherwise.
void android_sauer_set_move(float sideways, float forward);

// Implemented in rendergl.cpp; adds to the persistent body-yaw offset
// applied on top of raw head yaw (androidBodyYaw, alongside
// androidEyeYaw et al) -- this port's snap-turn locomotion. Called once
// per detected stick-past-threshold edge (sauerquest_vr_bootstrap.c),
// not continuously, so each call is one discrete snap, positive
// degrees turning the same direction increasing yaw already does
// (Sauerbraten's own convention: increasing yaw turns left).
void android_sauer_snap_turn(float degrees);

// Implemented in main.cpp; these three route through processkey() the
// exact same way desktop's own SPACE/ESCAPE/TAB key events do (see
// main.cpp's SDL_KEYDOWN/UP handling), so they pick up whatever those
// keys are bound to (defaults.cfg: SPACE->jump, ESCAPE->togglemainmenu,
// TAB->showscores) rather than duplicating that binding logic here.
void android_sauer_jump(int isdown);
void android_sauer_menu(int isdown);
void android_sauer_tab(int isdown);

// Implemented in main.cpp; pops one level off the gui menu stack
// (cleargui(1), the same "go back" step togglemainmenu itself falls
// back to before opening the top-level main screen) -- unlike
// android_sauer_menu() above (ESCAPE/togglemainmenu, which can also
// OPEN the menu from nothing), this only ever goes back/closes, never
// opens, matching a console-style "B = back" convention. Fires once per
// press, ignores release; safe to call with no menu open (cleargui(1)
// itself no-ops on an empty stack).
void android_sauer_menu_back(int isdown);

// Implemented in main.cpp; cycles to the next (dir>0) or previous
// (dir<0) weapon, the same way desktop's mouse wheel does
// (processkey(-4/-5, ...), bound to delta_do 1/-1 in defaults.cfg) --
// momentary, not a held state, so each call is one discrete step.
void android_sauer_weapon_cycle(int dir);

#ifdef __cplusplus
}
#endif

#endif
