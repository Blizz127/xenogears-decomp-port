# GitHub Wiki staging area

`docs/wiki/` is the source for the GitHub wiki of
[Blizz127/xenogears-decomp-ai](https://github.com/Blizz127/xenogears-decomp-ai/wiki).
The published pages are a separate git repository,
`Blizz127/xenogears-decomp-ai.wiki.git`.

The decompile and port totals live in `Home.md`, `Decompile-Status.md`, and
`Port-Status.md`. They should match the tables on the repository README.
`pc_port/README.md` is the build and architecture note for the port.

## Manual sync workflow

1. Clone the wiki repo (once):

   ```bash
   git clone git@github.com:Blizz127/xenogears-decomp-ai.wiki.git
   ```

   If your fork uses a different repo name, the wiki URL is:

   ```text
   git@github.com:Blizz127/<repo-name>.wiki.git
   ```

2. Preview what would be copied (does not write):

   ```bash
   ./scripts/preview_wiki_sync.sh ~/Projects/xenogears-decomp-ai.wiki
   ```

3. Copy staged pages into the wiki checkout, review, commit, and push manually:

   ```bash
   rsync -av \
     ~/Projects/xenogears-decomp/docs/wiki/*.md \
     ~/Projects/xenogears-decomp-ai.wiki/

   cd ~/Projects/xenogears-decomp-ai.wiki
   git status
   git add *.md
   git commit -m "Update Xenogears project wiki"
   git push
   ```

Do **not** treat wiki pages as authoritative unless they match current committed handoff notes.
When in doubt, trust `ACTIVE_HANDOFF.md` and recent commit messages.
