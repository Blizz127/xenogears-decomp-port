# Xenogears for {platform}

Build `{version}` uses source revision `{commit}`. The SHA-256 of `{binary}` is `{binary_hash}`. The release's external `SHA256SUMS` identifies your downloaded archive. The `SHA256SUMS` inside this folder covers each installed file.

## Supply your Disc 1 image

Use Python 3 and your own US Disc 1 raw, single-track MODE2/2352 BIN (SLUS-00664). The supported image is 718,738,272 bytes, SHA-1 `12db8ccb93516c391630f046a143762337cc21f4`. Allow at least 720 MB for the prepared files. Extract the complete package, open a terminal in its folder, and replace the example image path below with your dump. The preparation tool verifies the image before extracting the game files.

{commands}

At the title screen, select **New Game** with the arrow keys and press **Z** to confirm. The port uses {save_dir} for save files. Continue, memory-card menus, Options and the inventory screen are unavailable in this preview.

This preview supports US Disc 1. In-game story cinematics are skipped. The startup intro uses a separate player. Disc 2 play and disc switching are unavailable. The default font is public-domain Jiskan 16. To use the original system font, set the `XENO_BIOS` environment variable to your own SCPH-5500 dump (524,288 bytes, SHA-256 `11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef`).

## Runtime and licenses

This hybrid port runs compiled host game code and retains a PlayStation CPU interpreter for uncovered paths. The release notes describe verified gameplay, known gaps and device coverage for this build. The startup command above disables the developer menu.

The project's original host code and disc preparation tool use the MIT license in `LICENSE`. Xenogears and its game assets belong to their respective owners. {notices}
