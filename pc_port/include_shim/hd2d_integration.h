#ifndef PC_PORT_HD2D_INTEGRATION_H
#define PC_PORT_HD2D_INTEGRATION_H
#include "../include/xg_plat/hd2d.h"
int PcPort_Hd2dEnabled(void);
int PcPort_Hd2dOpeningActive(void);
int PcPort_Hd2dCurrentMap(void);
void PcPort_Hd2dSetEnabled(int enabled);
int PcPort_Hd2dSetIntensity(int effect, float intensity);
int PcPort_Hd2dSetOpeningMaps(const int *maps, unsigned count);
const XgHd2dConfig *PcPort_Hd2dConfiguration(void);
/* Optional producer: return 1 only for current-frame verified inputs. Its
 * absence never fabricates depth or coverage. Integration must agree this ABI. */
int PcPort_Hd2dSceneInputs(int path, int width, int height, XgHd2dFrame *out);
#endif
