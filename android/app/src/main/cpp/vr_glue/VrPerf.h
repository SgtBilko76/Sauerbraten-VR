/* Métricas de rendimiento (XR_META_performance_metrics) y foveado fijo
 * (XR_FB_foveation + XR_FB_foveation_configuration + XR_FB_swapchain_update_state)
 * para SauVREngine. Todo es opcional: si el runtime no trae la extensión, las
 * funciones no hacen nada y el motor sigue igual.
 *
 * Flujo:
 *   - TBXR_InitialiseOpenXR() pide las extensiones y guarda en
 *     vrPerfMetricsSupported / vrFoveationSupported si están.
 *   - ovrFramebuffer_Create() encadena VrPerf_FoveationCreateInfo() en el
 *     XrSwapchainCreateInfo (bit SCALED_BIN, el que usa GLES en Quest).
 *   - TBXR_InitRenderer() llama a VrPerf_Init() y VrPerf_SetFoveation().
 *   - El motor (rendergl.cpp, panel vrstats) lee VrPerf_Query() a través de
 *     androidbridge.h y cambia el foveado con android_vr_set_foveation().
 */
#ifndef VRPERF_H
#define VRPERF_H

#include <stdbool.h>
#include <openxr/openxr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* ms; -1 si el runtime no da ese contador */
    float appCpuMs, appGpuMs, compCpuMs, compGpuMs, latencyMs;
    /* % ; -1 si no disponible */
    float cpuUtilAvg, cpuUtilWorst, gpuUtil;
    /* frames que el compositor tuvo que repetir desde el último query; -1 si no */
    int droppedFrames;
    /* estado aplicado */
    int foveationLevel, foveationDynamic;
    float refreshHz;
} VrPerfMetrics;

/* Foveado por defecto al arrancar: alto y dinámico (el runtime lo baja cuando
 * la GPU tiene margen). El motor lo cambia con /vrfoveation y /vrfoveationdynamic. */
#define VR_DEFAULT_FOVEATION_LEVEL 3
#define VR_DEFAULT_FOVEATION_DYNAMIC 1

extern bool vrPerfMetricsSupported;
extern bool vrFoveationSupported; /* las tres extensiones de foveado */

void VrPerf_Init(XrInstance instance, XrSession session);
/* Rellena out; devuelve 1 si al menos un contador es válido. Barato: llamar
 * unas pocas veces por segundo, no por frame. */
int VrPerf_Query(VrPerfMetrics *out);

/* Devuelve el struct a encadenar en XrSwapchainCreateInfo.next, o NULL. */
const void *VrPerf_FoveationCreateInfo(void);
/* level 0..3 (XrFoveationLevelFB), dynamic 0/1. Aplica a los dos ojos. */
void VrPerf_SetFoveation(int level, int dynamic);

#ifdef __cplusplus
}
#endif

#endif
