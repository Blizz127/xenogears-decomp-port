# Build the standalone release archives

Use a Linux host with Python 3.9 or later, Git, and GNU `ar`. Run the command from the checkout containing the packaging profile and the exact game revision to package. Compile and link the Linux x86-64 game, Windows x64 game, and their respective static renderer archives from the same selected source commit. Retain the compiler dependency records and final linked files for both platforms.

Download `xenogears-v0.7.93-preview-linux-x86_64.tar.gz` and `xenogears-v0.7.93-preview-windows-x64.zip` from the [public support release](https://github.com/Blizz127/xenogears-decomp-port/releases/tag/v0.7.93-preview). `support.json` lists the download URLs, byte lengths, archive digests and the exact support members each package uses. This provides the platform libraries, their licenses, the corresponding OpenAL sources, and the disc preparation utility. The game executable in each package comes from the new build. The profile records the earlier executable digests to prevent accidental reuse.

Keep a checkout of [PsyCross](https://github.com/OpenDriver2/PsyCross) that contains commit `e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6`. The packager restores the published renderer source recipe from this checkout and compares its files with the actual compiler inputs. It retains the authenticated source recipe for unchanged inputs and their license notices. The source companion supplies the GPU, SPU and CD renderer headers and the data-access header from the selected game commit, including the prepared-media header on Windows. It records their paths alongside the measured compiler hashes. The packaged companion can reconstruct the measured inputs on a separate machine with that upstream commit.

## Producer records

Each platform's build receipt is a JSON object with these fields. Paths in file records can be absolute or relative to the receipt. Every file record has `path`, `bytes`, and `sha256`.

| Field | Value |
| --- | --- |
| `source` | Full Git commit of the fresh build |
| `tree` | Full Git tree of that commit |
| `binary` | File record for the stripped ELF or PE executable |
| `sdk` | File record for the static renderer archive actually linked |
| `renderer_inputs` | File record for the renderer compiler dependency report |

Generate the dependency report from the compiler dependency files and the objects that went into the archive. Preserve any wider compiler, link-map, source-origin and runtime evidence for release qualification.

| Dependency report field | Value |
| --- | --- |
| `platform`, `source_commit`, `source_tree`, `binary_sha256`, `sdk_sha256` | Platform (`linux` or `windows`) and identities of this build |
| `compiler_inputs` | Object mapping each project and renderer source/header path to its effective SHA-256. Vendor paths begin with `pc_port/extern/PsyCross/` |
| `members` | Array of object filenames contained in the static archive |
| `member_sha256` | Object mapping those same filenames to their object-content SHA-256 |
| `units` | Object mapping each filename to its complete list of project and vendor dependencies. Their union equals `compiler_inputs` |
| `source_files` | Mapping of edited vendor inputs to committed source paths. The GPU, SPU and CD headers each map from the vendor include/psx directory to its matching pc_port/renderer_headers/psx file. The data-access header and the Windows prepared-media header map to their own project paths. Their actual compiler bytes must match the Git blobs |

System and external dependency headers are qualified by the build producer. The producer must establish that the selected SDK and game were compiled together. The packaging checks reopen the files, match the archive members, replay the source recipe and verify the reported digests.

## Stage and verify

The output's parent directory must exist. Substitute your source commit, version, receipts and input locations. Keep the same inputs for readback.

```bash
args=(
  --commit "$(git rev-parse HEAD)" --version v0.7.94-preview
  --linux-build /build/linux/linux-link.json --windows-build /build/windows/windows-link.json
  --linux-support /downloads/xenogears-v0.7.93-preview-linux-x86_64.tar.gz
  --windows-support /downloads/xenogears-v0.7.93-preview-windows-x64.zip
  --psycross-upstream /sources/PsyCross --out /output/release
)
python3 -B pc_port/tools/package_public.py stage "${args[@]}"
python3 -B pc_port/tools/package_public.py verify "${args[@]}"
```

The output contains the Linux TAR.GZ, Windows ZIP and external `SHA256SUMS`. Each archive contains its own checksums and the instructions for supplying an owned disc and optional original BIOS. `verify` reconstructs both archives and checks their full bytes and the exact top-level file set. Run source/content scans and gameplay smoke tests on those exact archives before public release.
