# WEST Gaming OBS — next-chat handoff

**Verified:** 27 November 2026  
**Canonical status:** [WEST Gaming Streaming Stack](WEST-GAMING-STREAMING-STACK.md)  
**Repeatable maintenance procedure:** [UPDATE-PROCEDURE.md](UPDATE-PROCEDURE.md)

## Copy/paste prompt for a new ChatGPT conversation

> Continue maintaining my WEST Gaming OBS stack. Before changing code, read the WEST Gaming Streaming Stack source of truth and UPDATE-PROCEDURE in the WEST Vertical Canvas repository, plus the component-specific build/testing and licensing documents. Check my latest OBS log and the official upstream releases/tags for OBS, Aitum Vertical, Aitum Multistream, OBS DirectShow and DroidCam. Compare upstream changes against our merged WEST main branches. Preserve every existing custom feature and cross-plugin API. Never replace our WEST DLLs with official upstream binaries. Update both Aitum forks as a matched pair, update the OBS win-dshow patch against the exact OBS tag, build Windows x64, verify artifacts and hashes, update all documentation, and merge only after code review and CI. Distinguish compile success from runtime proof and report any tests still pending. Do not silently change encoder settings, output behavior, stream order, TikTok methods, camera capture or saved profiles.

## Current component inventory

| Component | Current source/version | Ownership and rule |
|---|---|---|
| OBS Studio | **33.0.3**, Windows x64 | Current validated source/build target. Check official stable release before any future upgrade. |
| WEST Vertical Canvas | **1.6.8**, westkerberos/obs-vertical-canvas, main merge 8c64a52994fd10fbc2414ea7bf98e7ee02975f42 | Vertical scenes/outputs, EBC portrait encoder reuse, TikTok session/runtime, safety and Dual LIVE. |
| WEST Aitum Multistream | **1.0.10**, westkerberos/obs-aitum-multistream, main merge 6d4c0922f60ddd0012778c69dc7584a0cef55d15 | Main Canvas destinations, Twitch EBC negotiation/output, Vertical destination bridge and TikTok configuration handoff. |
| OBS DirectShow patch | OBS **33.0.3** win-dshow.dll, westkerberos/ChatGPT, main merge ae7cc2dbcf71a3f6be74579577d47fd90fa452b3 | Native MJPEG camera-to-MOV recorder inside OBS core's Windows DirectShow module. Rebase/rebuild against each exact OBS tag. |
| DroidCam OBS (separate/optional) | **2.5.1**, official Dev47Apps release | Not the DirectShow patch. The 27 Nov startup log reported module release 251; do not rebuild if still current and compatible. |

Official versions checked on 27 Nov 2026:
- Aitum Vertical **1.6.8** (released 18 Nov; includes Safe Areas option and scene-loading hang fix).
- Aitum Multistream **1.0.10** (released 25 Nov).
- OBS stable target **33.0.3**.
- DroidCam OBS **2.5.1**, with an OBS 33-specific installer.

## Successful Windows x64 build artifacts

| Component | Workflow / artifact | SHA-256 |
|---|---|---|
| Vertical Canvas 1.6.8 | [CI #46036075188](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/46036075188), artifact ID 15217901109, vertical-canvas-1.6.8-windows-x64-8c64a5299 | Outer artifact ZIP: be84f005d1ad0046a24313153c36ed8428124e898d8ca7e206e63fb95600eee1; ProgramData ZIP: 58b617539bb7c2c1dee53db2e9a719f49f8453b68fb35e7a97cb94f39effe813; DLL: 3636abb099d425373911b507a8da344bc3169bd0e30d41827d6fbe735a8845db |
| Aitum Multistream 1.0.10 | [CI #46036486335](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/46036486335), artifact ID 15219040196, aitum-multistream-1.0.10-windows-x64-6d4c0922f | Outer artifact ZIP: f2d08b6b0832c336e80e4e65eb5a9dfc6f7f679f4c2ae3104d1273441c440d04; ProgramData ZIP: 5b16036008d5cb6781880c0dfd493e84a5c8c5512f2aeaec7b19b6bd6a1e69c3; DLL: 47a39f4236282b1bfbd58d28b22dd74ed6f1072446d63baabf4d1a891ddbe0a3 |
| OBS win-dshow 33.0.3 | [CI #46035805651](https://github.com/westkerberos/ChatGPT/actions/runs/46035805651), artifact ID 15215599998, obs-33.0.3-native-mjpeg-win-dshow | ZIP: 88dff9f2b12b735370d5f7928fef91a8c476627c67a43f4b2e813b3777cd9cc6; DLL: ebd7b77e9da422f9a6ed72bc4661c8387e8889d1bcc1d5efd1bac2fcf1ecf9ad |

These are compile/package results. They are not proof that every runtime feature has passed on the user's machine.

## Cross-plugin contracts — do not break

- Multistream owns Twitch EBC negotiation/output. Its output prefix is aitum_multi_ebc_output_.
- Vertical Canvas finds and reuses the active EBC portrait video encoder when a vertical target selects __aitum_twitch_ebc_vertical__. Legacy use_twitch_ebc_encoder=true profiles remain supported.
- Multistream's Vertical Canvas bridge includes aitum_vertical_configure_tiktok_output, aitum_vertical_get_stream_settings, aitum_vertical_start_stream_output and aitum_vertical_stop_stream_output.
- TikTok authentication, account/session, cookies/QR, signing, dynamic ingest, LIVE safety and Dual LIVE runtime belong only to Vertical Canvas.
- TikTok Dual LIVE Portrait uses the selected Vertical Canvas output encoders. Landscape reuses the active OBS Main Canvas video encoder and audio encoder 0.
- Recording, replay buffer and virtual camera must not accidentally inherit stream-only EBC encoders.

## Operator start order

1. Twitch EBC — wait for the connection and portrait track initialization.
2. YouTube Vertical.
3. YouTube Horizontal / Main Canvas.
4. Instagram Vertical.
5. TikTok Dual LIVE.

Two separate prerequisites: Twitch EBC supplies the portrait track; active Main Canvas supplies TikTok Landscape's horizontal video/audio encoders. Vertical Canvas has pre-flight prompts for missing EBC and missing Main Canvas, including Start All.

## Runtime evidence and open tests

- The 27 Nov OBS 33.0.3 log using Multistream 1.0.10 + Vertical Canvas 1.6.7 showed Twitch EBC creating the 1080×1920 track, Main Canvas starting, and TikTok Portrait + Landscape staying connected until manual stop.
- The exact merged Vertical Canvas 1.6.8 + Multistream 1.0.10 pair has successful Windows x64 CI but still needs an end-to-end runtime regression.
- The new pre-flight dialogs are in merged Vertical Canvas 1.6.8 source/build; verify both cancel and explicit fallback paths on the exact build.
- The DirectShow 33.0.3 patch builds, but actual 4K MJPEG capture, MOV integrity, Mic/Aux PCM audio and A/V sync still need camera-runtime testing.
- Test ten start/stop cycles, reconnect, one-feed failure, profile reload and a long-duration stream before calling the stack fully validated.

## Licensing/release gate

The TikTok-native implementation has unresolved source-license/provenance compatibility issues documented in LICENSING-NOTES.md and LICENSING-INVENTORY.md. Source merges and private engineering builds do not clear those issues. Do not publish or redistribute a combined binary or claim license clearance until the identified code is independently reimplemented/removed or the issue is otherwise independently cleared. Keep this warning visible in future documentation.

## Repositories

- [WEST Vertical Canvas](https://github.com/westkerberos/obs-vertical-canvas)
- [WEST Aitum Multistream](https://github.com/westkerberos/obs-aitum-multistream)
- [WEST OBS / DirectShow maintenance repository](https://github.com/westkerberos/ChatGPT)
- [Official Aitum Vertical](https://github.com/Aitum/obs-vertical-canvas)
- [Official Aitum Multistream](https://github.com/Aitum/obs-aitum-multistream)
- [Official OBS Studio](https://github.com/obsproject/obs-studio)
- [Official DroidCam OBS](https://github.com/dev47apps/droidcam-obs-plugin)
