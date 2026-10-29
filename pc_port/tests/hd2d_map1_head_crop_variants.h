#ifndef HD2D_MAP1_HEAD_CROP_VARIANTS_H
#define HD2D_MAP1_HEAD_CROP_VARIANTS_H
/* Predetermined paired source rectangles in unchanged original RGBA.
 * p5 covers left head/hair, p2 the adjoining right upper head; coherent source
 * positions are varied together. These are test candidates, not approved art. */
typedef struct { const char *name; unsigned short p2[4],p5[4]; } Hd2dMap1HeadVariant;
static const Hd2dMap1HeadVariant hd2d_map1_head_variants[8]={
 {"map1_original_pair",{172,37,51,48},{100,37,78,120}},
 {"map1_left_margin2",{172,37,51,48},{98,37,80,120}},
 {"map1_pair_right2",{174,37,51,48},{102,37,78,120}},
 {"map1_pair_left2",{170,37,51,48},{98,37,78,120}},
 {"map1_top_margin2",{172,35,51,50},{100,35,78,122}},
 {"map1_pair_down2",{172,39,51,48},{100,39,78,120}},
 {"map1_wider_pair",{170,37,53,48},{98,37,80,120}},
 {"map1_narrower_pair",{174,37,49,48},{102,37,76,120}}
};
#endif
