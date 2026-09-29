#!/usr/bin/env bash
# Deliberately fails until all218 retail words and symbol extent match.
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-list-match.XXXXXX)
BATTLE_MATCH_BUILD_DIR="$out" bash pc_port/tests/run_battle_attack_ring_match.sh
mips-linux-gnu-objcopy --only-section=.target_list -O binary "$out/handler.elf" "$out/list.bin"
perl - "$out" <<'PERL' | tee "$out/list-comparison.log"
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
my $expected = substr($retail, 0x146f0, 0x368);
my $actual = read_file("$ARGV[0]/list.bin");
my $different = 0;
my $prefix = 0;
# Retail basic-block phase boundaries, not independently accepted subsets.
my @phases = (
    ['init/eligibility', 0, 0x11c, 0],
    ['partition/append', 0x11c, 0x1f0, 0],
    ['rank selection', 0x1f0, 0x33c, 0],
    ['return', 0x33c, 0x368, 0],
);
for (my $offset=0; $offset<0x368; $offset+=4) {
    my $word = $offset+4 <= length($actual) ? substr($actual,$offset,4) : "\0\0\0\0";
    if ($offset+4 <= length($actual) && $word eq substr($expected,$offset,4)) {
        $prefix += 4 if $prefix == $offset;
        for my $phase (@phases) {
            ++$phase->[3] if $offset >= $phase->[1] && $offset < $phase->[2];
        }
        next;
    }
    printf "list offset %03X retail %08X candidate %08X\n", $offset,
        unpack('V',substr($expected,$offset,4)), unpack('V',$word);
    ++$different;
}
my $symbols = read_file("$ARGV[0]/symbols.txt");
my $extent = $symbols =~ /^800841e0 00000368 T func_800841E0$/m;
my $ok = !$different && $extent && length($actual) == 0x368;
printf "TARGET LIST MATCH %s text=%d/218 size=%X retail=368\n",
    $ok ? 'PASS' : 'FAIL', 218-$different, length($actual);
printf "TARGET LIST contiguous matching prefix=%03X bytes\n", $prefix;
for my $phase (@phases) {
    printf "TARGET LIST phase %s offsets=%03X..%03X words=%d/%d\n",
        $phase->[0], $phase->[1], $phase->[2]-1, $phase->[3],
        ($phase->[2]-$phase->[1])/4;
}
exit($ok ? 0 : 1);
PERL
