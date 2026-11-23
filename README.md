# Xenogears r7.21 PC port source

This source checkpoint covers the parts of the Windows and Linux ports cleared
for redistribution. It includes host file and checkpoint handling, platform
configuration, the opening-movie decoder and the Windows executable build tools.
Each platform directory contains a file manifest, source credits and license
notices. The gameplay code needed to rebuild and distribute the complete
executables is still under provenance review. Both tested builds are hybrid.

- [Windows x64 source](windows/README.md)
- [Linux x64 source](linux/README.md)
- [Changes, gameplay verification and known issues](CHANGELOG.md)
- [Prepare data from your own US Disc 1](tools/prepare-data/README.txt)

The data preparation tool runs with Python 3. It checks the raw disc image
and all six extracted files and preserves existing prepared directories.
For example, from this checkout:

```sh
python3 tools/prepare-data/prepare.py "/path/to/disc1.bin" "/path/to/xenogears-data"
```

On Windows, use `py -3` in place of `python3`. The tool was tested on
Linux and with Windows x64 Python under Wine. Xenogears and its trademarks
belong to their respective owners.
