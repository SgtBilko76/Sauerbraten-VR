#ifndef VRTELEMETRY_H
#define VRTELEMETRY_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Manda una línea de texto por UDP broadcast al puerto 5231 (ver VrTelemetry.c). */
void VrTelemetry_Send(const char *line);
void VrTelemetry_SendRaw(const void *data, size_t len);
/* 1 cuando el servidor del PC ha contestado y se manda en unicast. */
int VrTelemetry_HaveServer(void);
#ifdef __cplusplus
}
#endif
#endif
