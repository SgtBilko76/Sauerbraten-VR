#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VrCommon.h"
#include "TBXR_Common.h"
#include "VrPerf.h"

bool vrPerfMetricsSupported = false;
bool vrFoveationSupported = false;

/* ---- métricas -------------------------------------------------------- */

static PFN_xrEnumeratePerformanceMetricsCounterPathsMETA pfnEnumeratePaths = NULL;
static PFN_xrSetPerformanceMetricsStateMETA pfnSetState = NULL;
static PFN_xrQueryPerformanceMetricsCounterMETA pfnQuery = NULL;
static PFN_xrPathToString pfnPathToString = NULL;

typedef enum {
    C_APP_CPU, C_APP_GPU, C_COMP_CPU, C_COMP_GPU, C_LATENCY,
    C_CPU_AVG, C_CPU_WORST, C_GPU_UTIL, C_DROPPED, C_COUNT
} CounterId;

static const char *counterNeedle[C_COUNT] = {
    "app/cpu_frametime", "app/gpu_frametime",
    "compositor/cpu_frametime", "compositor/gpu_frametime",
    "app/motion_to_photon_latency",
    "device/cpu_utilization_average", "device/cpu_utilization_worst",
    "device/gpu_utilization",
    "compositor/dropped_frame_count",
};

static XrPath counterPath[C_COUNT];
static bool metricsReady = false;
static XrSession gSession = XR_NULL_HANDLE;
static XrInstance gInstance = XR_NULL_HANDLE;

static void loadFn(XrInstance instance, const char *name, PFN_xrVoidFunction *out)
{
    if (xrGetInstanceProcAddr(instance, name, out) != XR_SUCCESS) *out = NULL;
}

static void initMetrics(XrInstance instance, XrSession session)
{
    if (!vrPerfMetricsSupported) return;
    loadFn(instance, "xrEnumeratePerformanceMetricsCounterPathsMETA", (PFN_xrVoidFunction *)&pfnEnumeratePaths);
    loadFn(instance, "xrSetPerformanceMetricsStateMETA", (PFN_xrVoidFunction *)&pfnSetState);
    loadFn(instance, "xrQueryPerformanceMetricsCounterMETA", (PFN_xrVoidFunction *)&pfnQuery);
    loadFn(instance, "xrPathToString", (PFN_xrVoidFunction *)&pfnPathToString);
    if (!pfnEnumeratePaths || !pfnSetState || !pfnQuery || !pfnPathToString) {
        ALOGE("VrPerf: faltan funciones de XR_META_performance_metrics");
        return;
    }

    XrPerformanceMetricsStateMETA state = { XR_TYPE_PERFORMANCE_METRICS_STATE_META, NULL, XR_TRUE };
    if (pfnSetState(session, &state) != XR_SUCCESS) {
        ALOGE("VrPerf: xrSetPerformanceMetricsStateMETA falló");
        return;
    }

    for (int i = 0; i < C_COUNT; i++) counterPath[i] = XR_NULL_PATH;

    uint32_t count = 0;
    pfnEnumeratePaths(instance, 0, &count, NULL);
    XrPath *paths = (XrPath *)calloc(count ? count : 1, sizeof(XrPath));
    pfnEnumeratePaths(instance, count, &count, paths);
    for (uint32_t i = 0; i < count; i++) {
        char name[XR_MAX_PATH_LENGTH];
        uint32_t len = 0;
        if (pfnPathToString(instance, paths[i], sizeof(name), &len, name) != XR_SUCCESS) continue;
        ALOGV("VrPerf: contador disponible %s", name);
        for (int c = 0; c < C_COUNT; c++)
            if (counterPath[c] == XR_NULL_PATH && strstr(name, counterNeedle[c])) counterPath[c] = paths[i];
    }
    free(paths);
    metricsReady = true;
    ALOGV("VrPerf: métricas activas (%u contadores en el runtime)", count);
}

static float queryFloat(CounterId c, int *ok)
{
    if (!metricsReady || counterPath[c] == XR_NULL_PATH) return -1;
    XrPerformanceMetricsCounterMETA m = { XR_TYPE_PERFORMANCE_METRICS_COUNTER_META, NULL, 0, 0, 0, 0 };
    if (pfnQuery(gSession, counterPath[c], &m) != XR_SUCCESS) return -1;
    if (m.counterFlags & XR_PERFORMANCE_METRICS_COUNTER_FLOAT_VALUE_VALID_BIT_META) { *ok = 1; return m.floatValue; }
    if (m.counterFlags & XR_PERFORMANCE_METRICS_COUNTER_UINT_VALUE_VALID_BIT_META) { *ok = 1; return (float)m.uintValue; }
    return -1;
}

/* ---- foveado --------------------------------------------------------- */

static PFN_xrCreateFoveationProfileFB pfnCreateProfile = NULL;
static PFN_xrDestroyFoveationProfileFB pfnDestroyProfile = NULL;
static PFN_xrUpdateSwapchainFB pfnUpdateSwapchain = NULL;
static XrFoveationProfileFB currentProfile = XR_NULL_HANDLE;
static int currentLevel = -1, currentDynamic = -1;

static XrSwapchainCreateInfoFoveationFB foveationCreateInfo = {
    XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB, NULL,
    XR_SWAPCHAIN_CREATE_FOVEATION_SCALED_BIN_BIT_FB
};

const void *VrPerf_FoveationCreateInfo(void)
{
    return vrFoveationSupported ? &foveationCreateInfo : NULL;
}

static void initFoveation(XrInstance instance)
{
    if (!vrFoveationSupported) return;
    loadFn(instance, "xrCreateFoveationProfileFB", (PFN_xrVoidFunction *)&pfnCreateProfile);
    loadFn(instance, "xrDestroyFoveationProfileFB", (PFN_xrVoidFunction *)&pfnDestroyProfile);
    loadFn(instance, "xrUpdateSwapchainFB", (PFN_xrVoidFunction *)&pfnUpdateSwapchain);
    if (!pfnCreateProfile || !pfnDestroyProfile || !pfnUpdateSwapchain) {
        ALOGE("VrPerf: faltan funciones de foveado");
        vrFoveationSupported = false;
    }
}

void VrPerf_SetFoveation(int level, int dynamic)
{
    if (!vrFoveationSupported || gSession == XR_NULL_HANDLE) return;
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    dynamic = dynamic ? 1 : 0;
    if (level == currentLevel && dynamic == currentDynamic) return;

    XrFoveationLevelProfileCreateInfoFB levelInfo = {
        XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB, NULL,
        (XrFoveationLevelFB)level, 0.0f,
        dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB : XR_FOVEATION_DYNAMIC_DISABLED_FB
    };
    XrFoveationProfileCreateInfoFB profileInfo = { XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB, &levelInfo };
    XrFoveationProfileFB profile = XR_NULL_HANDLE;
    if (pfnCreateProfile(gSession, &profileInfo, &profile) != XR_SUCCESS) {
        ALOGE("VrPerf: xrCreateFoveationProfileFB falló (nivel %d)", level);
        return;
    }

    XrSwapchainStateFoveationFB state = { XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB, NULL, 0, profile };
    for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
        XrSwapchain sc = gAppState.Renderer.FrameBuffer[eye].ColorSwapChain.Handle;
        if (sc == XR_NULL_HANDLE) continue;
        if (pfnUpdateSwapchain(sc, (const XrSwapchainStateBaseHeaderFB *)&state) != XR_SUCCESS)
            ALOGE("VrPerf: xrUpdateSwapchainFB falló (ojo %d)", eye);
    }

    if (currentProfile != XR_NULL_HANDLE) pfnDestroyProfile(currentProfile);
    currentProfile = profile;
    currentLevel = level;
    currentDynamic = dynamic;
    ALOGV("VrPerf: foveado nivel %d dinámico %d", level, dynamic);
}

/* ---- API ------------------------------------------------------------- */

void VrPerf_Init(XrInstance instance, XrSession session)
{
    gInstance = instance;
    gSession = session;
    initMetrics(instance, session);
    initFoveation(instance);
}

int VrPerf_Query(VrPerfMetrics *out)
{
    int ok = 0;
    memset(out, 0, sizeof(*out));
    out->appCpuMs = queryFloat(C_APP_CPU, &ok);
    out->appGpuMs = queryFloat(C_APP_GPU, &ok);
    out->compCpuMs = queryFloat(C_COMP_CPU, &ok);
    out->compGpuMs = queryFloat(C_COMP_GPU, &ok);
    out->latencyMs = queryFloat(C_LATENCY, &ok);
    out->cpuUtilAvg = queryFloat(C_CPU_AVG, &ok);
    out->cpuUtilWorst = queryFloat(C_CPU_WORST, &ok);
    out->gpuUtil = queryFloat(C_GPU_UTIL, &ok);
    float dropped = queryFloat(C_DROPPED, &ok);
    out->droppedFrames = dropped < 0 ? -1 : (int)dropped;
    out->foveationLevel = vrFoveationSupported ? (currentLevel < 0 ? 0 : currentLevel) : -1;
    out->foveationDynamic = currentDynamic < 0 ? 0 : currentDynamic;
    out->refreshHz = gAppState.currentDisplayRefreshRate;
    return ok;
}
