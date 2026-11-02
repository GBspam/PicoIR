# Publishing the complete local Git history

The current GitHub `main` branch contains the PicoIR 0.5 source snapshot and project documentation. This was added as a source-sync commit after the documentation commits. It does not recreate the original local commit ancestry or publish the local version tags.

The prepared local history bundle preserves the original reconstructed local history and tags. To publish that full history, first ensure you have a clone of the target repository and then reconcile the existing GitHub documentation/source-sync commits with the local history before pushing; do not force-push over the GitHub branch without reviewing the histories.

The local bundle can be inspected with:

```sh
git clone /path/to/PicoIR-history.bundle PicoIR-history
cd PicoIR-history
git log --oneline --decorate --all
git tag --list
```

Tags `v0.1` and `v0.2` are archival design/recovery checkpoints, not complete firmware releases. Tags `v0.3`–`v0.5` represent source snapshots. GitHub Release pages are separate from Git tags.
