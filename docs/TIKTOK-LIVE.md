# TikTok LIVE in WEST Gaming Vertical Canvas

This guide describes the WEST Gaming TikTok destination/runtime in Vertical Canvas. The matching WEST Gaming Aitum Multistream fork provides the configuration/list/start-stop bridge and Twitch EBC output; it does not implement a second TikTok runtime.

**Current target:** OBS 33.0.3 · Vertical Canvas 1.6.8 · Aitum Multistream 1.0.10. See [the full stack source of truth](WEST-GAMING-STREAMING-STACK.md) and [update procedure](UPDATE-PROCEDURE.md) before replacing either plugin.

## Connection methods

| Method | Credentials / endpoint | What it does |
|---|---|---|
| Custom RTMP / stream key | Server URL and stream key | Uses the normal RTMP output path; does not create a TikTok-native room through the LIVE Studio session flow |
| TikTok LIVE Studio — Desktop | RapidAPI signing key and cookie file, or QR login | Creates/resumes a TikTok LIVE session, obtains dynamic ingest details and manages session lifecycle |
| TikTok LIVE Studio — Mobile | RapidAPI key, cookie file or QR login, mobile device profile | Uses mobile request/device profile; Camera LIVE and screen capture are different modes |
| Streamlabs Desktop | Streamlabs access token | Uses Streamlabs account/category and stream flow |

The editor also supports LIVE title/topic/game selection, gifts/comments, content disclosure, mature-content settings, account refresh, quota feedback and safety/violation workflows where supported.

## Recommended destination start sequence

The current stack has two separate encoder prerequisites. Use this order:

1. Start Twitch EBC; wait until it is connected and its portrait tracks are created.
2. Start YouTube Vertical so it can reuse the 1080×1920 EBC portrait video encoder.
3. Start YouTube Horizontal (or another Main Canvas horizontal output).
4. Start Instagram Vertical if it is configured to reuse EBC.
5. Start TikTok Dual LIVE.

YouTube Vertical and Instagram only depend on Twitch EBC when their selected video encoder is set to reuse its portrait track. TikTok Dual LIVE has the additional Main Canvas dependency described below.

## Credentials and local data

- The RapidAPI key is masked in the editor. Masking is display-only, not encryption at rest.
- The OBS/plugin profile stores TikTok connection settings and the cookie-file path. Protect the profile and cookie file like credentials.
- Mobile device/install identifiers and device profiles are persisted so reconnects can reuse the local identity.
- Never publish cookie files, RapidAPI keys, Streamlabs tokens, full OBS profiles or logs containing credentials/account identifiers.
- Account refresh and eligibility checks make network requests but do not start a LIVE.

## Dual LIVE — Portrait + Landscape

Dual LIVE requires an eligible TikTok account and a supported LIVE Studio method. The documented paired-feed modes are:
- TikTok LIVE Studio Desktop.
- TikTok LIVE Studio Mobile **Camera LIVE**.

Mobile screen capture, Streamlabs and Custom RTMP do not use this paired-feed mode.

### Automatic encoder reuse

The default option reuses existing OBS encoders instead of defining a second set of video/audio settings:

| TikTok feed | Encoder source | Required state |
|---|---|---|
| Portrait | Video/audio encoders already assigned to the Vertical Canvas TikTok output | Vertical output must have its video encoder attached. For Twitch EBC reuse, start Twitch first and wait for the 1080×1920 portrait track |
| Landscape | Video encoder and audio encoder 0 from OBS's primary Main Canvas streaming output | Start a horizontal Main Canvas stream first |

These prerequisites are independent. Twitch EBC being active does not satisfy the Main Canvas requirement. YouTube horizontal/Main Canvas being active does not create Twitch EBC's portrait track.

### Pre-flight warnings

Vertical Canvas now checks these conditions before starting:
- **Twitch EBC missing:** if a destination is configured to reuse EBC but no active portrait track is found, the dialog recommends starting Twitch first. The user can cancel or explicitly continue with an independent vertical encoder; continuing may add NVENC load.
- **Main Canvas missing:** if TikTok automatic Dual LIVE cannot obtain the horizontal Main Canvas output, OBS shows a specific warning explaining that a horizontal stream must be started first.

The EBC pre-flight also runs for Start All. The default choice is the safe/cancel path rather than silently creating an extra encoder.

### Advanced manual override

Turn off the automatic reuse option to configure independent portrait/landscape layouts. The manual editor supports separate scenes/canvases, width/height, video encoders, bitrate, frame-rate divisor, Fit/Crop framing, shared/independent audio and encoder properties. Use it only when you want layouts different from your existing Vertical and Main Canvas outputs.

### Failure and stop behaviour

Portrait and Landscape use separate RTMP output transports. A landscape transport failure is logged and can be retried by OBS; it no longer forcibly ends the portrait output. This avoids turning a secondary-feed failure into a full TikTok LIVE teardown. It also means a session can temporarily be **portrait-only** while Landscape is reconnecting or stopped. Confirm both feeds in TikTok; an active OBS portrait output alone does not prove Landscape is delivering.

A deliberate user stop of the TikTok destination still tears down the managed LIVE session and its feeds. Check the OBS log for the specific RTMP send/connect error before changing codec, bitrate or ingest settings.

## What this integration does not do

- It does not replace OBS's global Main Canvas streaming service.
- It does not make Multistream a second TikTok signing/session engine.
- It does not make every TikTok account eligible for Dual LIVE or guarantee ingest availability.
- It does not create an additional third encoder set in automatic mode.
- It does not make an unsigned CI artifact a production installer.

## Verification status

The 27 November 13:01 OBS 33.0.3 log used Multistream 1.0.10 and Vertical Canvas 1.6.7. Twitch EBC started first; the 1080×1920 encoder was found/reused; YouTube horizontal/Main Canvas started; TikTok Portrait and Landscape then connected and stayed active until manual stop with no TikTok RTMP send errors in that session.

Vertical Canvas 1.6.8 and the paired Multistream 1.0.10 branch have since passed Windows x64 compile/package CI. The exact 1.6.8 pair still needs an OBS runtime regression test, including the optional Safe Areas checkbox, all connection methods, Dual LIVE failure/reconnect and a longer soak. CI success alone is not runtime proof.

## Troubleshooting checklist

When TikTok fails to start, collect:
- OBS version and Windows version.
- Multistream and Vertical Canvas versions/commit SHAs.
- Which output was active first: Twitch EBC, YouTube Vertical, Main Canvas horizontal, Instagram, TikTok.
- Whether the error was EBC-unavailable or Main-Canvas-unavailable.
- Whether the failing feed was Portrait or Landscape.
- The relevant redacted OBS log section.

Do not attach cookie files, API keys, stream keys or unredacted OBS profiles.
