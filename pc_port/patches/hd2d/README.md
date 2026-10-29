# HD-2D lane PsyCross patches

`pc_port/build_port.sh` applies every `*.patch` here in byte-sorted order,
after all other PsyCross patches (including `psycross_hd2d_hooks.patch`).
Each patch should add the literal sentinel comment `/* _hd2d_<basename> */`
(basename without `.patch`); an already-applied patch is also skipped by a
reverse check. Generate with `diff -u` against the tree the earlier patches
produce, and check that it replays on a fresh clone (remove `pc_port/extern`
and rebuild).
