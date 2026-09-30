#!/usr/bin/env python3

import argparse
import os

from cdrom.cdxa import CdromXa
from psx.overlay import OverlayExtractor, parse_yaml


def find_disc_dir():
    start = os.path.abspath(os.path.dirname(__file__))
    candidate = f'{start}/disc'
    while start != '/':
        if os.path.isdir(candidate):
            return candidate
        start = os.path.dirname(start)
        candidate = f'{start}/disc'

    print("WARNING: failed to find disc dir, using current working directory as default instead")
    return os.getcwd()


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("bin_file", help="Path to the *.bin of the Xenogears bin/cue pair")
    parser.add_argument("--overlays", help="Path to the yaml describing the overlays to be extracted",
                        default=os.path.join(str(os.path.dirname(__file__)), "overlays.yaml"))
    parser.add_argument("--extract-to", help="Directory to extract overlays to",
                        default=find_disc_dir())
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def main():
    args = parse_arguments()
    overlays = parse_yaml(args.overlays)

    with CdromXa(args.bin_file) as xenogears:
        decoder = OverlayExtractor(xenogears)
        for ovl in overlays:
            if args.dry_run:
                print(f"Would try to extract {ovl.name} to {os.path.join(args.extract_to, ovl.name)}")
            else:
                decoder.extract(ovl, args.extract_to)
                print(f"Unpacked overlay: {os.path.join(args.extract_to, ovl.name)}")

    print("Extracted and decompressed all overlays")


if __name__ == '__main__':
    main()
