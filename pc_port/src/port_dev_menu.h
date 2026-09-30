#ifndef XENO_PORT_DEV_MENU_H
#define XENO_PORT_DEV_MENU_H
#ifdef __cplusplus
extern "C" {
#endif
enum PcPortDevKey { PC_DEV_OPEN, PC_DEV_UP, PC_DEV_DOWN, PC_DEV_ACCEPT, PC_DEV_BACK };
void PcPort_DevMenuInit(void);
int PcPort_DevMenuEnabled(void);
int PcPort_DevMenuOpen(void);
int PcPort_DevMenuKey(int key, int pressed);
int PcPort_DevMenuFilterPad(unsigned held);
int PcPort_DevMenuCursor(void);
int PcPort_DevMenuRows(void);
const char *PcPort_DevMenuTitle(void);
const char *PcPort_DevMenuRow(int row);
const char *PcPort_DevMenuStatus(void);
int PcPort_DevMenuAction(const char *command);
#ifdef __cplusplus
}
#endif
#endif
