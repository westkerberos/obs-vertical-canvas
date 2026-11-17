# WEST Gaming fork: changes relative to Aitum Vertical Canvas

This file separates the inherited Vertical Canvas project from the TikTok functionality added by WEST Gaming. It is a source/maintenance guide, not a claim that the changes have been merged into Aitum's upstream repository.

## Project identity

- Upstream project: [Aitum/obs-vertical-canvas](https://github.com/Aitum/obs-vertical-canvas)
- WEST Gaming fork: [westkerberos/obs-vertical-canvas](https://github.com/westkerberos/obs-vertical-canvas)
- TikTok integration PR: [#1 — Integrate TikTok LIVE into Vertical Canvas outputs](https://github.com/westkerberos/obs-vertical-canvas/pull/1)
- Development branch: `feature/tiktok-native-output`

The fork retains the Vertical Canvas plugin and adds a TikTok output provider/runtime inside the same Vertical Canvas module. It does not replace the existing canvas/scene product with an unrelated streaming engine.

## Inherited project area

The base Vertical Canvas functionality remains the upstream foundation: native OBS canvas handling, scene/source/transitions docks, canvas previews and the existing Vertical Canvas output lifecycle. These capabilities should not be described as new TikTok features or as code newly authored by WEST Gaming.

The existing Aitum website, Flatpak distribution, upstream Discord and upstream troubleshooting links refer to Aitum's project and releases. They do **not** automatically distribute or support the WEST Gaming TikTok changes.

## WEST Gaming additions

### TikTok output connection layer

The fork adds a provider configuration dialog and a runtime session integrated with Vertical Canvas outputs. The source is organized around:

- `src/tiktok-output-settings-dialog.*`: connection editor, credential validation, saved settings and configuration bridge.
- `src/tiktok-output-session.*` and `src/tiktok-live-session.*`: dynamic endpoint acquisition, TikTok room/session ownership, output coordination and packet signing.
- `src/tiktok-native/`: native account, client, QR, device, domain-routing, signing, quota and safety support.
- `src/account-info.*` and `src/account-info-widget.*`: account/device refresh and eligibility display.
- `src/tiktok-topic-widget.*` and `src/tiktok-game-info.*`: topic and game selection.
- `src/tiktok-safety.*`: violation monitoring and appeal UI.
- `src/tiktok-dual-layout-widget.*`, `src/tiktok-layout-scene.*` and `src/tiktok-encoder-properties-dialog.*`: Dual LIVE configuration, preview and advanced settings.
- `src/streamlabs-*.{cpp,h}` and `src/streamlabs-desktop.*`: Streamlabs connection helpers.

The CMake target adds the corresponding Qt, libcurl, QR-code and cryptographic dependencies needed by the TikTok integration.

### Connection methods

The output editor provides:
- Custom RTMP / stream key.
- TikTok LIVE Studio Desktop.
- TikTok LIVE Studio Mobile.
- Streamlabs Desktop.

These methods are intentionally different. Native LIVE Studio/Mobile uses a cookie-backed account session and hosted signing service; Streamlabs uses a Streamlabs token; Custom RTMP uses a supplied server/key and does not create a TikTok-native room through the native session code.

### Account, metadata and session additions

The fork adds account refresh, device-profile persistence, QR login, topic/game metadata, gifts/comments settings, content disclosure and mature-content controls. It also adds RapidAPI quota feedback and LIVE violation/appeal controls.

A saved cookie path, signing key, connection method, mobile profile, safety preference and Dual LIVE settings are retained with the TikTok destination. The normal Vertical Canvas scene/canvas data remains the host application's responsibility.

### Dual LIVE architecture

The earlier manual Dual LIVE editor supports two selected layouts with separate scene/canvas, resolution, encoders, bitrate, frame divisor and Fit/Crop choices, plus shared or independent audio.

WEST Gaming subsequently added a default automatic reuse mode:

- Portrait reuses the encoder objects already assigned to the Vertical Canvas TikTok output.
- Landscape reuses the video/audio encoder objects from OBS's primary Main Canvas streaming output.
- The Main Canvas stream must already be active for automatic mode.
- Manual layout configuration remains available as an advanced override.

This is specific to the WEST Gaming Vertical Canvas implementation. The earlier Multi-RTMP source was only a donor for the TikTok integration port; it is not maintained as a separate WEST Gaming product or supported runtime.

### Aitum Multistream bridge

The Vertical Canvas fork exposes the OBS proc-handler call `aitum_vertical_configure_tiktok_output`. The matching WEST Gaming Aitum Multistream fork calls this bridge to open the TikTok configuration dialog for a Vertical Canvas target.

This is a deliberate ownership boundary:
- Vertical Canvas owns TikTok account/session/signing/heartbeat/safety, dynamic endpoints, output encoders and Dual LIVE runtime.
- Aitum Multistream owns its destination wizard and dock presentation.
- The Multistream fork must not implement a second TikTok session or create a duplicate encoder path.

## What was not changed into something else

- This fork is not a replacement for the OBS built-in Main Canvas streaming provider.
- It is not a general-purpose TikTok API SDK.
- It does not require or install a separate Multi-RTMP plugin.
- It does not make Aitum's official website downloads, Flatpak package or upstream release artifacts include WEST Gaming's changes.

## TikTok integration source and attribution

The TikTok integration was ported and adapted from the TikTok implementation in [Loukious/obs-multi-rtmp](https://github.com/Loukious/obs-multi-rtmp), a downstream fork of [sorayuki/obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp). The native TikTok client/signing components also carry a direct upstream attribution to [Smile-4-Me/tiktok-live-obs](https://github.com/Smile-4-Me/tiktok-live-obs), including its stated GPL-3.0-only license. The exact notices and license texts are retained in `data/licenses/`.

WEST Gaming's changes in this repository are the OBS/Qt integration, connection/settings UI, Vertical Canvas output lifecycle, account and device interface, safety/appeal interface, canvas and encoder ownership, automatic Portrait/Main Canvas encoder reuse, advanced Dual LIVE override, and the configuration bridge used by the companion Aitum Multistream fork.

There is no separate Multi-RTMP plugin in the maintained product architecture. End users install the WEST Gaming Vertical Canvas fork and, when they want the configuration wizard in the Multistream dock, the matching WEST Gaming Aitum Multistream bridge. The upstream Multi-RTMP repositories are source provenance, not required runtime dependencies or supported install targets.

## How to read the upstream comparison

The repository is a fork of [Aitum/obs-vertical-canvas](https://github.com/Aitum/obs-vertical-canvas). The feature branch is not yet merged into the WEST Gaming `main` branch. Its current head `03c83c3a991265a2c8a445d3fc99c26708d53687` passed the Windows x64 CI/package workflow [#44159137647](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/44159137647).

The exact changed-file, addition and deletion totals are intentionally not duplicated here because they change as the PR is updated. Use the live [PR #1 Files view](https://github.com/westkerberos/obs-vertical-canvas/pull/1/files) for current totals. Those PR-level totals are not a claim that every changed line is newly authored by WEST Gaming: the branch retains upstream Aitum code and includes third-party adaptations. The source-provenance and licensing sections below distinguish those contributions.

The practical source-level additions are the native TikTok client/session/signing/safety modules, connection and account widgets, Streamlabs helpers, dual-layout controls/output session, localization, build dependencies and the companion Multistream configuration bridge. The existing canvas/scene plugin remains the upstream foundation.

## Compatibility and release status

The TikTok work is on the feature branch and open PR noted above. The default `main` branch does not contain it until the PR is merged. The repository had no GitHub Release at the time this document was updated (17 November 2026).

The current CI workflow builds Windows x64. Linux/macOS jobs are not enabled in this fork, and Windows signing is skipped when signing secrets are unavailable. Do not advertise a broader operating-system support matrix without adding and testing those builds.

## Maintenance rules

1. Keep TikTok session/runtime ownership in Vertical Canvas.
2. Keep the Aitum Multistream integration as a thin configuration bridge.
3. Never describe the automatic Dual LIVE mode as a third independent encoder set: it reuses the current Vertical Canvas and Main Canvas encoders.
4. Keep Custom RTMP, Streamlabs and native LIVE Studio/Mobile credential flows distinct.
5. When a new TikTok capability is added, document its method restrictions, required credentials, persisted settings, failure behavior and actual runtime test status.
6. Keep upstream support and WEST Gaming fork support clearly separated.
