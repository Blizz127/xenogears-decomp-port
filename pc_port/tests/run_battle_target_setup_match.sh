#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-setup-match.XXXXXX)
echo "Evidence: $out"
cpp -P -undef -nostdinc -D_LANGUAGE_C -DSKIP_ASM -Iinclude src/battle/mainc114.c -o "$out/body.i"
# gears.toml BattleCdk applies to battle/mainc*, including mainc114.
tools/gcc-2.7.2-cdk-psx/cc1 -O2 -G8 -mips1 -mcpu=3000 -w \
    -funsigned-char -fpeephole -ffunction-cse -fpcc-struct-return \
    -fcommon -fverbose-asm -msoft-float -mgas -fgnu-linker -quiet \
    -o "$out/body.s" "$out/body.i"
perl -pe 'if (/^\s*\.ent\s+func_800BC2F0\b/) { print ".section .setup,\"ax\",\@progbits\n"; }
          if (/^\s*\.end\s+func_800BC2F0\b/) { $_ .= ".text\n"; }' "$out/body.s" > "$out/sections.s"
python3 tools/maspsx/maspsx.py --dont-expand-li --use-comm-section --run-assembler -EL \
    -Iinclude -Ibuild -O2 -G8 -march=r3000 -mtune=r3000 -no-pad-sections \
    -o "$out/body.o" "$out/sections.s"
mips-linux-gnu-ld -EL -T pc_port/tests/battle_target_setup_match.ld -o "$out/body.elf" "$out/body.o"
mips-linux-gnu-nm -S "$out/body.elf" > "$out/symbols.txt"
mips-linux-gnu-objcopy --only-section=.setup -O binary "$out/body.elf" "$out/setup.bin"
perl - "$out" <<'PERL' | tee "$out/comparison.log"
use strict; use warnings; use Digest::SHA qw(sha256_hex);
sub read_file { open my $f,'<:raw',$_[0] or die $!; local $/; return <$f>; }
my $retail=read_file('disc/battle.bin');
die 'retail pin mismatch' unless sha256_hex($retail) eq
    '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291';
my $actual=read_file("$ARGV[0]/setup.bin"); my $different=0;
for (my $o=0;$o<0x108;$o+=4) {
    my $expected=substr($retail,0x4C800+$o,4);
    my $word=$o+4<=length($actual) ? substr($actual,$o,4) : "\0\0\0\0";
    next if $o+4<=length($actual) && $word eq $expected;
    printf "SETUP offset %03X retail %08X candidate %08X\n",$o,unpack('V',$expected),unpack('V',$word);
    $different++;
}
my $symbols=read_file("$ARGV[0]/symbols.txt");
sub accepts {
    my ($bytes,$syms,$expected)=@_;
    return length($bytes)==0x108 && $bytes eq $expected &&
        $syms =~ /^800bc2f0 00000108 T func_800BC2F0$/m;
}
my $expected_text=substr($retail,0x4C800,0x108);
my $ok=accepts($actual,$symbols,$expected_text);
printf "TARGET SETUP MATCH %s text=%d/66 size=%X retail=108\n",$ok?'PASS':'FAIL',66-$different,length($actual);
if ($ok) {
    my $bad=$actual; substr($bad,0,1)=chr(ord(substr($bad,0,1))^1);
    die 'content mutant survived' if accepts($bad,$symbols,$expected_text);
    die 'size mutant survived' if accepts(substr($actual,0,-4),$symbols,$expected_text);
    my $wrong=$symbols; $wrong =~ s/^800bc2f0 00000108 T/800bc2f4 00000108 T/m or die 'symbol mutation did not apply';
    die 'symbol mutant survived' if accepts($actual,$wrong,$expected_text);
    print "SETUP match content/size/symbol negative controls PASS\n";
}
exit($ok?0:1);
PERL
