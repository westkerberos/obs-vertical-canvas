# WEST Gaming Streaming Stack — Source of Truth

**Status date:** 2026-11-26  
**Target host:** OBS Studio 33.0.3, Windows x64  
**Purpose:** Preserve the complete WEST Gaming streaming system when OBS, Aitum upstream plugins, OBS modules or build dependencies change. This document is the system-level map; component-specific guides remain linked below.

> This is an engineering inventory, not a release claim. A green CI build is not proof that the full streaming graph works in OBS. Do not install an experimental stack over the production OBS installation until the paired components pass the test matrix at the end of this document.

## 1. Required end-to-end behavior

The system must preserve all of the following as one integrated product:

1. **Twitch landscape + portrait through Twitch Enhanced Broadcasting (EBC).** Aitum Multistream owns the Twitch EBC output and negotiates the available tracks with Twitch. When the Vertical Canvas is present, the Twitch configuration includes both the OBS Main Canvas and the portrait Vertical Canvas.
2. **Vertical Canvas destinations.** The vertical canvas is independently managed and can feed the configured vertical destinations (for example YouTube, TikTok and Instagram). It is not an optional feature to be removed when updating Multistream or OBS.
3. **Video encoder reuse.** A vertical destination configured for the Twitch EBC portrait track reuses the portrait video encoder negotiated by Twitch EBC. If the EBC output/track is unavailable, Vertical Canvas falls back to an independent vertical encoder. A destination can also be configured to use its own encoder; the EBC path is opt-in per destination.
4. **Native TikTok integration.** TikTok configuration, account/session handling, authentication, dynamic endpoint/key acquisition, request signing, LIVE metadata/safety and TikTok Dual LIVE runtime remain owned by Vertical Canvas. The Multistream integration is a configuration/management bridge, not a second TikTok runtime.
5. **TikTok Dual LIVE.** When the account and method are eligible, portrait uses the Vertical Canvas output and landscape uses active OBS Main Canvas encoders. This is a separate paired-feed feature from Twitch EBC.
6. **Independent recording/replay paths.** Recording, replay buffer and virtual-camera paths must not accidentally inherit Twitch EBC output tracks or create additional stream encodes unless explicitly configured.

Do not review or ship only one item in isolation. A change to either Aitum fork, OBS, output creation, encoder ownership or plugin installation can affect this whole graph.

## 2. Component ownership and contracts

| Component | Owns | Must not take over |
|---|---|---|
| OBS Studio | Main Canvas video/audio, scenes, source graph, core output API, plugin loader | WEST-specific destination/session orchestration |
| WEST Aitum Multistream fork | Main Canvas destination UI/output lifecycle; Vertical Canvas destination list/start-stop UI; Twitch EBC negotiation/output; TikTok target configuration bridge | TikTok cookies, LIVE session, signing, ingest transport or Dual LIVE runtime |
| WEST Vertical Canvas fork | Portrait canvas/scenes; vertical destination outputs; EBC portrait-encoder lookup/reuse; shared fallback vertical video encoder; TikTok session/output lifecycle | Twitch EBC negotiation itself; duplicating the Multistream Twitch output |
| WEST DirectShow patch (`win-dshow.dll`) | Optional native MJPEG camera recorder inside OBS's Windows DirectShow source module | Twitch/Vertical output management; this is not an Aitum plugin |

### Inter-plugin contract

- Multistream calls the Vertical Canvas proc-handler `aitum_vertical_configure_tiktok_output` to open/save TikTok-specific destination settings.
- Multistream obtains and refreshes the vertical output list through `aitum_vertical_get_stream_settings`; it delegates vertical output start/stop through the Vertical Canvas proc-handler API.
- Vertical Canvas identifies the Twitch EBC output by the output-name prefix `aitum_multi_ebc_output_`.
- A vertical destination opts into EBC portrait-video reuse through the reserved encoder value `__aitum_twitch_ebc_vertical__`. Older profiles using `use_twitch_ebc_encoder=true` are migrated by the Vertical Canvas/Multistream settings code.
- Keep these names, settings keys, ownership rules and lifetimes synchronized across both repositories. Renaming a handler, marker, output prefix or stored setting is a coordinated cross-repository change.

## 3. Twitch EBC and vertical encoder path

### Twitch side

The Multistream EBC implementation creates a dedicated custom RTMP output named with the prefix `aitum_multi_ebc_output_`. It requests Twitch's client configuration and creates the encoder/track group required by the response. The canvas configuration includes Main Canvas and, when available, the Aitum Vertical portrait canvas. The Main Canvas OBS stream is not silently converted into Twitch EBC; this is a separate Multistream-managed output path.

### Vertical side

When a Vertical Canvas destination is configured to use the special EBC vertical encoder:

1. Vertical Canvas searches active OBS outputs for the Multistream EBC output prefix.
2. It inspects the attached video encoders and matches the current vertical canvas video pointer, with a portrait-aspect-ratio fallback for grouped tracks.
3. It selects the highest-resolution matching portrait encoder and retains an OBS encoder reference for the destination output.
4. If no matching EBC encoder is available, it logs a warning and falls back to an independent vertical encoder.

This means **video/NVENC reuse is conditional**, not a guarantee for every destination or every stream state. Check each destination's encoder selection and the OBS log. New vertical destinations default to the EBC selector in the current Multistream feature branch; existing profiles should be inspected after migration. An explicit encoder choice can opt out.

### Audio is a separate path

Do not describe the EBC change as sharing every encoder object. Vertical Canvas currently shares one ordinary Vertical Canvas audio encoder among its normal vertical destinations when those outputs use the shared-audio path. That code does not select the same Twitch EBC audio track group. TikTok Dual LIVE has its own audio reuse path for the active Main/Vertical Canvas encoders. Video encoder reuse is the NVENC optimization that must be verified explicitly.

## 4. TikTok integration — preserve all connection methods

The TikTok runtime is in Vertical Canvas. The current guide documents these methods:

- Custom RTMP / user-supplied server and stream key.
- TikTok LIVE Studio Desktop.
- TikTok LIVE Studio Mobile (camera LIVE and screen-capture modes have different capabilities).
- Streamlabs Desktop.

The native LIVE Studio paths include cookie/session handling, QR login, account lookup, LIVE metadata/topic/game selection, hosted request signing, dynamic ingest details and LIVE safety/violation flows. Do not remove or replace these paths when integrating upstream Vertical Canvas changes.

Dual LIVE is currently documented for LIVE Studio Desktop and Mobile Camera LIVE with an eligible account. It is not the paired-feed mode for Custom RTMP, Streamlabs or Mobile screen capture. The default dual-feed mode reuses the active Vertical Canvas portrait encoders and OBS Main Canvas landscape encoders; the Main Canvas stream must be active first.

The separate experimental `tiktok-relay` directory is a JSON-lines control-protocol prototype. It does not carry video/audio media, implement RTMP/FLV transport, session authentication, request signing or Dual LIVE. It must not be presented as a completed process boundary or as the TikTok runtime.

Detailed guide: [TikTok LIVE in Vertical Canvas](TIKTOK-LIVE.md).

## 5. DirectShow / 4K native MJPEG webcam recorder

The DirectShow modification is a separate patch to OBS Studio's `plugins/win-dshow` module, not an Aitum plugin. It adds an optional recorder that writes the incoming MJPEG packets to a MOV file when all of these conditions are met:

- The DirectShow source is active.
- The user enables `native_mjpeg_record`.
- The selected camera capture format is MJPEG.
- The capture mode is at least 3840x2160.
- A valid output directory is configured.

The current patch also attempts to capture PCM S16LE audio from the named OBS audio source (default `Mic/Aux`); if the source is missing, it warns and writes video only. This is a packet-level recording path, not a video re-encode.

**Current evidence:** branch `obs-33.0.1-build` built `win-dshow.dll` against OBS 33.0.1 successfully (workflow [#43638792702](https://github.com/westkerberos/ChatGPT/actions/runs/43638792702)). Its artifact contains `win-dshow.dll`, a PDB and SHA-256 file; it is a temporary Actions artifact, not a release. A separate OBS 33.0.3 validation branch is being prepared. Do not install the 33.0.1 DLL into OBS 33.0.3 as though compatibility were already proven.

Static review item: `DShowInput::Activate` calls `device.Start()` before `NativeMjpegRecorder::Start()`. The device may start delivering encoded MJPEG callbacks before the recorder initialization is complete. `Active()`, `Write()` and `WriteAudio()` use the recorder mutex, but `Start()` mutates `format_`, stream pointers and header state without holding that mutex. This is a code-review concurrency risk, not yet a reproduced runtime fault; fix the initialization ordering/locking and test callback concurrency before relying on the recorder in production.

Component guide and validation record: [DirectShow native MJPEG recorder](https://github.com/westkerberos/ChatGPT/blob/obs-33.0.3-native-mjpeg/docs/DIRECTSHOW-NATIVE-MJPEG.md). Patch source: [DirectShow native MJPEG patch](https://github.com/westkerberos/ChatGPT/blob/obs-33.0.1-build/obs-33.0.0-beta4-native-mjpeg.patch).

## 6. Branch and version matrix

The following are the development branches currently carrying the combined work. The default `main` branches are not the supported combined stack.

| Component | Current integration source | Version declared in fork | Upstream version to port | Status |
|---|---|---:|---:|---|
| Vertical Canvas | [`feature/tiktok-native-ebc-combined`](https://github.com/westkerberos/obs-vertical-canvas/tree/feature/tiktok-native-ebc-combined), current head `407799eb58dca76eb4fa55946c8e177810b4726b`, [PR #6](https://github.com/westkerberos/obs-vertical-canvas/pull/6) | 1.6.7 (custom feature-preserving build) | Aitum Vertical 1.6.8 | OBS 33.0.3 Windows build passed; 1.6.8 scene-list fix and new installer path present; Safe Areas intentionally omitted |
| Aitum Multistream | [`feature/aitum-twitch-enhanced-broadcasting`](https://github.com/westkerberos/obs-aitum-multistream/tree/feature/aitum-twitch-enhanced-broadcasting), current head `98103d39e4e972b649ce912678e273eab8dbf3e7`, [PR #2](https://github.com/westkerberos/obs-aitum-multistream/pull/2) | 1.0.10 | Aitum Multistream 1.0.10 | OBS 33.0.3 Windows build passed; EBC + TikTok bridge retained; upstream reload and installer fixes present |
| DirectShow MJPEG | [`obs-33.0.3-native-mjpeg`](https://github.com/westkerberos/ChatGPT/tree/obs-33.0.3-native-mjpeg) | OBS module patch | OBS 33.0.3 | Separate validation workflow; latest run after patch-file repair is pending |
| OBS host | [OBS Studio 33.0.3](https://github.com/obsproject/obs-studio/releases/tag/33.0.3) | User target | 33.0.3 | Both Aitum forks now build from OBS 33.0.3 source dependencies; DShow is validated separately |

### OBS 33.0.3 build status

Both Aitum fork `buildspec.json` files now pin the OBS source dependency to **33.0.3** and use the Windows dependency/Qt snapshots dated 2026-10-23.

- Vertical Canvas: [Windows CI #45863996075](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/45863996075), commit `407799eb58dca76eb4fa55946c8e177810b4726b`, success; artifact `vertical-canvas-1.6.7-windows-x64-407799eb5`.
- Multistream: [Windows CI #45863976132](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/45863976132), commit `98103d39e4e972b649ce912678e273eab8dbf3e7`, success; artifact `aitum-multistream-1.0.10-windows-x64-98103d39e`.

These runs establish Windows x64 compilation and packaging against OBS 33.0.3. Vertical's CI also passes its relay control-protocol test. Formatting checks and non-Windows builds are skipped by design. Neither CI run validates actual Twitch EBC negotiation, encoder-object reuse, TikTok authentication/media transport, Dual LIVE, or the paired runtime graph.

## 7. Upstream changes integrated or intentionally excluded

### Aitum Multistream 1.0.10

The companion fork now declares **1.0.10** and contains both relevant upstream changes:
- [Commit 64ef323](https://github.com/Aitum/obs-aitum-multistream/commit/64ef3233a8b078268adf68ec8427d0bfba68678d): `LoadVerticalOutputs(false)` is called after `LoadSettingsFile()` on `OBS_FRONTEND_EVENT_FINISHED_LOADING`.
- [Commit 41329c5](https://github.com/Aitum/obs-aitum-multistream/commit/41329c56d8f1254b7b55a2821a44b08abeeab3bb): the OBS 33 ProgramData plugin install layout is present in the Windows installer.

The fork retains the custom Twitch EBC implementation and Vertical Canvas/TikTok configuration bridge. Do not replace it with the official Aitum binary.

### Aitum Vertical 1.6.8

The WEST Vertical Canvas branch already contains the upstream scene-list loading/order fix from [commit fde49d2](https://github.com/Aitum/obs-vertical-canvas/commit/fde49d2339af4180cf014e4d7a23fc0ebccd9324), and its installer already contains the OBS 33 ProgramData plugin path. The only functional addition in upstream 1.6.8 beyond that baseline is the optional Safe Areas preview UI/rendering change. It is intentionally not imported because the update requirement is to preserve the existing feature set without adding functions. The fork therefore continues to identify its feature-preserving plugin build as 1.6.7.

### OBS Studio 33 plugin-loading layout

OBS 33 introduced a new third-party plugin layout. Standard Windows installs should use:

```text
C:\ProgramData\obs-studio\plugins\<plugin-name>\<plugin-name>.dll
C:\ProgramData\obs-studio\plugins\<plugin-name>\data\...
```

Portable OBS uses a `plugins\<plugin-name>\<plugin-name>.dll` directory under the portable OBS root; ProgramData is ignored by portable launches. Old `obs-plugins\64bit` layouts are legacy and are scheduled to stop loading in OBS 34.0. References: [OBS legacy plugin locations](https://obsproject.com/kb/legacy-plugin-locations), [developer layout details](https://obsproject.com/kb/legacy-plugin-developer-info).

This means the earlier portable test ZIP created using the old `obs-plugins` layout must not be treated as the OBS 33.0.3 test package. Repackage it into `plugins\\<plugin-name>\\<plugin-name>.dll` and `plugins\\<plugin-name>\\data\\...`; do not place the module only in the legacy `obs-plugins` directory.

## 8. Test plan — paired stack, not isolated plugins

Record the exact OBS build, Windows version, GPU driver, source commit, plugin SHA-256 and log for every test.

1. **Install/load:** clean OBS 33.0.3 portable copy; verify exactly one copy of each plugin is loaded from the intended new plugin path; no missing DLLs or duplicate legacy modules.
2. **Scene and canvas:** Main Canvas + Vertical Canvas scenes, scene ordering, switching, source transforms, vertical canvas preview and vertical source layout.
3. **Twitch EBC:** Twitch Main + portrait tracks both appear; check negotiated resolutions/FPS/bitrate; verify all expected tracks are attached to the correct canvases.
4. **Encoder identity:** log/inspect actual OBS encoder object identity for each vertical target configured for EBC; verify all reuse the same EBC portrait encoder, no new NVENC session is created per destination; test explicit independent encoder and no-EBC fallback.
5. **Vertical targets:** test YouTube, TikTok and Instagram output independently and together; test Start All, individual start/stop, target edit/save/restart, and profile reload.
6. **TikTok methods:** Custom RTMP, LIVE Studio Desktop, LIVE Studio Mobile and Streamlabs, each using valid test credentials and with secrets redacted from logs.
7. **TikTok Dual LIVE:** eligible account; Main Canvas active first; validate portrait and landscape feeds, one-feed failure, manual stop and reconnect/cleanup.
8. **Network failure/lifecycle:** force a network reset on one destination; verify OBS remains alive, independent destinations continue, session/output references are released and a retry can start.
9. **NVENC/GPU:** capture encoder-engine usage before, during and after EBC + three vertical outputs; verify no unexpected encoder duplication, GPU saturation or sustained overload.
10. **DShow MJPEG:** 4K MJPEG source, video-only and video+audio, source hide/show/deactivate, reactivation, stop while callback is active, long-duration recording, path with non-ASCII characters, file integrity and A/V sync.
11. **Shutdown:** stop all outputs, close OBS, reopen profile, load scenes and targets; repeat at least ten start/stop cycles and one long soak test.

## 9. Release blockers and maintenance policy

- The TikTok-native branch still has unresolved source-license/provenance issues documented in [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md): GPL-3.0-only-derived code is compiled into the GPL-2.0 Aitum-derived module, and a device-registration path has no published license. Do not distribute a combined public binary until this is remediated or independently cleared.
- No stable combined installer/GitHub Release exists for the current pair.
- The experimental relay does not resolve the licensing/process-boundary issue.
- Do not publish "fixed", "OBS 33.0.3 compatible" or "production ready" based on compile success alone.

For every OBS or upstream update:
1. Fetch upstream release notes and exact tag/commit.
2. Diff upstream changes against each WEST fork.
3. Check the full feature invariants in section 1 before cherry-picking or merging.
4. Update buildspec/dependencies and the Windows CI toolchain for the target OBS.
5. Build Vertical and Multistream from their matching commits.
6. Run the paired runtime test plan; update the tested-version matrix and SHA-256 artifacts.
7. Update this document and the component guides in the same change set.

## 10. Project links

- [WEST Vertical Canvas repository](https://github.com/westkerberos/obs-vertical-canvas)
- [WEST Multistream repository](https://github.com/westkerberos/obs-aitum-multistream)
- [DirectShow/MJPEG patch repository](https://github.com/westkerberos/ChatGPT)
- [Aitum Vertical upstream](https://github.com/Aitum/obs-vertical-canvas)
- [Aitum Multistream upstream](https://github.com/Aitum/obs-aitum-multistream)
- [OBS Studio upstream](https://github.com/obsproject/obs-studio)
