# WEST Gaming Streaming Stack — Source of Truth

**Status date:** 2026-11-27  
**Vertical Canvas integration merge:** `8c64a52994fd10fbc2414ea7bf98e7ee02975f42` ([PR #6 merged](https://github.com/westkerberos/obs-vertical-canvas/pull/6))  
**Multistream integration merge:** `6d4c0922f60ddd0012778c69dc7584a0cef55d15` ([PR #2 merged](https://github.com/westkerberos/obs-aitum-multistream/pull/2))  
**Target host:** OBS Studio 33.0.3 · Windows x64  
**Purpose:** Single operational and engineering reference for the complete WEST Gaming OBS stack. Use this document before changing OBS, either Aitum plugin, DirectShow, DroidCam or the build dependencies.

> A successful compile proves only that the source built. It does not prove Twitch EBC negotiation, encoder identity/reuse, TikTok authentication/media delivery, Dual LIVE, or the native MJPEG recorder works at runtime. Keep a backup and validate the paired stack before replacing a working OBS installation.

## 1. Inventory and version matrix

| Component | Current version / source | Official upstream status | Current evidence |
|---|---|---|---|
| OBS Studio | **33.0.3**, Windows x64 | Current target used by the logs and builds in this update | The user's 27 Nov logs are OBS 33.0.3 |
| WEST Vertical Canvas | **1.6.8**, merged to `main` via PR #6 (`8c64a52994fd10fbc2414ea7bf98e7ee02975f42`) | Ports Aitum Vertical 1.6.8 while retaining WEST TikTok/EBC code | Post-merge Windows x64 CI [#46036075188](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/46036075188) succeeded; artifact `vertical-canvas-1.6.8-windows-x64-8c64a5299` |
| WEST Aitum Multistream | **1.0.10**, merged to `main` via PR #2 (`6d4c0922f60ddd0012778c69dc7584a0cef55d15`) | Contains Aitum Multistream 1.0.10 upstream changes plus WEST EBC and TikTok bridge | post-merge Windows x64 CI [#46036486335](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/46036486335) succeeded; artifact `aitum-multistream-1.0.10-windows-x64-6d4c0922f` |
| OBS DirectShow module | Custom **OBS 33.0.3 `win-dshow.dll`** patch, merged into [maintenance repo main](https://github.com/westkerberos/ChatGPT/commit/ae7cc2dbcf71a3f6be74579577d47fd90fa452b3) | This is a patch to OBS core's DirectShow module, not a third-party Aitum plugin | Latest Windows x64 build [#46035805651](https://github.com/westkerberos/ChatGPT/actions/runs/46035805651) succeeded; runtime MJPEG recording still requires validation |
| DroidCam OBS (optional) | **2.5.1** | Latest official release inspected on 27 Nov 2026; use the OBS 33-specific installer if reinstalling | User's 27 Nov startup log reports DroidCam release 251; official OBS 33 installer: [DroidCamOBS.Setup.2.5.1.obs33_1.exe](https://github.com/dev47apps/droidcam-obs-plugin/releases/download/2.5.1/DroidCamOBS.Setup.2.5.1.obs33_1.exe) |

For a concise handoff into a future ChatGPT conversation, see [NEW-CHAT-START-HERE.md](NEW-CHAT-START-HERE.md). It includes the exact merged SHAs, build artifact IDs/hashes and remaining validation gates.

The Aitum plugins are a **matched pair**. Do not install the official upstream Aitum binaries over the WEST builds: doing so removes the custom Twitch EBC, TikTok bridge/runtime and pre-flight safeguards. DroidCam is an independent optional plugin; it is not part of the Aitum build or the DirectShow MJPEG patch.

## 2. Feature ownership and cross-plugin contracts

| Component | Owns | Must not take over |
|---|---|---|
| OBS Studio | Main Canvas, source/scene graph, global audio, output API and plugin loading | WEST destination/session policy |
| WEST Aitum Multistream | Main Canvas destination UI/output lifecycle, Twitch Enhanced Broadcasting negotiation/output, Vertical Canvas destination list/start-stop UI, TikTok configuration bridge | TikTok cookies, LIVE session, signing, TikTok ingest or Dual LIVE runtime |
| WEST Vertical Canvas | Portrait canvas/scenes, vertical destination outputs, Twitch EBC portrait-video encoder lookup/reuse, shared independent vertical fallback, TikTok account/session/output/safety/Dual LIVE | Twitch EBC negotiation itself or a duplicate Multistream Twitch output |
| OBS DirectShow patch (`win-dshow.dll`) | Optional packet-level native MJPEG webcam recorder inside OBS's Windows DirectShow module | Aitum output/encoder orchestration |
| DroidCam OBS | Optional DroidCam camera source | Native DirectShow MJPEG recorder and Aitum streaming outputs |

### Required inter-plugin API contract

- Multistream opens the TikTok destination editor through `aitum_vertical_configure_tiktok_output`.
- Multistream obtains the Vertical Canvas destination list via `aitum_vertical_get_stream_settings` and delegates destination start/stop via `aitum_vertical_start_stream_output` and `aitum_vertical_stop_stream_output`.
- Vertical Canvas identifies Twitch EBC output(s) using the `aitum_multi_ebc_output_` prefix.
- A vertical destination opts into Twitch EBC portrait-video reuse through the reserved video-encoder value `__aitum_twitch_ebc_vertical__`. Legacy profiles using `use_twitch_ebc_encoder=true` remain supported.
- Changes to these proc-handler names, output prefixes, marker values, saved settings keys, encoder reference ownership or start/stop callbacks are **cross-repository changes**. Update and build both forks together.

## 3. Encoder routing and operational dependencies

### Twitch Enhanced Broadcasting

Multistream negotiates Twitch EBC and owns the EBC RTMP output. With Vertical Canvas available, the Twitch request contains:
- Main Canvas (horizontal) tracks.
- Vertical Canvas (portrait) tracks.

Vertical Canvas locates the active Multistream EBC output and reuses the highest-resolution portrait video encoder that matches its vertical canvas. The tested 1080×1920 track is named like `aitum_twitch_ebc_video_3_Twitch Output`.

This reuses the same OBS video-encoder object; it does not create another vertical NVENC instance for every opted-in destination. The feature is conditional on EBC being active and the track matching. Per-destination independent encoders and the shared independent fallback remain available.

### TikTok Dual LIVE

Automatic TikTok Dual LIVE has two separate prerequisites:

1. **Portrait:** reuses the encoder(s) already attached to the TikTok Vertical Canvas output. When configured for Twitch EBC, that vertical output must be started after Twitch EBC has created its portrait track.
2. **Landscape:** reuses the active OBS Main Canvas streaming output's video encoder and audio encoder 0. A horizontal Main Canvas stream must already be active before TikTok Dual LIVE starts.

These are distinct dependencies. Twitch being active does not, by itself, satisfy the Main Canvas requirement. Conversely, YouTube horizontal being active does not provide Twitch's EBC portrait track.

### Recommended pre-flight start order

Use this predictable sequence for the user's current profile:

1. **Twitch EBC** — start it and wait until Twitch is connected and its EBC tracks are initialized.
2. **YouTube Vertical** — start only after Twitch EBC; confirm it selects the 1080×1920 EBC track rather than a fallback NVENC.
3. **YouTube Horizontal** — start the Main Canvas output. This supplies the encoder objects required by TikTok Landscape.
4. **Instagram Vertical** — start after EBC if its destination is configured to reuse that track.
5. **TikTok Dual LIVE** — start last, after Portrait's selected encoder is ready and the Main Canvas horizontal output is active.

The exact position of YouTube Vertical and Instagram relative to YouTube Horizontal does not affect TikTok's Main Canvas prerequisite. The order above is the recommended repeatable operator checklist, not a claim that every target requires every preceding destination.

Vertical Canvas now includes explicit pre-flight prompts:
- If an output is configured for Twitch EBC but the EBC vertical track is unavailable, it asks whether to start Twitch first or explicitly continue with an independent vertical encoder (with a warning about extra encoder load).
- If TikTok Dual LIVE cannot find an active Main Canvas output, it shows a specific OBS-level warning even when the Vertical Canvas dock is hidden.
- The EBC pre-flight applies to both individual destination start and Start All.

### Audio is not a single shared EBC path

The EBC reuse optimization primarily concerns **video/NVENC**. Ordinary Vertical Canvas destinations use the Vertical Canvas audio encoder path when sharing audio. TikTok Dual LIVE has its own shared or manual audio behaviour. Do not describe the system as reusing one identical audio/video bitstream across all platforms.

## 4. TikTok feature inventory

The TikTok runtime belongs to Vertical Canvas and must remain intact through upstream updates:
- Custom RTMP / user-supplied server and stream key.
- TikTok LIVE Studio Desktop.
- TikTok LIVE Studio Mobile (Camera LIVE and screen-capture modes have different capabilities).
- Streamlabs Desktop.
- Cookie-backed account sessions, QR login, account refresh, device profiles and quota feedback.
- LIVE title, topic/game, gifts/comments, content-disclosure and mature-content settings.
- LIVE safety monitoring and manual violation/appeal workflows where supported by the account and API.
- Dual LIVE portrait + landscape with automatic reuse or manual advanced layout/encoder overrides.

Automatic Dual LIVE is documented for eligible LIVE Studio Desktop and Mobile Camera LIVE workflows. It is not the paired-feed mode for Custom RTMP, Streamlabs or Mobile screen capture. Consult [TikTok LIVE](TIKTOK-LIVE.md) for connection and account constraints.

The `tiktok-relay` directory is only a JSON-lines control-protocol prototype. It does **not** carry media, implement RTMP/FLV transport or replace the current TikTok runtime.

## 5. DirectShow native MJPEG recorder

The custom camera DLL is a patch to OBS's `plugins/win-dshow` module that builds **`win-dshow.dll`**. It is not the DroidCam plugin and not an Aitum extension.

The optional recorder:
- Requires an active DirectShow source.
- Requires `native_mjpeg_record` to be enabled.
- Requires MJPEG capture at 3840×2160 or higher.
- Writes the incoming MJPEG packets to MOV without video re-encoding.
- Attempts PCM S16LE audio from the configured OBS audio source (default `Mic/Aux`); if that source is unavailable, the recorder warns and writes video only.

Latest build: [workflow #46035805651](https://github.com/westkerberos/ChatGPT/actions/runs/46035805651), DirectShow source head `80c45786d1cb4659a364b17580d937aaeaa518dc`, merged maintenance repo main `ae7cc2dbcf71a3f6be74579577d47fd90fa452b3`, artifact `obs-33.0.3-native-mjpeg-win-dshow`. ZIP SHA-256: `88dff9f2b12b735370d5f7928fef91a8c476627c67a43f4b2e813b3777cd9cc6`; DLL SHA-256: `EBD7B77E9DA422F9A6ED72BC4661C8387E8889D1BCC1D5EFD1BAC2FCF1ECF9AD`. This is compile/package evidence only. The actual camera's 4K MJPEG mode, MOV integrity, audio sync, source deactivation/reactivation and long-session handling still need runtime validation.

- [DirectShow patch source](https://github.com/westkerberos/ChatGPT/blob/main/obs-33.0.0-beta4-native-mjpeg.patch)
- [DirectShow implementation and test notes](https://github.com/westkerberos/ChatGPT/blob/main/docs/DIRECTSHOW-NATIVE-MJPEG.md)

## 6. Official upstream changes carried forward

### Aitum Multistream 1.0.10

The WEST fork declares 1.0.10 and carries both official upstream changes:
- [Aitum commit 64ef323](https://github.com/Aitum/obs-aitum-multistream/commit/64ef3233a8b078268adf68ec8427d0bfba68678d): reload Vertical Canvas outputs after OBS fires `OBS_FRONTEND_EVENT_FINISHED_LOADING`.
- [Aitum commit 41329c5](https://github.com/Aitum/obs-aitum-multistream/commit/41329c56d8f1254b7b55a2821a44b08abeeab3bb): version 1.0.10 and OBS 33 ProgramData plugin installer paths.

The WEST branch additionally retains the TikTok destination bridge, Twitch EBC negotiation/output and OBS 33.0.3 build configuration. Do not substitute upstream binaries.

### Aitum Vertical 1.6.8

The WEST fork declares 1.6.8 and carries:
- The earlier upstream scene-list loading/order fix (commit [fde49d2](https://github.com/Aitum/obs-vertical-canvas/commit/fde49d2339af4180cf014e4d7a23fc0ebccd9324)).
- The optional Safe Areas preview feature from [Aitum commit 3751ef7](https://github.com/Aitum/obs-vertical-canvas/commit/3751ef743337b9e7635f52d4e8fc74011596d8d0). It is disabled by default and can be enabled in Vertical Canvas settings.
- Aitum's 1.6.8 version/manifest changes.

The WEST fork deliberately retains its own OBS 33.0.3 buildspec and matching 2026-10-23 dependency pins. Do not copy upstream's older OBS 31.1.0 buildspec over it. The OBS 33 ProgramData installer layout was already present in the WEST branch and remains.

### OBS 33 plugin installation layout

For standard Windows OBS 33 installs, use the third-party plugin path:
```text
C:\ProgramData\obs-studio\plugins\<plugin-name>\<plugin-name>.dll
C:\ProgramData\obs-studio\plugins\<plugin-name>\data\...
```

For portable OBS, use `plugins\<plugin-name>\<plugin-name>.dll` and its `data` directory under the portable OBS root. Ensure only one copy of each module is active; don't leave a legacy copy in `obs-plugins\64bit` alongside the ProgramData copy. See [OBS legacy plugin locations](https://obsproject.com/kb/legacy-plugin-locations).

## 7. Evidence from the 27 November OBS logs

The two uploaded logs were created by OBS 33.0.3 on Windows 10 22H2:
- `2026-11-27 12-50-14.txt` (conversation upload) — Multistream 1.0.9.
- `2026-11-27 13-01-00.txt` (conversation upload) — Multistream 1.0.10.

Observed in the 12:50 session:
- TikTok was initially started without Main Canvas; Vertical Canvas reported `OBS Main Canvas streaming output is unavailable`.
- A later attempt used the independent vertical encoder because Twitch EBC was not active yet.
- After Twitch EBC became active, the 1080×1920 track was found and reused; TikTok Portrait and Landscape both connected and remained until the operator stopped them.
- Earlier landscape attempts had RTMP socket resets while the output was running with missing prerequisites/fallback encoder. The logs do not prove that the fallback alone caused the resets.

Observed in the 13:01 session:
- Twitch EBC started first and created five video encoders, including 1080×1920.
- YouTube horizontal/Main Canvas started next.
- TikTok then found/reused the EBC portrait encoder; both portrait and landscape RTMP feeds connected to TikTok and stayed active until manual stop.
- No TikTok RTMP errors were recorded in that final tested session.

The evidence supports the pre-flight order above. It does not prove all destinations, all TikTok methods, long-duration streaming or all failure paths have been runtime-tested.

## 8. Paired validation matrix

For every update, record OBS build, Windows build, GPU driver, plugin commit SHAs, artifact names, DLL hashes and the test log. Use a clean portable OBS 33.0.3 test installation before replacing the production profile.

1. Confirm one copy of each plugin loads from the intended OBS 33 plugin path.
2. Check Main Canvas and Vertical Canvas scenes, source layout, preview and scene changes.
3. Start Twitch EBC; verify Main Canvas and 1080×1920 portrait tracks.
4. Start YouTube Vertical and inspect the actual EBC encoder object name/identity.
5. Start YouTube Horizontal; confirm the Main Canvas output is active.
6. Start Instagram Vertical and TikTok. Test TikTok Portrait and Dual LIVE separately.
7. Test Start All and individual start/stop. Confirm pre-flight prompts appear when EBC or Main Canvas prerequisites are missing.
8. Test one output disconnecting while independent targets remain active; test reconnect and complete shutdown.
9. Measure NVENC/3D engine use with the intended combination; check for duplicate vertical encoder creation.
10. Test Custom RTMP, LIVE Studio Desktop, Mobile Camera LIVE, Mobile screen capture restrictions, Streamlabs and Dual LIVE eligibility.
11. Test DirectShow 4K MJPEG video-only and video+audio; check MOV integrity, timing, source deactivation/reactivation and long recordings.
12. Test OBS restart/profile switch and at least ten stream start/stop cycles; then perform an extended soak test.

### Current test status (27 November 2026)

| Area | Status |
|---|---|
| Vertical Canvas 1.6.8 Windows x64 compile/package against OBS 33.0.3 | **Passed** — post-merge workflow [#46036075188](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/46036075188) |
| Multistream 1.0.10 Windows x64 compile/package against OBS 33.0.3 | **Passed** — workflow [#46035894193](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/46035894193) |
| DirectShow patched `win-dshow.dll` Windows x64 compile/package against OBS 33.0.3 | **Passed** — workflow [#46035805651](https://github.com/westkerberos/ChatGPT/actions/runs/46035805651) |
| TikTok Dual LIVE with correct prerequisites, in the user's 13:01 log | **Passed for a short session**; portrait + landscape active until manual stop |
| Twitch EBC 1080×1920 encoder detection/reuse in 13:01 log | **Observed working** |
| TikTok missing-prerequisite prompts | **Included in the Vertical Canvas 1.6.8 Windows x64 build**; OBS runtime verification still pending |
| YouTube Vertical and Instagram with EBC in the same paired test session | **Not yet evidenced by the two uploaded logs** |
| DShow real-camera 4K MJPEG recording, MOV/audio integrity | **Not yet runtime-validated** |
| Long-duration soak, ten-cycle stop/restart, failure injection | **Pending** |

CI formatting, Linux and macOS jobs are skipped by the current Windows-only fork workflows. Do not claim cross-platform support based on these runs.

## 9. Release and provenance gates

The current TikTok-native source still contains unresolved provenance/licensing questions:
- at least 24 integration files are identified as derived from GPL-3.0-only projects but are compiled into the same Aitum-derived GPL-2.0 OBS plugin;
- the desktop/mobile device-registration implementation cites a public repository with no published license.

See [LICENSING-NOTES.md](LICENSING-NOTES.md), [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md), [LICENSING-ARCHITECTURE-OPTIONS.md](LICENSING-ARCHITECTURE-OPTIONS.md) and [TIKTOK-REIMPLEMENTATION-PLAN.md](TIKTOK-REIMPLEMENTATION-PLAN.md). The separate relay remains a control-protocol stub and does not resolve those issues. This is not legal advice. Do not publish a public combined binary or claim license clearance until the provenance is independently cleared or the implementation is remediated.

## 10. Update workflow

When OBS or an upstream plugin changes, follow [UPDATE-PROCEDURE.md](UPDATE-PROCEDURE.md). In short: inventory official releases, diff upstream changes, update the matched pair together, preserve WEST feature invariants, build both components against the exact OBS source/dependency baseline, test the camera patch separately, run the paired runtime matrix, then update all version/commit/hash records in one documentation change.

## 11. Project links

- [WEST Vertical Canvas](https://github.com/westkerberos/obs-vertical-canvas)
- [WEST Aitum Multistream](https://github.com/westkerberos/obs-aitum-multistream)
- [DirectShow/MJPEG patch source](https://github.com/westkerberos/ChatGPT/tree/obs-33.0.3-native-mjpeg)
- [DroidCam OBS 2.5.1, OBS 33 installer](https://github.com/dev47apps/droidcam-obs-plugin/releases/download/2.5.1/DroidCamOBS.Setup.2.5.1.obs33_1.exe)
- [Aitum Vertical upstream](https://github.com/Aitum/obs-vertical-canvas)
- [Aitum Multistream upstream](https://github.com/Aitum/obs-aitum-multistream)
- [OBS Studio upstream](https://github.com/obsproject/obs-studio)
