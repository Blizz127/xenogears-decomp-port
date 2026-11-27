# GitHub Wiki staging area

The project wiki pages live in `docs/wiki/`. The decompile and port totals in `Home.md`, `Decompile-Status.md` and `Port-Status.md` should match the tables in the repository README. [The port README](../../pc_port/README.md) describes the build and architecture.

## Manual sync workflow

Run these commands from the source checkout. Set `WIKI_URL` to the Git URL of a wiki you maintain and `WIKI_DIR` to the local destination.

```bash
git clone "$WIKI_URL" "$WIKI_DIR"
rsync -avn docs/wiki/*.md "$WIKI_DIR/"
```

After reviewing the preview, copy and publish the pages.

```bash
rsync -av docs/wiki/*.md "$WIKI_DIR/"
git -C "$WIKI_DIR" status
git -C "$WIKI_DIR" add -- '*.md'
git -C "$WIKI_DIR" commit -m "Update Xenogears project wiki"
git -C "$WIKI_DIR" push
```

Check wiki claims against the committed source and release notes.
