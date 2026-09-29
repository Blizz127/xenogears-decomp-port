#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-eligibility-match.XXXXXX)
BATTLE_MATCH_BUILD_DIR="$out" bash pc_port/tests/run_battle_attack_ring_match.sh
mips-linux-gnu-objcopy --only-section=.eligibility -O binary "$out/handler.elf" "$out/eligibility.bin"
perl - "$out" <<'PERL' | tee "$out/eligibility-comparison.log"
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
my $expected = substr($retail, 0x14504, 0x114);
my $actual = read_file("$ARGV[0]/eligibility.bin");
my $different = 0;
for (my $offset=0; $offset<0x114; $offset+=4) {
    my $word = $offset+4 <= length($actual) ? substr($actual,$offset,4) : "\0\0\0\0";
    next if $offset+4 <= length($actual) && $word eq substr($expected,$offset,4);
    printf "eligibility offset %03X retail %08X candidate %08X\n", $offset,
        unpack('V',substr($expected,$offset,4)), unpack('V',$word);
    ++$different;
}
my $symbols = read_file("$ARGV[0]/symbols.txt");
my $extent = $symbols =~ /^80083ff4 00000114 T func_80083FF4$/m;
my $ok = !$different && $extent && length($actual) == 0x114;
printf "ELIGIBILITY MATCH %s text=%d/69 size=%X retail=114\n",
    $ok ? 'PASS' : 'FAIL', 69-$different, length($actual);
exit($ok ? 0 : 1);
PERL
