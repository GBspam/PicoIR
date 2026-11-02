# GitHub publishing notes

Repository: https://github.com/GBspam/PicoIR

The repository already exists and the `main` branch contains the v0.8-dev checkpoint. The v0.9-dev source is being published as a follow-on commit to that branch. This reconstructed history is intentionally committed onto the existing remote history rather than force-pushing the local recovery bundle, whose historical commit IDs differ from the remote repository.

This commit publishes the v0.9-dev source, host library, tests, README, changelog, and release notes. It does not create a version tag or a GitHub Release page; those are separate actions and should only be created after reviewing the published tree.

The v0.1 and v0.2 entries are documentation/recovery checkpoints because original source archives for those versions were not present in the recovered materials. Hardware support is not claimed: PIO/DMA timing, dual-CDC TinyUSB integration, physical optical circuitry, and end-to-end Game Boy IR interoperability remain unvalidated.
