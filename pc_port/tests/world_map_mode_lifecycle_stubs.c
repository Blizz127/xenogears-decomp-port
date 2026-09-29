/* Link-only stubs for world-map mode entry/exit points that focused
 * lifecycle tests reference (via world_map_main_loop_71034.c dispatch)
 * but never traverse. Traversal aborts loudly instead of silently
 * passing: link the real world_map_modeNN_lifecycle.c TU for any mode a
 * test actually enters. */
#include <stdlib.h>

int __attribute__((weak)) wm_80078A60(void) { abort(); }
void __attribute__((weak)) wm_80078D24(void) { abort(); }
int __attribute__((weak)) wm_8007BF50(void) { abort(); }
void __attribute__((weak)) wm_8007C260(void) { abort(); }
int __attribute__((weak)) wm_8007FF70(void) { abort(); }
void __attribute__((weak)) wm_80080218(void) { abort(); }
int __attribute__((weak)) wm_8007A5DC(void) { abort(); }
void __attribute__((weak)) wm_8007A8AC(void) { abort(); }
int __attribute__((weak)) wm_8007D918(void) { abort(); }
void __attribute__((weak)) wm_8007DCE0(void) { abort(); }
int __attribute__((weak)) wm_80080D00(void) { abort(); }
void __attribute__((weak)) wm_8008106C(void) { abort(); }
int __attribute__((weak)) wm_80082324(void) { abort(); }
void __attribute__((weak)) wm_800826B4(void) { abort(); }
int __attribute__((weak)) wm_8008355C(void) { abort(); }
void __attribute__((weak)) wm_800837DC(void) { abort(); }
