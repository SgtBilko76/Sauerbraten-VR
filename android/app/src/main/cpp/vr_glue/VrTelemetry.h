#ifndef VRTELEMETRY_H
#define VRTELEMETRY_H
#ifdef __cplusplus
extern "C" {
#endif
/* Manda una línea de texto por UDP broadcast al puerto 5231 (ver VrTelemetry.c). */
void VrTelemetry_Send(const char *line);
#ifdef __cplusplus
}
#endif
#endif
