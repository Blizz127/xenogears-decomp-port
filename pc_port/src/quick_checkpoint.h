#ifndef XENO_PC_PORT_QUICK_CHECKPOINT_H
#define XENO_PC_PORT_QUICK_CHECKPOINT_H

#include <stddef.h>

void PcPort_QuickCheckpointRequestSave(void);
void PcPort_QuickCheckpointRequestLoad(void);
int PcPort_QuickCheckpointGetUiState(void);
void PcPort_QuickCheckpointSetFieldActive(int active);
int PcPort_QuickCheckpointFieldIsActive(void); /* TEST TOOLING: see .c */
int PcPort_QuickCheckpointFieldIsSafe(void);
unsigned PcPort_FieldLoopCount(void);               /* FieldMain loop passes */
int PcPort_QuickCheckpointPlayerPosition(int out[3]); /* whole units; 0 = no player */
/* The safety gate's inputs, for a rejected dev-menu action's log line. */
void PcPort_QuickCheckpointSafetyDescribe(char* out, size_t n);
int PcPort_QuickCheckpointRequestWarp(int map, int entrance);
int PcPort_QuickCheckpointPoll(void);
void PcPort_QuickCheckpointCommitLoad(void);
void PcPort_QuickCheckpointRestorePlayer(void);
/* On-screen quick save notice: current text, or NULL once it has expired. */
const char* PcPort_QuickCheckpointNotice(void);
void PcPort_QuickCheckpointNoticeSet(const char* text);

#endif
