# WEST Gaming Vertical Canvas for OBS Studio

A WEST Gaming-maintained fork of [Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas). It is the vertical-output and TikTok runtime half of the matched WEST Gaming OBS streaming stack.

**Current merged stack — 27 November 2026:** OBS Studio 33.0.3 · Vertical Canvas 1.6.8 · Aitum Multistream 1.0.10 · Windows x64.

## What this fork owns

- Vertical canvas, scenes, source layout and preview.
- Vertical output creation and encoder assignment.
- Twitch Enhanced Broadcasting portrait-video encoder lookup/reuse, including an independent fallback when explicitly selected.
- Native TikTok destination/runtime: Custom RTMP, LIVE Studio Desktop/Mobile, Streamlabs, account/session handling, signing, safety options and Dual LIVE.
- Pre-flight checks for missing Twitch EBC portrait track and missing Main Canvas output in TikTok Dual LIVE.
- Recording/replay/virtual-camera paths kept separate from stream-only encoder reuse.

The matching [WEST Aitum Multistream fork](https://github.com/westkerberos/obs-aitum-multistream) owns Twitch EBC negotiation/output and the Multistream vertical-destination bridge. Do not replace either WEST fork with its official upstream binary; that would remove custom integrations.

## Current merged integration

- Vertical Canvas: [1.6.8 main branch](https://github.com/westkerberos/obs-vertical-canvas/tree/main) — Merged to main as commit `8c64a52994fd10fbc2414ea7bf98e7ee02975f42`; the post-merge Windows x64 build passed in [CI #46036075188](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/46036075188), artifact `vertical-canvas-1.6.8-windows-x64-8c64a5299`.
- Multistream: **1.0.10**, merged to main as commit `6d4c0922f60ddd0012778c69dc7584a0cef55d15` ([PR #2](https://github.com/westkerberos/obs-aitum-multistream/pull/2)); final Windows x64 build passed in [CI #46035894193](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/46035894193), artifact `aitum-multistream-1.0.10-windows-x64-3c582ebb5`.
- OBS DirectShow native MJPEG patch: merged into [maintenance repo main](https://github.com/westkerberos/ChatGPT/commit/ae7cc2dbcf71a3f6be74579577d47fd90fa452b3); the OBS 33.0.3 Windows x64 build passed in [CI #46035805651](https://github.com/westkerberos/ChatGPT/actions/runs/46035805651).
- DroidCam OBS: official 2.5.1 is current; the startup log reports release 251. Use the [OBS 33-specific 2.5.1 installer](https://github.com/dev47apps/droidcam-obs-plugin/releases/download/2.5.1/DroidCamOBS.Setup.2.5.1.obs33_1.exe) if reinstalling.

The exact commit/version matrix, encoder routing, component ownership, start order, runtime evidence and pending tests live in the [WEST Gaming Streaming Stack source of truth](docs/WEST-GAMING-STREAMING-STACK.md).

## Operator start order

1. Start Twitch EBC and wait for connection/track creation.
2. Start YouTube Vertical.
3. Start YouTube Horizontal/Main Canvas.
4. Start Instagram Vertical.
5. Start TikTok Dual LIVE.

TikTok Portrait can reuse the Twitch EBC 1080×1920 track. TikTok Landscape reuses the active OBS Main Canvas video/audio encoders. These are separate prerequisites; Twitch alone does not satisfy the Main Canvas requirement. The plugin now displays specific prompts instead of silently falling back when those dependencies are missing.

## Documentation

- [WEST Gaming streaming stack — source of truth](docs/WEST-GAMING-STREAMING-STACK.md)
- [Repeatable OBS/plugin update procedure](docs/UPDATE-PROCEDURE.md)
- [Build and runtime verification](docs/BUILD-AND-TESTING.md)
- [TikTok LIVE setup and Dual LIVE](docs/TIKTOK-LIVE.md)
- [Upstream differences](docs/UPSTREAM-DIFFERENCES.md)
- [Source and licensing review](docs/LICENSING-NOTES.md) · [File-level inventory](docs/LICENSING-INVENTORY.md)

## Distribution status

The source branches and Windows CI artifacts are engineering/test builds, not a stable public WEST Gaming release. The TikTok-native source still has unresolved licensing/provenance questions described in the licensing documents. Do not publish or redistribute a combined binary or claim license clearance until those issues are remediated or independently cleared. This is not legal advice.

## Upstream

- [Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas)
- [Aitum Multistream](https://github.com/Aitum/obs-aitum-multistream)
- [OBS Studio](https://github.com/obsproject/obs-studio)
