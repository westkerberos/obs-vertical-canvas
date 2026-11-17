# Build and verification — WEST Gaming Vertical Canvas fork

## Scope

These instructions apply to the WEST Gaming branch `feature/tiktok-native-output`, not automatically to Aitum's upstream default branch.

The current integration PR is [#1](https://github.com/westkerberos/obs-vertical-canvas/pull/1). The latest implementation documented here is commit `cb22d71497d2e025bfad686cab8079937f7f6387`.

## Dependencies visible in the build configuration

The plugin is a CMake C++ module that links to OBS libobs and OBS frontend API. The TikTok additions use:

- OBS development libraries: libobs and obs-frontend-api.
- Qt 6 Core and Widgets.
- libcurl for HTTP requests.
- qrcodegencpp for QR code generation.
- C++20.
- Windows BCrypt for the Windows cryptographic path; OpenSSL Crypto is used on supported non-Apple, non-Windows builds.

The exact packages and toolchain versions depend on the OBS development environment used for the build.

## Standalone build

With the required OBS development packages installed:

```sh
cmake -S . -B build -DBUILD_OUT_OF_TREE=ON
cmake --build build --config Release
```

For an in-tree OBS build, follow the OBS plugin integration pattern already used by the upstream Vertical Canvas project and build it as part of OBS Studio. Do not use the standalone command as a substitute for the platform-specific packaging workflow.

## CI and packaging

The latest Windows x64 build/package workflow passed for the current documented branch head:

- Workflow: [#44148907864](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/44148907864)
- Commit: `884e19f39e6bb17a031261983487dc4a3faca90d`
- Result: Windows build succeeded; package succeeded; artifact upload succeeded.
- Artifact: `vertical-canvas-1.6.7-windows-x64-4117ef013` (SHA-256 `438a024dfb539c5fb175c3f4ea64e97ce5375ab21584c6c730645a162afcf177`).
- The tested runtime code commit for automatic Dual LIVE encoder reuse is `cb22d71497d2e025bfad686cab8079937f7f6387`; the later branch-head commits add documentation and third-party license notices.
- Signing: skipped because this fork does not have signing secrets configured.
- Ubuntu/macOS: not built by the current fork workflow.

There is no GitHub Release published for this repository yet. GitHub Actions artifacts are build outputs, not a stable release channel.

## What CI proves — and what it does not

A successful build proves that the configured Windows x64 workflow compiled and packaged the source at the tested commit. It does not prove:
- TikTok will grant Dual LIVE eligibility to every account.
- QR login works for every account/region.
- Both ingest feeds remain healthy across all network interruptions.
- Every GPU/encoder combination is supported.
- Linux/macOS runtime compatibility.
- TikTok will accept a LIVE or appeal request.

The repository owner confirmed the new integration in a real OBS/TikTok environment, including the automatic Dual LIVE encoder-reuse mode, on 17 November 2026. Treat that as a maintainer test, not as a broad hardware/platform compatibility matrix.

## Runtime test checklist for a new release

Run each test on a clean OBS profile first and record the OBS version, Windows build, GPU, encoder and TikTok connection method.

1. Single-feed LIVE using an existing saved cookie file.
2. Account refresh after changing or replacing the cookie file.
3. QR login: generate, scan, confirm, save cookies and restart OBS. This flow should be treated as independently unverified until a maintainer records a successful run.
4. Topic/game validation, gifts/comments and content disclosure.
5. Safety: manual violation lookup, monitoring while live, stop-time final check and appeal actions where eligible.
6. Dual LIVE with an eligible account: verify Portrait uses the Vertical Canvas output settings, Landscape uses the active Main Canvas encoders, and both feeds are visible in TikTok.
7. Dual LIVE failure behavior: start without Main Canvas active, interrupt one feed, stop manually and verify the pair is cleaned up.
8. Advanced manual override: test independent scenes, encoder settings, Fit/Crop and shared/separate audio.
9. Restart OBS and verify connection settings, device profiles and Dual LIVE choices persist.

Never run first-time Dual LIVE tests during a critical public broadcast.

## Release checklist

- [ ] Merge PR #1 or clearly identify the exact supported feature branch/commit.
- [ ] Resolve the third-party attribution/licensing review for TikTok-derived code before redistributing binaries.
- [ ] Verify the standard installer and the ZIP on a clean OBS installation.
- [ ] Decide whether Windows code signing will be configured.
- [ ] Add macOS/Linux jobs only when the dependencies and runtime paths are implemented and tested.
- [ ] Publish a GitHub Release with exact OBS compatibility, asset names, hashes and known limitations.
- [ ] Keep the matching Aitum Multistream bridge documentation/version synchronized.
