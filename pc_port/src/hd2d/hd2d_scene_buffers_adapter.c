/* Borrowed coverage metadata only. No GL calls or texture ownership. */
#include "hd2d_scene_buffers.h"
#include "hd2d_integration.h"
#include <string.h>

static PcPortHd2dSceneBuffers pending;
static int available;

void PcPort_Hd2dSceneBuffersReset(void)
{
    available=0;
    memset(&pending,0,sizeof pending);
}

void PcPort_Hd2dSceneBuffersOffer(const PcPortHd2dSceneBuffers *buffers)
{
    if(!PcPort_Hd2dEnabled())return;
    PcPort_Hd2dSceneBuffersReset();
    if(!buffers)return;
    pending=*buffers;
    available=1;
}

int PcPort_Hd2dSceneBuffersConsume(int path,int drawable_width,
                                 int drawable_height,XgHd2dFrame *frame)
{
    if(!PcPort_Hd2dEnabled())return 0;
    PcPortHd2dSceneBuffers buffers=pending;
    int had=available;
    PcPort_Hd2dSceneBuffersReset();
    if(!frame)return 0;

    frame->linear_depth_texture=frame->protection_texture=0;
    frame->linear_depth_verified=frame->protection_verified=0;
    frame->auxiliary_width=frame->auxiliary_height=0;
    frame->depth_mode=XG_HD2D_DEPTH_LINEAR;
    frame->protection_mode=XG_HD2D_PROTECTION_COVERAGE_BITS;
    frame->order_scale=0;
    if(!had||path!=0||!frame->field_active||
       drawable_width<=0||drawable_height<=0||
       buffers.drawable_width!=drawable_width||
       buffers.drawable_height!=drawable_height||
       frame->x<0||frame->y<0||frame->width<=0||frame->height<=0||
       frame->width>drawable_width||frame->height>drawable_height||
       frame->x>drawable_width-frame->width||
       frame->y>drawable_height-frame->height||
       buffers.viewport_x!=frame->x||buffers.viewport_y!=frame->y||
       buffers.viewport_width!=frame->width||
       buffers.viewport_height!=frame->height||
       buffers.flags!=PC_PORT_HD2D_SCENE_COVERAGE_VALID||
       !buffers.coverage_tex||!buffers.epoch)return 0;
    if(buffers.epoch!=PcPort_Hd2dSceneEpoch()||
       !PcPort_Hd2dOpeningActive()||
       buffers.field_id!=PcPort_Hd2dCurrentMap()||
       buffers.field_id!=frame->field_id)return 0;

    frame->protection_texture=buffers.coverage_tex;
    frame->protection_verified=1;
    frame->auxiliary_width=drawable_width;
    frame->auxiliary_height=drawable_height;
    return 1;
}
