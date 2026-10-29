#ifndef XENO_PC_PORT_ROUTE_RECORDER_H
#define XENO_PC_PORT_ROUTE_RECORDER_H
/* Harness-only progression recorder, inert unless XENO_ROUTE_LOG=<file>.
 * See route_recorder.c. */
#ifdef __cplusplus
extern "C" {
#endif
int PcPort_RouteLogOn(void);
void PcPort_RouteLogInit(void);
void PcPort_RouteLogFieldLoad(int map);
void PcPort_RouteLogChangeField(int map, int entrance);
void PcPort_RouteLogScriptWrite(int address, int value);
void PcPort_RouteLogTrigger(int actor, int scriptId, int routine);
#ifdef __cplusplus
}
#endif
#endif
