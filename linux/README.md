# Xenogears r7.21 Linux host source

This source snapshot covers independently cleared portions of the Linux PC port at `f342cad91b871ddf20afa49cb209977315a5dd65`, used for `xeno-r7.21-dev-f342cad91b`. It includes platform settings and file handling, checkpoint transport, the opening-movie decoder, font tooling, and an independent disc-preparation utility. The Windows snapshot also contains the native PE build and inspection tools.

The frozen game executable cannot be built from this snapshot. Its gameplay sources and runtime dependencies include code with unresolved redistribution provenance. Both frozen builds execute compiled host code and retain a PS1 CPU fallback.

To prepare the files used by r7.21, supply your own raw US Disc 1 image. The included standard-library utility verified all seven outputs under Linux and Windows x64 Python under Wine. [Data preparation](prepare-data/README.txt)

`SOURCE-MANIFEST.json` gives the purpose, origin and SHA-256 of each included file. [Distribution notice](NOTICE.md) · [Third-party credits](CREDITS.md)
