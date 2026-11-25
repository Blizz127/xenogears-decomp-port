# Xenogears PC port source

This source tree contains 135 independently written C functions for the host port. The shared
sources cover field scripts, graphics, dialogue, archive and controller services,
and sound. The Linux and Windows directories contain the previously reviewed
platform source. Building the complete game still requires code under
provenance review.

- [Shared native implementations](common/README.md)
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
