# Synthetic menu heap preview

This fixture previews the mode-zero first-fit heap search without allocating,
then compares the selected address with the native `HeapAlloc` on a synthetic
heap, including an allocation that requires consolidation. It also checks out-of-range and backward links, insufficient space,
misalignment and models pending consolidation on copied metadata. The walk is
deliberately bounded. A
snapshot comparison detects changed traversed headers even if the selected
block stays the same; it cannot detect a mutation that is later reversed.

The preview is test-only. It does not validate live heap ownership, callback
mutation, allocation rollback, the full allocator or CPU parity, and it is not
wired into the battle menu. The test uses a hard timeout and a temporary build
directory; it does not access game saves.
