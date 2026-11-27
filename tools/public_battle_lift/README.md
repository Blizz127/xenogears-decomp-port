# Build the native battle modules

These three host translation units implement 101 routines from the US battle overlay. Generate them locally from a copy of Disc 1 that you own. The project calls the same public lifting tool for the command menu, frame, and status modules. Python 3.8 or later and the project's pinned Splat installation are required.

Keep this directory underneath `tools/public_battle_lift`. From the project root, extract the overlay and prepare a fresh split directory. The preparation tool copies the five public inputs and your local battle overlay into an isolated directory, so the contents of an existing matching-source checkout do not affect the result. The pinned public configuration was verified with splat64 0.33.2 and spimdisasm 1.33.0.

```sh
python3 tools/scripts/extract_overlays.py "/path/to/your/Disc 1.bin" --extract-to build/owned-overlays
python3 tools/public_battle_lift/prepare_split.py --project . \
    --overlay build/owned-overlays/battle.bin --out build/battle-split
```

Run the following in the project checkout. The split directory already contains the pinned public lifter and the two generated linker lists. Point the native compiler at the generated units using their normal project object names.

```sh
for group in cmdmenu frame status; do
    python3 tools/public_battle_lift/generate.py --root build/battle-split --group "$group" \
        --out "build/generated/src/battle/mainasm_${group}_port.c"
done
```

Each group verifies the overlay, split configuration, symbol catalogs, lifter and selected listings. Keep generated assembly and C local. When a check fails, the driver preserves the previous output file.

## Source and attribution

Blizz127 and contributors authored the tool and the existing generic lifting interface as PC-port development tools. The public project distributes its own scripts under the MIT license. The unchanged generic lifter is Git blob `5b884637bba2d9e19ed75d75083e080188591d62`. [Public lifter](https://github.com/Blizz127/xenogears-decomp-port/blob/7c729166cbcf076cd5a30872a7022171fc7f6bbb/tools/scripts/battle_lift_asm.py) · [Project license](https://github.com/Blizz127/xenogears-decomp-port/blob/7c729166cbcf076cd5a30872a7022171fc7f6bbb/LICENSE)

The manifest records native entry addresses, argument counts and return widths from the project's working native-port ABI. Its digests identify the 101 listings produced by a fresh split of the player's US overlay and the public split configuration. The driver implements signature narrowing and relative switch labels over the public lifter's output. The project's license covers its tools. Square Enix retains the rights to the original game material. [Public split configuration](https://github.com/Blizz127/xenogears-decomp-port/blob/7c729166cbcf076cd5a30872a7022171fc7f6bbb/config/battle.yaml)
