#!/usr/bin/env bash
# Exact-match gate for the isolated handler and its relocated jump table.
set -euo pipefail
cd "$(dirname "$0")/../.."
out=${BATTLE_MATCH_BUILD_DIR:-$(mkdir -p scratchpad && mktemp -d scratchpad/battle-attack-match.XXXXXX)}
mkdir -p "$out"
echo "Evidence: $out"
cpp -P -undef -nostdinc -D_LANGUAGE_C -DSKIP_ASM -Iinclude \
    src/battle/main35.c -o "$out/main35.i"
tools/gcc-2.7.2-psx/cc1 -O2 -G8 -mips1 -mcpu=3000 -w \
    -funsigned-char -fpeephole -ffunction-cse -fpcc-struct-return \
    -fcommon -fverbose-asm -msoft-float -mgas -fgnu-linker -quiet \
    -o "$out/main35.s" "$out/main35.i"
# Separate this new body for independent retail placement without changing
# compiler-generated instructions or the existing text/table comparisons.
perl -pe 'if (/^\s*\.ent\s+func_80084B40\b/) { $ui=1; print ".section .target_ui,\"ax\",\@progbits\n"; }
          if ($ui && /^\s*\.rdata\s*$/) { $_ = ".section .target_ui_rodata,\"a\",\@progbits\n"; }
          if ($ui && /^\s*\.text\s*$/) { $_ = ".section .target_ui,\"ax\",\@progbits\n"; }
          if (/^\s*\.end\s+func_80084B40\b/) { $ui=0; $_ .= ".text\n"; }
          if (/^\s*\.ent\s+func_800841E0\b/) { print ".section .target_list,\"ax\",\@progbits\n"; }
          if (/^\s*\.ent\s+func_80083FF4\b/) { print ".section .eligibility,\"ax\",\@progbits\n"; }
          if (/^\s*\.ent\s+func_80084108\b/) { print ".section .status_filter,\"ax\",\@progbits\n"; }
          if (/^\s*\.ent\s+func_80084750\b/) { print ".section .actor_range,\"ax\",\@progbits\n"; }
          if (/^\s*\.ent\s+func_80084854\b/) { print ".section .direction,\"ax\",\@progbits\n"; }
          if (/^\s*\.end\s+func_800(?:841E0|83FF4|84108|84750|84854)\b/) { $_ .= ".text\n"; }' \
    "$out/main35.s" > "$out/main35.sections.s"
python3 tools/maspsx/maspsx.py --use-comm-section --run-assembler -EL \
    -Iinclude -Ibuild -O2 -G8 -march=r3000 -mtune=r3000 -no-pad-sections \
    -o "$out/main35.o" "$out/main35.sections.s"
mips-linux-gnu-ld -EL -T pc_port/tests/battle_attack_ring_match.ld \
    -o "$out/handler.elf" "$out/main35.o"
mips-linux-gnu-nm -S "$out/handler.elf" > "$out/symbols.txt"
rg -q '^8008115c 000001bc T func_8008115C$' "$out/symbols.txt" || {
    echo 'ATTACK RING MATCH FAIL entry/size'; exit 1;
}
mips-linux-gnu-objcopy --only-section=.text -O binary "$out/handler.elf" "$out/text.bin"
mips-linux-gnu-objcopy --only-section=.rodata -O binary "$out/handler.elf" "$out/rodata.bin"
perl - "$out" <<'PERL' | tee "$out/comparison.log"
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
sub read_file {
    open my $f, '<:raw', $_[0] or die "$_[0]: $!";
    local $/; return <$f>;
}
my $retail = read_file('disc/battle.bin');
my $text = substr($retail, 0x1166c, 0x1bc);
my $table = substr($retail, 0x520, 32);
die "retail text pin mismatch\n" unless sha256_hex($text) eq
    'c3639f06d1cb7097d5551490f63c8eea7431404779ce59cbcd108ae38238678f';
die "retail table pin mismatch\n" unless sha256_hex($table) eq
    'c9be78ab2802b5bc30ad1feede1534b2b4f85e9d4181dc1db277a4adfad9238a';
my $candidate = substr(read_file("$ARGV[0]/text.bin"), 0, 0x1bc);
my $candidate_table = substr(read_file("$ARGV[0]/rodata.bin"), 0, 32);
die "short candidate\n" unless length($candidate) == 0x1bc && length($candidate_table) == 32;
my $different = 0;
for (my $offset=0; $offset<0x1bc; $offset+=4) {
    next if substr($candidate,$offset,4) eq substr($text,$offset,4);
    printf "offset %03X retail %08X candidate %08X\n", $offset,
        unpack('V',substr($text,$offset,4)), unpack('V',substr($candidate,$offset,4));
    ++$different;
}
my $table_ok = $candidate_table eq $table;
printf "ATTACK RING MATCH %s text=%d/111 table=%s\n",
    (!$different && $table_ok ? 'PASS' : 'FAIL'), 111-$different,
    ($table_ok ? '8/8' : 'MISMATCH');
exit($different || !$table_ok ? 1 : 0);
PERL

# A second relocation places the new helper at its own retail address.
mips-linux-gnu-ld -EL -T pc_port/tests/battle_target_selection_match.ld \
    -o "$out/target.elf" "$out/main35.o"
mips-linux-gnu-nm -S "$out/target.elf" > "$out/target-symbols.txt"
rg -q '^80084a7c 000000c4 T func_80084A7C$' "$out/target-symbols.txt"
mips-linux-gnu-objcopy --only-section=.text -O binary "$out/target.elf" "$out/target-text.bin"
perl - "$out" <<'PERL' | tee "$out/target-comparison.log"
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
sub read_file {
    open my $f, '<:raw', $_[0] or die "$_[0]: $!";
    local $/; return <$f>;
}
my $retail = read_file('disc/battle.bin');
die "retail pin mismatch\n" unless sha256_hex($retail) eq
    '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291';
my $expected = substr($retail, 0x14f8c, 0xc4);
my $actual = substr(read_file("$ARGV[0]/target-text.bin"), 0x1bc, 0xc4);
die "short target\n" unless length($actual) == 0xc4;
my $different = 0;
for (my $offset=0; $offset<0xc4; $offset+=4) {
    next if substr($actual,$offset,4) eq substr($expected,$offset,4);
    printf "target offset %03X retail %08X candidate %08X\n", $offset,
        unpack('V',substr($expected,$offset,4)), unpack('V',substr($actual,$offset,4));
    ++$different;
}
printf "TARGET SELECTION MATCH %s text=%d/49\n", $different ? 'FAIL' : 'PASS', 49-$different;
exit($different ? 1 : 0);
PERL
