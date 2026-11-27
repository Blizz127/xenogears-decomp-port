#ifndef XENO_MENU_CATEGORY_REFRESH_H
#define XENO_MENU_CATEGORY_REFRESH_H

/* Item CC720 indexes four cursor/label pairs per mode; Gear CDA0C passes
 * its two active pairs after verifying that the pointer coordinates agree. */
void PcPort_ShopMenuRefreshCategory(const int *pairs);

#endif
