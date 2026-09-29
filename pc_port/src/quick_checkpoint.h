#ifndef XENO_PC_PORT_QUICK_CHECKPOINT_H
#define XENO_PC_PORT_QUICK_CHECKPOINT_H

void PcPort_QuickCheckpointRequestSave(void);
void PcPort_QuickCheckpointRequestLoad(void);
int PcPort_QuickCheckpointGetUiState(void);
void PcPort_QuickCheckpointSetFieldActive(int active);
int PcPort_QuickCheckpointFieldIsActive(void); /* TEST TOOLING: see .c */
int PcPort_QuickCheckpointPoll(void);
void PcPort_QuickCheckpointCommitLoad(void);
void PcPort_QuickCheckpointRestorePlayer(void);

#endif
