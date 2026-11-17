# Build and verification — WEST Gaming Vertical Canvas fork

## Scope

These instructions apply to the WEST Gaming branch `feature/tiktok-native-output`, not automatically to Aitum's upstream default branch.

The current integration PR is [#1](https://github.com/westkerberos/obs-vertical-canvas/pull/1). The automatic encoder-reuse runtime change was compiled at commit `cb22d71497d2e025bfad686cab8079937f7f6387`. The latest PR head, including the subsequent documentation and attribution updates, is `03c83c3a991265a2c8a445d3fc99c26708d53687`.

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

The latest documented branch head built and packaged successfully for Windows x64:

- Workflow: [#44162348342](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/44162348342)
- Commit: `e03efd606a7828f52d883b1d41f250ba0fd48b13`
- Artifact: `vertical-canvas-1.6.7-windows-x64-ec2ec5c53`
- The automatic encoder-reuse runtime change was compiled earlier at `cb22d71497d2e025bfad686cab8079937f7f6387`; the latest successful package includes the subsequent documentation and attribution updates.
- The standard Windows ZIP and ProgramData-layout ZIP were inspected in a preceding package run. Both contained `GPL-2.0.txt`, `qrcodegen.txt`, `tiktok-live-obs.txt` and `tiktok-live-obs-GPL-3.0.txt`.
- The automatic encoder-reuse runtime change was compiled at commit `cb22d71497d2e025bfad686cab8079937f7f6387`; the later branch commits include documentation, attribution and license-distribution updates.
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
