# Xenogears PC port source

This branch contains reviewed, redistributable work for the experimental
r7.21 PC port. Both Windows and Linux executables still use a PlayStation
CPU interpreter for parts of the game. Their public binary release is pending
independent replacements for upstream-authored code.

## Preparing your own game data

Run the data preparation tool with Python 3 and your own raw single-track
US Disc 1 image (SLUS-00664). Choose a new output directory. The tool
checks the disc identity and the SHA-256 of each extracted file. It also
verifies an existing prepared directory without overwriting it.

```sh
python3 tools/prepare-data/prepare.py "/path/to/disc1.bin" "/path/to/xenogears-data"
```

On Windows, use `py -3` in place of `python3`. Linux Python and Windows
x64 Python under Wine produced the same seven files. See
[disc preparation](tools/prepare-data/README.txt) for supported inputs,
checksums and disk requirements.

Xenogears and its trademarks belong to their respective owners. This
unofficial port requires the player's own disc. The MIT license applies
to the original preparation tool and accompanying documentation.
