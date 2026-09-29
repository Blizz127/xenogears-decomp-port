/* Isolated function/table relocation, not a full overlay link. */
SECTIONS {
  .text 0x8008115C : SUBALIGN(4) { *(.text) }
  .rodata 0x80070010 : SUBALIGN(4) { *(.rodata) }
  .target_list 0x800841E0 : SUBALIGN(4) { *(.target_list) }
  .eligibility 0x80083FF4 : SUBALIGN(4) { *(.eligibility) }
  .status_filter 0x80084108 : SUBALIGN(4) { *(.status_filter) }
  .actor_range 0x80084750 : SUBALIGN(4) { *(.actor_range) }
  .direction 0x80084854 : SUBALIGN(4) { *(.direction) }
  .target_ui 0x80084B40 : SUBALIGN(4) { *(.target_ui) }
  .target_ui_rodata 0x80070210 : SUBALIGN(4) { *(.target_ui_rodata) }
  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.comment) *(.gnu.attributes) }
}
D_800C3E29 = 0x800C3E29;
D_800C3D64 = 0x800C3D64;
func_80089C08 = 0x80089C08;
func_800BC404 = 0x800BC404;
func_800BCD98 = 0x800BCD98;
func_800716D8 = 0x800716D8;
ratan2 = 0x8004B32C;
D_800C3EC0 = 0x800C3EC0;
D_800C3EAC = 0x800C3EAC;
D_800C3E90 = 0x800C3E90;
D_800D3274 = 0x800D3274;
D_800C3EB4 = 0x800C3EB4;
D_800CCD34 = 0x800CCD34;
D_800D2DCC = 0x800D2DCC;
D_800C3EB7 = 0x800C3EB7;
D_800D32A1 = 0x800D32A1;
D_800D3364 = 0x800D3364;
D_800CCD64 = 0x800CCD64;
D_800CCE08 = 0x800CCE08;
D_800C3EBE = 0x800C3EBE;
D_800D2D5C = 0x800D2D5C;
D_800D2D70 = 0x800D2D70;
D_800D3014 = 0x800D3014;
D_800D366C = 0x800D366C;
func_80077698 = 0x80077698;
func_80084A7C = 0x80084A7C;
func_80087A38 = 0x80087A38;
func_8008AA74 = 0x8008AA74;
