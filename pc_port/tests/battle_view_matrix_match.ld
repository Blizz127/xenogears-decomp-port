SECTIONS {
 .text 0x800BB844 : SUBALIGN(4) { *(.text) }
 /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.comment) *(.gnu.attributes) }
}
func_80021B14 = 0x80021B14;
VectorNormal = 0x80048D7C;
OuterProduct12 = 0x8004A480;
PushMatrix = 0x8004960C;
ApplyMatrix = 0x80049CEC;
PopMatrix = 0x800496AC;
