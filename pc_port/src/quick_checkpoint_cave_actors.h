#ifndef XENO_QUICK_CHECKPOINT_CAVE_ACTORS_H
#define XENO_QUICK_CHECKPOINT_CAVE_ACTORS_H

#include "quick_checkpoint_file.h"

int PcPort_CaveActorsCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES]);
int PcPort_CaveActorsRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES]);

/* BAL1 preserves the idle merchant after the sensor briefing. */
int PcPort_BalActorCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES]);
int PcPort_BalActorRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES]);

int PcPort_FollowerCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES]);
int PcPort_FollowerRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES]);
int PcPort_FollowerGroupAtPlayer(void);
int PcPort_ForestRegroupAtPlayer(void);
int PcPort_StreetFollowersCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES]);
int PcPort_StreetFollowersRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES]);
int PcPort_StreetFollowersRegroup(void);

/* The forest follow camera reduces the shared step by two every fourth tick. */
typedef struct {
    uint8_t step, phase;
} PcPortForestCameraTiming;

/* Elly and the movement trail after the Disc 2 workshop handback. */
int PcPort_ForestPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES], PcPortForestCameraTiming timing);
int PcPort_ForestPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES], PcPortForestCameraTiming *timing);

/* Elly on the ordinary workshop floor before the laboratory meeting. */
int PcPort_TauraWorkshopPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES]);
int PcPort_TauraWorkshopPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES]);

/* Elly in the laboratory after the second Taura meeting. */
int PcPort_TauraPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES]);
int PcPort_TauraPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES]);
int PcPort_TauraPartyRegroup(void);

/* DAZ1 reserves this optional region for the post-dialogue Dazil party. */
enum { PC_PORT_DAZIL_PARTY_BYTES = 1800 };
int PcPort_DazilPartyCapture(uint8_t out[PC_PORT_DAZIL_PARTY_BYTES]);
int PcPort_DazilPartyRestore(const uint8_t in[PC_PORT_DAZIL_PARTY_BYTES], int apply);

/* EST1 owns the same-sized companion payload. Its camera occupies the tail. */
int PcPort_EstateActorsCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES]);
int PcPort_EstateActorsRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES], int apply);

#endif
