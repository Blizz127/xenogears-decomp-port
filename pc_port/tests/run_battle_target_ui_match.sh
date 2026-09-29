#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-ui-match.XXXXXX)
BATTLE_MATCH_BUILD_DIR="$out" bash pc_port/tests/run_battle_attack_ring_match.sh
mips-linux-gnu-objcopy --only-section=.target_ui -O binary "$out/handler.elf" "$out/ui.bin"
mips-linux-gnu-objcopy --only-section=.target_ui_rodata -O binary "$out/handler.elf" "$out/ui-table.bin"
perl - "$out" <<'PERL' | tee "$out/ui-comparison.log"
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
sub read_file { open my $f,'<:raw',$_[0] or die $!; local $/; return <$f>; }
my $retail=read_file('disc/battle.bin');
die 'retail pin mismatch' unless sha256_hex($retail) eq
    '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291';
my $actual=read_file("$ARGV[0]/ui.bin");
my $table=read_file("$ARGV[0]/ui-table.bin");
my $different=0;
for (my $o=0;$o<0x1E8;$o+=4) {
    my $expected=substr($retail,0x15050+$o,4);
    my $word=$o+4<=length($actual) ? substr($actual,$o,4) : "\0\0\0\0";
    next if $o+4<=length($actual) && $word eq $expected;
    printf "UI offset %03X retail %08X candidate %08X\n",$o,unpack('V',$expected),unpack('V',$word);
    $different++;
}
my $table_ok=$table eq substr($retail,0x720,32);
my $symbols=read_file("$ARGV[0]/symbols.txt");
my $ok=!$different && length($actual)==0x1E8 && $table_ok &&
    $symbols =~ /^80084b40 000001e8 T func_80084B40$/m;
printf "TARGET UI MATCH %s text=%d/122 size=%X retail=1E8 table=%s\n",
    $ok?'PASS':'FAIL',122-$different,length($actual),$table_ok?'8/8':'MISMATCH';
exit($ok?0:1);
PERL
