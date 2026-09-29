#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
candidate=${1:-src/battle/mainc114.c}
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-view-matrix-match.XXXXXX)
echo "Evidence: $out"
cpp -P -undef -nostdinc -D_LANGUAGE_C -DSKIP_ASM -Iinclude "$candidate" -o "$out/body.i"
tools/gcc-2.7.2-cdk-psx/cc1 -O2 -G8 -mips1 -mcpu=3000 -w \
    -funsigned-char -fpeephole -ffunction-cse -fpcc-struct-return \
    -fcommon -fverbose-asm -msoft-float -mgas -fgnu-linker -quiet \
    -o "$out/body.s" "$out/body.i"
perl -ne 'print if /^\s*\.globl\s+func_800BB844\b/;
          if (/^\s*\.ent\s+func_800BB844\b/) { $keep=1; print ".text\n"; }
          print if $keep;
          $keep=0 if /^\s*\.end\s+func_800BB844\b/;' "$out/body.s" > "$out/isolated.s"
test -s "$out/isolated.s" || { echo 'FAIL: BB844 C body absent'; exit 1; }
python3 tools/maspsx/maspsx.py --dont-expand-li --use-comm-section --run-assembler -EL \
    -Iinclude -Ibuild -O2 -G8 -march=r3000 -mtune=r3000 -no-pad-sections \
    -o "$out/body.o" "$out/isolated.s"
mips-linux-gnu-ld -EL -T pc_port/tests/battle_view_matrix_match.ld -o "$out/body.elf" "$out/body.o"
mips-linux-gnu-nm -S "$out/body.elf" > "$out/symbols.txt"
mips-linux-gnu-objcopy --only-section=.text -O binary "$out/body.elf" "$out/body.bin"
perl - "$out" <<'PERL' | tee "$out/comparison.log"
use strict; use warnings; use Digest::SHA qw(sha256_hex);
sub read_file { open my $f,'<:raw',$_[0] or die $!; local $/; return <$f>; }
my $retail=read_file('disc/battle.bin');
die 'retail pin mismatch' unless sha256_hex($retail) eq
 '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291';
my $expected=substr($retail,0x4BD54,0x190);
my $actual=read_file("$ARGV[0]/body.bin");
my $symbols=read_file("$ARGV[0]/symbols.txt");
sub accepts {
 my ($a,$s,$e)=@_;
 return length($a)==0x190 && $a eq $e && $s =~ /^800bb844 00000190 T func_800BB844$/m;
}
my $equal=0;
for (my $o=0;$o<0x190;$o+=4) {
 my $e=substr($expected,$o,4); my $a=substr($actual,$o,4);
 if ($e eq $a) { $equal++; next; }
 printf "offset %03X retail %08X candidate %s\n",$o,unpack('V',$e),length($a)==4?sprintf('%08X',unpack('V',$a)):'MISSING';
}
my $ok=accepts($actual,$symbols,$expected);
printf "VIEW MATRIX MATCH %s text=%d/100 size=%X retail=190\n",$ok?'PASS':'FAIL',$equal,length($actual);
# Exercise the acceptance predicate even while the candidate is nonmatching.
my $fixture_symbols="800bb844 00000190 T func_800BB844\n";
die 'baseline rejected' unless accepts($expected,$fixture_symbols,$expected);
my $bad=$expected; substr($bad,0,1)=chr(ord(substr($bad,0,1))^1);
die 'content mutant survived' if accepts($bad,$fixture_symbols,$expected);
die 'size mutant survived' if accepts(substr($expected,0,-4),$fixture_symbols,$expected);
die 'symbol mutant survived' if accepts($expected,'800bb848 00000190 T func_800BB844',$expected);
print "VIEW MATRIX content/size/symbol negative controls PASS\n";
exit($ok?0:1);
PERL
