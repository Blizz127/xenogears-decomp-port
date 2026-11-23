Xenogears r7.21 data preparation

You need your own US Xenogears Disc 1 (SLUS-00664), as a raw single-track
MODE2/2352 BIN of 718,738,272 bytes, and Python 3.6 or newer. The input
SHA-1 is 12db8ccb93516c391630f046a143762337cc21f4. The tool contains
sector locations, sizes and file digests for that pressing. A BIOS is optional.

Pick a new data directory whose parent already exists and has 720 MB free.
On Linux, run

  python3 prepare.py "/path/to/Disc 1.bin" "/path/to/Xenogears Data"
  ./xenogears.sh "/path/to/Xenogears Data"

On Windows, from PowerShell, run

  py -3 .\prepare.py 'D:\My Discs\Disc 1.bin' 'D:\Games\Xenogears Data'
  $env:XENOGEARS_DATA = 'D:\Games\Xenogears Data'

Then start runtime\xeno-port.exe from your writable profile directory.

The tool reads your image and verifies it while copying. It checks every
extracted file before publishing the local data directory. Running the same
command again verifies all seven outputs. A mismatch or incomplete existing
directory produces an error. Contents, saves and settings in existing
directories are preserved. Keep the BIN, extracted files and saves private.
