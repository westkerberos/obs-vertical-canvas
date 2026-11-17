# TikTok LIVE in WEST Gaming's Vertical Canvas fork

This guide describes the TikTok integration in the WEST Gaming fork of Aitum Vertical Canvas. Only the TikTok authentication portion was borrowed and adapted from [Loukious/obs-multi-rtmp](https://github.com/Loukious/obs-multi-rtmp), a downstream fork of [sorayuki/obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp). The native TikTok client/runtime components carry separate direct attribution to [Smile-4-Me/tiktok-live-obs](https://github.com/Smile-4-Me/tiktok-live-obs). WEST Gaming adapted the connection UI and the runtime/output lifecycle to OBS/Qt and Vertical Canvas, including canvas/encoder ownership and Dual LIVE. The broader OBS Multi-RTMP plugin is not a runtime dependency. The maintained setup is Vertical Canvas plus the optional Aitum Multistream wizard bridge.

> **Branch status (17 November 2026):** the integration is developed in [PR #1](https://github.com/westkerberos/obs-vertical-canvas/pull/1), branch `feature/tiktok-native-output`. Until that PR is merged, the default `main` branch and the upstream Aitum downloads do not contain this fork's TikTok changes. There is no GitHub Release for this fork yet.

## What the integration adds

A Vertical Canvas output can be configured to publish to TikTok using one of four connection methods:

| Method | Credentials / endpoint | What the plugin does |
| --- | --- | --- |
| Custom RTMP / stream key | Server URL and stream key supplied by the user | Uses the normal RTMP output path. No TikTok account lookup or native LIVE room creation is performed by this method. |
| TikTok LIVE Studio — Desktop | RapidAPI signing key and a TikTok cookie file, or QR login | Creates or resumes a TikTok LIVE session, obtains the ingest endpoint and key, signs video packet metadata and manages the session lifecycle. |
| TikTok LIVE Studio — Mobile | RapidAPI signing key and a TikTok cookie file, or QR login; mobile device profile | Uses the mobile request profile and device identity. Camera LIVE and screen capture are separate broadcast modes. |
| Streamlabs Desktop | Streamlabs Desktop access token | Starts and ends a Streamlabs-managed TikTok stream and can look up the account/category information. |

The native LIVE Studio and Mobile methods use the hosted signing service configured by the RapidAPI key. A cookie file is required for the TikTok account session. A QR login can create a new cookie export and select it in the editor.

## Configuring an output

1. Add or edit a TikTok destination in the Vertical Canvas output workflow. When using the matching Aitum Multistream fork, open the **Vertical Canvas** settings tab and add a new TikTok output there.
2. Choose the connection method.
3. For LIVE Studio Desktop or Mobile, enter the RapidAPI key and select a readable TikTok cookie file. Use **Login with TikTok QR** to authenticate and save a new cookie export instead.
4. Select the LIVE topic. When the selected topic requires a game, choose a game from the searchable list. Do not type an arbitrary game name and expect it to be accepted; selections are tied to TikTok's returned IDs.
5. Set the LIVE title, gifts/comments permissions, content disclosure and mature-content option as required.
6. Use **Refresh account** to repeat the account lookup after changing credentials or cookies. The account panel shows the resolved TikTok username and, where available, Dual LIVE eligibility.
7. Save the destination. The editor validates required credentials, cookie readability, topic/game selection and mobile device settings before accepting the configuration.

### Credentials and local data

- The RapidAPI key is masked in the editor. Masking is a display behavior, not encryption at rest.
- The destination stores the selected cookie-file path and its TikTok connection settings in OBS/plugin configuration. Protect the OBS profile and the cookie file as credentials.
- Mobile device/install identifiers and related device-profile fields are persisted so reconnects can reuse the same local identity. Leave both IDs blank for automatic registration; do not enter only one of the pair.
- Never publish a cookie file, RapidAPI key, Streamlabs token, full OBS profile or unredacted log in a GitHub issue.
- Account refresh and eligibility checks make network requests but do not themselves start a LIVE.

## TikTok account, LIVE metadata and safety

For native LIVE Studio and Mobile connections, the editor provides:

- Account username lookup and refresh.
- Topic and game selection.
- Gifts and comments toggles.
- Content disclosure: none, own brand, third-party brand, or both.
- Mature-content setting.
- RapidAPI quota feedback based on observed response headers.
- Device/profile setup for the selected client.
- LIVE violation monitoring, manual violation/appeal lookup and account-verified appeal actions where TikTok exposes the required route.

The violation monitor starts after the LIVE has been confirmed as started. Manual checking is also available from the configuration dialog. Monitoring and appeal requests use the same signing credentials and may consume the signing service's quota. An appeal is an explicit user action; the plugin does not automatically submit appeals.

Streamlabs uses its own token, account lookup and category flow. TikTok-native room controls such as gifts/comments/content disclosure are not exposed by Streamlabs in this integration.

## Dual LIVE: portrait + landscape

Dual LIVE is optional and requires TikTok to report that the account is eligible. It is supported by LIVE Studio Desktop and Mobile **Camera LIVE**. Mobile screen capture, Streamlabs and Custom RTMP do not use this paired-feed mode.

One TikTok LIVE session supplies two distinct ingest endpoints. Vertical Canvas coordinates the two output feeds and stops the pair if the secondary feed fails unexpectedly.

### Recommended mode: reuse the existing canvas encoders

The checkbox **Reuse the existing Vertical Canvas and Main Canvas settings (recommended)** is enabled by default.

In this mode the plugin inherits the two encoder configurations already in use:

| TikTok feed | Source of settings | Practical result |
| --- | --- | --- |
| Portrait | The video/audio encoders already assigned to the TikTok output on the Vertical Canvas | Keeps the current vertical canvas scene, resolution, video/audio encoder and encoder settings. No separate portrait profile needs to be configured in the Dual LIVE panel. |
| Landscape | OBS's primary Main Canvas streaming output, returned by OBS's frontend streaming-output API | Reuses the horizontal stream's current video/audio encoder objects and their settings, such as the configuration used by the normal YouTube or Twitch stream. |

**Start the Main Canvas stream first**, then start the TikTok Dual LIVE output. The automatic mode needs the Main Canvas streaming output to be active so it can obtain the actual horizontal video and audio encoders. If it is not active or does not have both encoders assigned, startup stops with an explanatory error.

This mode shares the existing encoder objects with the TikTok transports; it does not build another set of manual per-layout encoder profiles. It also does not change the Main Canvas stream's encoder settings.

### Advanced manual override

Turn off the reuse checkbox to expose the manual layout editor. The manual mode can define separate portrait and landscape scenes/canvases, dimensions, video encoders, bitrate, frame-rate divisor and Fit/Crop framing. Audio can be shared or configured independently, including encoder, mixer and bitrate. It also provides layout preview and the OBS encoder-property editor.

Use manual mode only when the inherited Vertical Canvas and Main Canvas configurations are not the layouts you want to send to TikTok. It is not necessary for the default WEST Gaming setup.

### Dual LIVE constraints

- TikTok account eligibility must be confirmed; an unknown or locked state does not enable Dual LIVE.
- Mobile Dual LIVE requires **Camera LIVE**, not screen capture.
- The Main Canvas stream must be active in automatic encoder-reuse mode.
- The two feeds need separate valid ingest endpoints from the same TikTok LIVE session.
- If either transport fails terminally, the pair is stopped and the managed LIVE session is cleaned up.
- A successful OBS start request is not itself proof that TikTok is receiving or displaying both feeds; confirm both layouts in TikTok during the live test.

## What this integration does not do

- It does not replace OBS's global streaming service or take ownership of the ordinary Main Canvas stream.
- It does not turn Aitum Multistream into a second TikTok API/signing engine. The matching Multistream fork only invokes the Vertical Canvas configuration bridge.
- It does not offer Dual LIVE for Streamlabs, Custom RTMP or Mobile screen capture.
- It does not guarantee TikTok account eligibility, approval of a LIVE, or uninterrupted ingest. Those decisions and service availability remain on TikTok's side.
- It does not make an unsigned test build equivalent to a signed production installer.

## Current verification status

The latest verified PR build compiled and packaged successfully for Windows x64 in [GitHub Actions run #44163406280](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/44163406280). The workflow artifact is `vertical-canvas-1.6.7-windows-x64-017a3e5eb`. The automatic encoder-reuse runtime change was first compiled at `cb22d71497d2e025bfad686cab8079937f7f6387`; the later commits update documentation and third-party notices.

On 17 November 2026, the repository owner completed an approximately 30-minute maintainer smoke test in OBS 33.0.1 on Windows. Landscape, Portrait and Dual LIVE were all tested and reported working correctly. The separate OBS Multi-RTMP plugin was disabled in OBS during the test; the integration still worked, confirming it is not a runtime dependency. This is evidence for the tested Windows/OBS setup, not a published cross-platform compatibility matrix or an extended soak test. QR login, other operating systems, extended reconnect soak tests and every appeal edge case should be regression-tested before a general release.

The workflow currently builds Windows x64 only. Code signing is skipped because signing secrets are not configured in this fork; the generated test package is unsigned. Ubuntu and macOS jobs are not enabled.

## Reporting a problem

Include:
- OBS version and Windows version.
- Whether the target uses Custom RTMP, LIVE Studio Desktop, LIVE Studio Mobile or Streamlabs.
- Whether the problem occurs in single-feed or Dual LIVE mode.
- Whether the Main Canvas stream was already active when automatic Dual LIVE was started.
- The relevant OBS log section with cookies, keys, tokens, account IDs and personal data removed.

Do not attach cookie files or API keys.
