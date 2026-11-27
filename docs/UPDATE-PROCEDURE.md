# WEST Gaming OBS stack — update procedure

This is the repeatable procedure for requests such as **“OBS updated; bring all WEST plugins up to date.”** Do not start by replacing DLLs or copying the latest upstream installers over the WEST forks.

## 1. Establish the current environment

Record the user's actual versions from the latest OBS log, not from memory:
- OBS Studio version, Windows build, architecture and portable/standard mode.
- GPU and driver.
- Loaded module names and versions.
- Actual plugin DLL paths (check for duplicate legacy and ProgramData copies).
- Which output graph is enabled: Twitch EBC, Main Canvas, Vertical Canvas, YouTube, Instagram, TikTok, recording/replay and camera modules.

Distinguish these components:
- **Aitum Multistream fork** — Main Canvas targets, Twitch EBC and the Vertical Canvas integration bridge.
- **Aitum Vertical Canvas fork** — vertical scenes/outputs, EBC portrait encoder reuse and TikTok runtime.
- **OBS DirectShow `win-dshow.dll` patch** — modification to OBS core's DirectShow module for native MJPEG recording.
- **DroidCam OBS** — separate third-party camera plugin; it is not the DirectShow patch.

Do not infer which camera component the user means from the phrase “camera DLL”. Check the loaded module list and version log.

## 2. Check official upstream releases

For each component, query its official GitHub Releases/tags and upstream default branch. Record:
- Latest release/tag and date.
- Exact upstream commit SHA for that release.
- The commits/files added since the last upstream revision incorporated by the WEST fork.
- Buildspec/dependency/toolchain changes and install-layout changes.
- Whether the update changes OBS APIs, encoder lifecycle, threading, output start/stop, plugin loading or saved settings.

For OBS, check the official stable release and source tag. Do not assume that the latest OBS release is automatically compatible with every plugin. The OBS source/dependency pin and plugin install layout must be updated deliberately.

## 3. Preserve the matched Aitum pair

The two WEST Aitum repositories are one product and must be reviewed together:

- [WEST Vertical Canvas](https://github.com/westkerberos/obs-vertical-canvas)
- [WEST Aitum Multistream](https://github.com/westkerberos/obs-aitum-multistream)

Before merging upstream changes, preserve these contracts:

| Contract | Owner | Invariant |
|---|---|---|
| Twitch EBC negotiation/output | Multistream | Dedicated EBC output prefix remains `aitum_multi_ebc_output_` |
| Vertical destination UI/list and start/stop | Multistream bridge + Vertical Canvas | Proc-handler APIs remain compatible |
| EBC portrait video selection | Vertical Canvas | Reserved encoder marker remains `__aitum_twitch_ebc_vertical__`; old checkbox profiles remain supported |
| TikTok connection/account/session/signing/safety | Vertical Canvas | Multistream does not grow a second TikTok runtime |
| TikTok Dual LIVE Portrait | Vertical Canvas output | Uses the configured vertical output encoders |
| TikTok Dual LIVE Landscape | Vertical Canvas runtime | Reuses the active OBS Main Canvas video/audio encoder objects |
| Recording/replay/virtual camera | OBS + Vertical Canvas | Must not accidentally inherit stream-only EBC encoders |

The key API boundary currently includes:
- `aitum_vertical_configure_tiktok_output`
- `aitum_vertical_get_stream_settings`
- `aitum_vertical_start_stream_output`
- `aitum_vertical_stop_stream_output`

Rename a marker, proc-handler, output prefix or persisted settings key only as a coordinated change across both repositories, with migration coverage.

## 4. Port upstream changes surgically

1. Create an update branch from the last known-good paired integration commit.
2. Merge or cherry-pick upstream commits one component at a time.
3. Do not replace a WEST fork wholesale with the official upstream binary or source archive.
4. Preserve the WEST OBS 33+ buildspec/dependency pins; compare upstream `buildspec.json`, manifest, CMake, CI and installer templates separately.
5. Apply upstream source/UI/bug fixes while preserving TikTok and EBC ownership boundaries.
6. Review changes to output callbacks, OBS refs, thread affinity, stop/reconnect semantics and encoder reference ownership line-by-line.
7. Update English/Spanish locale entries and saved-setting migration where applicable.
8. Keep optional upstream UI features opt-in and default-off unless their behaviour has been tested.

## 5. Camera components are separate workstreams

### OBS DirectShow patch
The MJPEG camera recorder is a patch to the OBS source tree's `plugins/win-dshow` module, producing `win-dshow.dll`. When OBS changes:
- Rebase/apply the patch to the exact OBS tag.
- Check for upstream changes in DirectShow packet handling, source lifecycle and audio/video timestamps.
- Build the `win-dshow` target against that exact OBS source and dependency baseline.
- Test 4K MJPEG capture, MOV integrity, audio/video sync, source hide/show/deactivation/reactivation, non-ASCII paths and long-duration recording.
- Never copy an older OBS 33.0.1 DLL into OBS 33.0.3.

### DroidCam OBS
DroidCam is a separate third-party plugin. Check its official release and the installer variant that explicitly supports the target OBS version. Do not rebuild or replace it when its installed version is already the current compatible release.

## 6. Build gate

A release candidate must have:
- Exact OBS source tag and hashes recorded in `buildspec.json`.
- Matching OBS dependencies and Qt version.
- Windows x64 build and package success for both Aitum forks.
- Separate DirectShow target build if its patch is included.
- Artifact names, commit SHAs and SHA-256 hashes.
- No accidental duplicate module installation paths.
- Formatting/static checks run where configured; clearly report jobs skipped by design.

Build success is **not** runtime validation. Never label a pair production-ready solely because CI is green.

## 7. Runtime test sequence

Use a clean portable OBS copy first. Save the log and record the exact branch heads/DLL hashes.

1. Start Twitch EBC and wait until Twitch is connected and portrait tracks are initialized.
2. Start YouTube Vertical; verify it reuses the negotiated 1080×1920 EBC video encoder.
3. Start YouTube Horizontal/Main Canvas.
4. Start Instagram Vertical.
5. Start TikTok Dual LIVE last; verify Portrait and Landscape independently.
6. Confirm Main Canvas is active before TikTok Dual LIVE. It supplies the landscape video/audio encoders; Twitch EBC supplies the portrait video track.
7. Test start/stop each output, Start All, missing-prerequisite prompts, reconnect and one-feed failure.
8. Verify no duplicate vertical NVENC sessions, no encoder reference errors and no unrelated output gets stopped.
9. Test profile save/reload, OBS restart, ten start/stop cycles and a long-duration soak.
10. Test the DirectShow MJPEG recorder separately with the actual camera and capture mode.

The current pre-flight safeguards must remain:
- Missing EBC track prompts the user to start Twitch first or explicitly continue with an independent encoder.
- Missing Main Canvas prompts the user to start a horizontal stream before TikTok Dual LIVE.
- The default action should be the safe/cancel path, not silent fallback.

## 8. Merge and release workflow

- Keep upstream integration, pre-flight changes and version bumps in a reviewable branch.
- Make the Windows build pass for both forks at the exact branch heads.
- Run the paired OBS test matrix and update [WEST-GAMING-STREAMING-STACK.md](WEST-GAMING-STREAMING-STACK.md), [BUILD-AND-TESTING.md](BUILD-AND-TESTING.md), [UPSTREAM-DIFFERENCES.md](UPSTREAM-DIFFERENCES.md) and [TIKTOK-LIVE.md](TIKTOK-LIVE.md) in the same change set.
- Merge only after the code review, CI and release gates are satisfied; then update the source-of-truth to the actual merged SHAs.
- Keep a known-good tag/backup so rollback is deterministic.
- Do not publish a combined binary until the licensing/provenance blockers documented in [LICENSING-NOTES.md](LICENSING-NOTES.md) and [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md) are resolved or independently cleared. This document is engineering guidance, not legal advice.

## 9. Minimum update report

Every future update should conclude with this compact record:

```text
OBS:
Windows:
Vertical Canvas: version / commit / CI run / artifact / SHA-256
Aitum Multistream: version / commit / CI run / artifact / SHA-256
DirectShow win-dshow.dll: OBS base commit / patch commit / CI run / DLL SHA-256
DroidCam OBS: version / installer variant / status
Upstream commits ported:
WEST features preserved:
Runtime tests passed:
Runtime tests pending:
Known risks / licensing:
Rollback tag:
```
