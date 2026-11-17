# WEST Gaming — Aitum Vertical Canvas for OBS Studio

A WEST Gaming-maintained fork of [Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas), built on the upstream native multi-canvas plugin and extended with TikTok LIVE output support.

This repository is the WEST Gaming-maintained Vertical Canvas fork. It owns the TikTok destination, connection UI and runtime. The matching [Aitum Multistream fork](https://github.com/westkerberos/obs-aitum-multistream) is an optional wizard/dock bridge; it does not implement TikTok connectivity.

> **Development status — 17 November 2026:** the TikTok integration is on [PR #1](https://github.com/westkerberos/obs-vertical-canvas/pull/1), branch `feature/tiktok-native-output`. The default `main` branch does not contain these additions until the PR is merged. The TikTok-native code also has an unresolved GPL-2 / GPL-3.0-only compatibility issue; see [licensing review](docs/LICENSING-NOTES.md). Do not redistribute a combined binary until that issue is resolved. There is no GitHub Release for this fork yet. The current Windows x64 workflow produces test artifacts, not a stable public release.

## Who should use this fork

Use this project when you already use **Aitum Vertical Canvas** and want TikTok as an output managed by the vertical canvas workflow, especially when you want Portrait to reuse your existing Vertical Canvas encoders and Landscape to reuse the active OBS Main Canvas encoders.

This fork does not provide TikTok as a standalone OBS Main Canvas streaming provider. It integrates TikTok with Vertical Canvas. To add and start the destination from the Aitum Multistream dock, install this plugin together with the matching [obs-aitum-multistream](https://github.com/westkerberos/obs-aitum-multistream) bridge.

## What this fork adds

The upstream Vertical Canvas canvas/scene workflow remains the foundation. WEST Gaming adds TikTok connection and output management inside the Vertical Canvas output lifecycle:

- **Connection methods:** Custom RTMP, TikTok LIVE Studio Desktop, TikTok LIVE Studio Mobile and Streamlabs Desktop.
- **TikTok account setup:** cookie import, QR login, account refresh, mobile device registration and saved device profiles.
- **LIVE configuration:** title, topic, searchable game selection, gifts/comments, content disclosure and mature-content settings.
- **Account feedback:** Streamlabs status/category information, TikTok account identity, Dual LIVE eligibility and RapidAPI quota feedback.
- **LIVE safety:** optional violation monitoring, manual violation/appeal lookup and account-verified appeal submission where TikTok exposes the required action.
- **Dual LIVE:** portrait and landscape feeds from one TikTok LIVE session, with coordinated startup/shutdown.
- **Default Dual LIVE encoder reuse:** Portrait inherits the existing Vertical Canvas output encoders; Landscape inherits the OBS Main Canvas streaming encoders. No manual per-layout encoder setup is required in this mode.
- **Advanced Dual LIVE override:** optional separate scenes, canvas dimensions, video/audio encoders, bitrate, frame-rate divisor, Fit/Crop, preview and encoder properties.
- **Aitum Multistream bridge:** the matching WEST Gaming Aitum Multistream fork can open the TikTok configuration dialog for a Vertical Canvas destination without duplicating the TikTok runtime.

The implementation and user workflow are documented in [TikTok LIVE](docs/TIKTOK-LIVE.md). See [Upstream differences](docs/UPSTREAM-DIFFERENCES.md) for what is inherited and what this fork adds.

## What this fork does not do

- It does not replace OBS's ordinary Main Canvas streaming service or turn TikTok into a standalone Main Canvas provider.
- It does not require a separate Multi-RTMP plugin. The maintained product pair is this Vertical Canvas runtime plus the optional Aitum Multistream configuration bridge.
- It does not move TikTok session/signing/safety logic into Multistream or create a second copy of that runtime.
- Automatic Dual LIVE does not clone every horizontal scene/source into a new vertical composition. It reuses the already configured Vertical Canvas output for Portrait and the active Main Canvas stream encoders for Landscape. Scene composition remains configured through OBS and Vertical Canvas.
- It does not guarantee account eligibility, LIVE approval, endpoint availability or policy outcomes controlled by TikTok.

## Dual LIVE: recommended configuration

The checkbox **Reuse the existing Vertical Canvas and Main Canvas settings (recommended)** is enabled by default.

| Feed | Encoder source |
| --- | --- |
| Portrait | The video/audio encoders already assigned to the TikTok destination in Vertical Canvas. |
| Landscape | The video/audio encoders attached to OBS's primary Main Canvas streaming output. |

Start the normal horizontal Main Canvas stream (for example, YouTube or Twitch) **before** starting TikTok Dual LIVE. This gives the landscape feed the real active horizontal encoders and settings. The plugin does not create a second set of manual encoders in this default mode or modify the Main Canvas encoder configuration.

Dual LIVE requires an eligible TikTok account and supports LIVE Studio Desktop or Mobile **Camera LIVE**. Streamlabs, Custom RTMP and Mobile screen capture do not use this paired-feed mode. Turn off the reuse checkbox only when you intentionally want the advanced manual layout editor.

## Documentation

- [TikTok LIVE setup and Dual LIVE guide](docs/TIKTOK-LIVE.md)
- [What changed from upstream Aitum Vertical Canvas](docs/UPSTREAM-DIFFERENCES.md)
- [Build, CI and runtime testing status](docs/BUILD-AND-TESTING.md)
- [Third-party licensing review notes](docs/LICENSING-NOTES.md)
- [Upstream translations guide](TRANSLATIONS.md)

## Requirements and current limitations

- OBS Studio with native canvas support. The owner has tested this integration in a Windows OBS 33.0.1 environment; this is not yet a published minimum-version or cross-platform support matrix.
- TikTok LIVE Studio/Mobile requires a RapidAPI signing key and an authenticated TikTok cookie session. Streamlabs requires its own access token. Custom RTMP requires a user-supplied endpoint and key.
- Dual LIVE additionally requires TikTok account eligibility and an active OBS Main Canvas streaming output when automatic encoder reuse is selected.
- The current fork workflow builds **Windows x64 only**. Linux and macOS jobs are not enabled.
- Test packages are unsigned while signing secrets are unavailable.
- TikTok account eligibility, endpoint availability, policy decisions and LIVE/appeal outcomes remain controlled by TikTok.

## Install and release status

There is currently no GitHub Release published for this fork. Do not mistake the upstream Aitum installer, Flatpak or website downloads for a WEST Gaming build: they do not automatically contain this fork's TikTok additions.

For development, use the branch and exact commit identified in [PR #1](https://github.com/westkerberos/obs-vertical-canvas/pull/1), or the corresponding GitHub Actions artifact linked from the successful workflow in [Build and testing](docs/BUILD-AND-TESTING.md). That guide records the latest verified Windows x64 workflow and artifact. Do not install an unreviewed CI artifact into a critical production OBS profile.

## Build

The standalone CMake target links against OBS libobs and obs-frontend-api, Qt 6 Core/Widgets and libcurl. The TikTok additions also use qrcodegencpp and the platform cryptographic library. The project is C++20.

With the required OBS development packages available:

```sh
cmake -S . -B build -DBUILD_OUT_OF_TREE=ON
cmake --build build --config Release
```

For CI status, packaging details, and what has and has not been runtime-tested, see [Build and testing](docs/BUILD-AND-TESTING.md).

## Attribution and licensing

This is a fork of Aitum's GPL-licensed Vertical Canvas project. Only the TikTok authentication portion was borrowed and adapted from [Loukious/obs-multi-rtmp](https://github.com/Loukious/obs-multi-rtmp), itself downstream of [sorayuki/obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp). The native TikTok client/signing/runtime components carry separate direct attribution to [Smile-4-Me/tiktok-live-obs](https://github.com/Smile-4-Me/tiktok-live-obs). The broader OBS Multi-RTMP plugin is not required or included in the maintained runtime architecture. The exact notices and license texts, including the QR generator attribution, are recorded in [data/licenses](data/licenses/).

**Maintainer action before publishing a release:** the repository's main LICENSE is the GNU GPL version 2 text, while the referenced TikTok-native source is explicitly identified as GPL-3.0-only. Review the compatibility and the original source's exact licensing grant before distributing a combined binary. Do not change the repository's license or remove third-party notices without confirming the rights to do so.

## Support and issue reports

This is the WEST Gaming fork. Do not send fork-specific bugs to Aitum's upstream support channels as though the upstream build contains these changes. When opening an issue, include the exact commit/build, OBS version, connection method and a redacted OBS log excerpt.

Never upload TikTok cookies, RapidAPI keys, Streamlabs tokens, account identifiers or an unredacted OBS profile.