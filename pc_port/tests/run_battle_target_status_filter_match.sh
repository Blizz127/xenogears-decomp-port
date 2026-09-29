#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-status-filter-match.XXXXXX)
BATTLE_MATCH_BUILD_DIR="$out" bash pc_port/tests/run_battle_attack_ring_match.sh
mips-linux-gnu-objcopy --only-section=.status_filter -O binary "$out/handler.elf" "$out/status_filter.bin"
perl - "$out" <<'PERL' | tee "$out/status-filter-comparison.log"
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
my $expected = substr($retail, 0x14618, 0xD8);
my $actual = read_file("$ARGV[0]/status_filter.bin");
my $different = 0;
for (my $offset=0; $offset<0xD8; $offset+=4) {
    my $word = $offset+4 <= length($actual) ? substr($actual,$offset,4) : "\0\0\0\0";
    next if $offset+4 <= length($actual) && $word eq substr($expected,$offset,4);
    printf "status filter offset %03X retail %08X candidate %08X\n", $offset,
        unpack('V',substr($expected,$offset,4)), unpack('V',$word);
    ++$different;
}
my $symbols = read_file("$ARGV[0]/symbols.txt");
my $extent = $symbols =~ /^80084108 000000d8 T func_80084108$/m;
my $ok = !$different && $extent && length($actual) == 0xD8;
printf "STATUS FILTER MATCH %s text=%d/54 size=%X retail=D8\n",
    $ok ? 'PASS' : 'FAIL', 54-$different, length($actual);
exit($ok ? 0 : 1);
PERL
