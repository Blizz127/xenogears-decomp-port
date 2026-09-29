/* Isolated BC404 only; neighboring text is deliberately parked. */
SECTIONS {
  .text 0x800BD000 : SUBALIGN(4) { *(.text) }
  .display 0x800BC404 : SUBALIGN(4) { *(.display) }
  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.comment) *(.gnu.attributes) }
}
D_800C367C = 0x800C367C;
D_800C37C8 = 0x800C37C8;
D_800C3CDC = 0x800C3CDC;
D_80059454 = 0x80059454;
D_800C3740 = 0x800C3740;
func_800BC2F0 = 0x800BC2F0;
func_800BC460 = 0x800BC460;
D_800C3CC0 = 0x800C3CC0;
D_800C3CBC = 0x800C3CBC;
D_800C3680 = 0x800C3680;
D_800C3684 = 0x800C3684;
D_800D30A0 = 0x800D30A0;
D_8006F99C = 0x8006F99C;
D_8006F9AC = 0x8006F9AC;
func_80021B14 = 0x80021B14;
VectorNormal = 0x80048D7C;
OuterProduct12 = 0x8004A480;
PushMatrix = 0x8004960C;
ApplyMatrix = 0x80049CEC;
PopMatrix = 0x800496AC;
