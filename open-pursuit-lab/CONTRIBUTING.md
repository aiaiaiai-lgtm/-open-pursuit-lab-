# Contributing and task completion

Use one backlog ID per focused change. Read the status ledger and architecture,
confirm dependencies, and state which behavior becomes observable. Run:

```sh
python3 tools/verify.py --report evidence/contributor-release.json
python3 tools/verify.py --ubsan --report evidence/contributor-ubsan.json
```

Run combined ASan/UBSan where the toolchain supports it. A sanitizer runtime
startup failure must remain visible; never call it a clean memory check. Review
new report files before committing because local environment metadata is included.
Keep original delivery reports as historical evidence; create new reports for
new revisions. No hosted CI or engine pass is implied by a workflow file.

For UE changes, add engine build ID, target/platform, asset manifest, commands,
exit codes and the specific backlog acceptance artifacts. Engine source and
third-party content require their own distribution rights; do not copy GTA assets.
Use Git LFS for future `.uasset`/`.umap` files and distribute large captures as
build artifacts, with actual artifact links recorded after upload.

Tests should cover contracts and failure cases. Keep exact integer policy tests
separate from tolerance-based physics tests. Golden digest changes require a
reviewed explanation and first-differing-tick trace; they are not automatic fixes.

PR description: concrete problem, resulting behavior, actual validation and
remaining limitations. Update `docs/STATUS.md` only when artifacts support the
change. Do not close a task based on comments, mock output or planned tests.
