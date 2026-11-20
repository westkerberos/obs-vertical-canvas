# WEST Gaming Vertical Canvas for OBS Studio

A WEST Gaming-maintained fork of [Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas) for OBS Studio. The project focuses on native multi-canvas streaming and an experimental TikTok LIVE integration with Portrait, Landscape and Dual LIVE workflows.

**Status: experimental development — no stable WEST Gaming release is published.**

The TikTok integration is being developed in [Pull Request #1](https://github.com/westkerberos/obs-vertical-canvas/pull/1), branch `feature/tiktok-native-output`. That branch has passed a Windows x64 build and a maintainer smoke test in OBS 33.0.1 on 18 November 2026. The maintainer tested Portrait, Landscape and Dual LIVE for approximately 30 minutes, with the separate OBS Multi-RTMP plugin disabled. This is one Windows setup, not a broad compatibility matrix.

## What the TikTok development branch contains

- TikTok LIVE destinations managed by Vertical Canvas.
- Custom RTMP, TikTok LIVE Studio Desktop/Mobile and Streamlabs connection paths.
- Portrait and Landscape output, plus a Dual LIVE mode that reuses Vertical Canvas and OBS Main Canvas encoders.
- Account setup, LIVE metadata and safety-related UI.

The matching [WEST Gaming Aitum Multistream fork](https://github.com/westkerberos/obs-aitum-multistream) adds an optional configuration-wizard bridge. It is not a second TikTok runtime and the original OBS Multi-RTMP plugin is not required to be enabled.

## Twitch Enhanced Broadcasting encoder reuse

The combined development branch also supports the WEST Gaming Aitum Multistream Twitch Enhanced Broadcasting path. Vertical destinations can opt to reuse the negotiated portrait EBC encoder; when the EBC track is unavailable, Vertical Canvas falls back to an independent vertical encoder. Recording and replay keep their separate encoder paths.

The TikTok-native session lifecycle remains enabled for both individual output starts and Start All, including dynamic TikTok ingest details and optional Dual LIVE.

The combined development branch also contains explicit C++20 lambda captures and Qt-safe QJsonArray iteration for strict Linux/macOS compiler configurations.

## Important: do not treat the development branch as a release

The current feature branch still contains third-party TikTok code with unresolved source-license compatibility and provenance questions. In particular, the audit found GPL-3.0-only-derived code compiled into the Aitum-derived GPL-2.0 OBS module, and a device-registration path whose cited source has no published license. The branch also contains an experimental relay protocol stub; that stub does **not** implement the TikTok session, RTMP transport, signing or Dual LIVE runtime.

Do not redistribute its combined binary or advertise it as license-cleared. No stable installer or GitHub Release is available yet. The project will only be promoted to a release after the code boundary/remediation, source audit, packaging and runtime regression tests are complete.

## Development links

- [TikTok LIVE setup and feature description](https://github.com/westkerberos/obs-vertical-canvas/blob/feature/tiktok-native-output/docs/TIKTOK-LIVE.md)
- [Build and testing status](https://github.com/westkerberos/obs-vertical-canvas/blob/feature/tiktok-native-output/docs/BUILD-AND-TESTING.md)
- [File-level source and license inventory](https://github.com/westkerberos/obs-vertical-canvas/blob/feature/tiktok-native-output/docs/LICENSING-INVENTORY.md)
- [Licensing architecture options](https://github.com/westkerberos/obs-vertical-canvas/blob/feature/tiktok-native-output/docs/LICENSING-ARCHITECTURE-OPTIONS.md)
- [TikTok relay implementation plan](https://github.com/westkerberos/obs-vertical-canvas/blob/feature/tiktok-native-output/docs/TIKTOK-REIMPLEMENTATION-PLAN.md)
- [Upstream Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas)

## Build

The upstream plugin uses CMake and the OBS Studio plugin development environment. Build instructions for the experimental TikTok branch are maintained in its [build and testing guide](https://github.com/westkerberos/obs-vertical-canvas/blob/feature/tiktok-native-output/docs/BUILD-AND-TESTING.md).

## Search terms

OBS Studio plugin, Aitum Vertical Canvas, vertical canvas, TikTok LIVE, TikTok streaming, OBS TikTok output, Dual LIVE, portrait streaming, landscape streaming, RTMP, WEST Gaming.
