# RTMP media transport decision — TikTok Relay

Status: MediaMTX is the selected authenticated-ingest candidate for the next proof of concept. No MediaMTX binary has yet been run in this development environment, and no OBS/Windows integration is implemented.

## Decision

Use **MediaMTX as the local authenticated RTMP ingest server** and keep FFmpeg's C API inside the separate WEST TikTok Relay process as an RTMP reader/packet-processing client.

The intended flow is:

1. Vertical Canvas publishes the encoded Portrait feed to a local MediaMTX RTMP path.
2. For Dual LIVE, Vertical Canvas publishes a second encoded Landscape feed to a separate local path.
3. MediaMTX binds its RTMP listener to loopback only and enforces per-feed publisher credentials.
4. The separate TikTok Relay reads the authenticated local feed(s) through FFmpeg/libavformat.
5. The relay performs TikTok-specific packet metadata/SEI transformation and forwards the packets to the remote TikTok ingest endpoint(s).
6. The relay owns the TikTok LIVE session, remote credentials, coordinated stop/failure behavior and cleanup. It reports status over the existing JSON control protocol.

This is a candidate architecture, not yet a working integration. The user-facing OBS plugin must not load TikTok-native GPL-3.0 code; the helper remains a separate process. The process boundary is an engineering design to review, not an automatic legal safe harbor.

## Why direct FFmpeg listening was rejected

A local CLI experiment was run in an isolated Debian Linux environment with FFmpeg 7.1.5 (GPL-enabled build):

- FFmpeg `-listen 1` bound to the explicit host `127.0.0.1`.
- `ss -ltnp` showed the listener on `127.0.0.1:<port>`, not `0.0.0.0:<port>`.
- A second FFmpeg process successfully published a synthetic H.264 video + AAC audio stream.
- The listener demuxed the expected H.264/AAC tracks and dimensions.

But a wrong-path test exposed a security failure. The listener expected `/live/expected-token`; the publisher used `/live/wrong-token`. FFmpeg logged:

```text
Unexpected stream wrong-token, expecting expected-token
```

It nevertheless accepted the connection and demuxed the incoming media. Therefore FFmpeg's listen mode does not enforce the expected stream name as authentication. A random RTMP path is not a sufficient publisher-access control.

**Decision:** do not use FFmpeg's `-listen 1` mode as the product's local RTMP server. A random path/port may reduce accidental collisions but is not authentication.

## Why MediaMTX is the leading candidate

The upstream [MediaMTX](https://github.com/bluenviron/mediamtx) project provides an independently maintained RTMP server and media router. Its inspected root LICENSE at v1.21.4 is MIT. MIT is compatible with the inherited GPL-2.0 plugin, provided its copyright and license notice are preserved.

Version review as of 18 November 2026:

- Current inspected release: [MediaMTX v1.21.4](https://github.com/bluenviron/mediamtx/releases/tag/v1.21.4), published 15 November 2026.
- The release provides Windows x64, Linux amd64/arm64 and macOS builds.
- The official authentication guide documents an internal user database with per-user `publish` and `read` permissions scoped to exact paths.
- For RTMP, MediaMTX accepts username/password query parameters, for example `rtmp://localhost/mystream?user=myuser&pass=mypass`.
- RTMP multitrack OBS authentication had a bug reported in MediaMTX #6007; the underlying [gortmplib PR #93](https://github.com/bluenviron/gortmplib/pull/93) fixing OBS URL parsing was merged on 1 August 2026. The v1.21.4 release includes a later gortmplib version. This reduces a known compatibility concern but does not replace testing our exact OBS output path.

References:

- [MediaMTX authentication documentation](https://mediamtx.org/docs/features/authentication)
- [MediaMTX v1.21.4 release](https://github.com/bluenviron/mediamtx/releases/tag/v1.21.4)
- [MediaMTX license](https://github.com/bluenviron/mediamtx/blob/v1.21.4/LICENSE)
- [OBS multitrack RTMP authentication issue #6007](https://github.com/bluenviron/mediamtx/issues/6007)
- [Fix merged as gortmplib PR #93](https://github.com/bluenviron/gortmplib/pull/93)

## Required secure MediaMTX configuration

The helper must generate a temporary configuration for each LIVE session. Do not ship the upstream default configuration unchanged: it contains permissive `user: any` entries for default publishing/reading and local API/metrics/pprof permissions.

The generated config must:

- Set `authMethod: internal`.
- Replace the entire default `authInternalUsers` list with explicit users. Do not append users while leaving the default anonymous users in place.
- Create unpredictable per-session credentials and a separate path for Portrait and Landscape.
- Grant only the OBS publisher account `publish` permission for its exact feed path.
- Grant only the relay reader account `read` permission for its exact feed path.
- Restrict those accounts to loopback addresses (for example `127.0.0.1/32`).
- Bind RTMP to an explicit loopback address, such as `127.0.0.1:<ephemeral-port>`, never `:1935`/all interfaces.
- Enable RTMP only. Disable RTMPS unless there is a demonstrated need.
- Disable the Control API, metrics, pprof, playback, HLS, WebRTC, SRT and all other unused listeners.
- Use a supported current MediaMTX release and preserve its MIT copyright/license notice in the package.
- Avoid legacy per-path options `readUser`, `readPass`, `publishUser`, `publishPass` and their IP counterparts. Use `authInternalUsers` directly.

The auth guide confirms that RTMP credentials can be supplied in the URL query:

```text
rtmp://127.0.0.1:<port>/<session>/<feed>?user=<publisher>&pass=<publisher-secret>
```

The relay's read URL uses a separate reader credential. Secrets must be generated at runtime, never hard-coded, never written to persistent logs and never reused between LIVE sessions. The local username/password are access-control values for the loopback transport; they are not TikTok credentials.

### Security advisories reviewed

The version floor must account for upstream security fixes:

- MediaMTX v1.21.0 fixes RTMP H.264/HEVC zero-length parameter-set crashes.
- MediaMTX v1.21.1 fixes a critical Control API CSRF issue that could lead to command execution when the API was enabled and exposed to a browser.
- MediaMTX v1.21.2 fixes a playback authorization bypass affecting deprecated legacy per-path credentials when recording/playback was enabled.

The selected v1.21.4 release is newer than those fixes. We still disable unused listeners and APIs, avoid deprecated legacy auth fields, and recheck advisories before each packaged release.

Official advisories:

- [Control API CSRF — patched in v1.21.1](https://github.com/bluenviron/mediamtx/security/advisories/GHSA-w334-5mp5-h897)
- [RTMP H.264/HEVC parser crash — patched in v1.21.0](https://github.com/bluenviron/mediamtx/security/advisories/GHSA-7498-vjgw-9cr2)
- [Legacy playback auth bypass — patched in v1.21.2](https://github.com/bluenviron/mediamtx/security/advisories/GHSA-jh89-m3cv-fmv2)

## Intended media flow

### Single-feed LIVE

1. Relay starts MediaMTX with a temporary config, loopback-only RTMP bind and one exact feed path.
2. Relay generates separate publisher and reader credentials and passes the local publish destination to the OBS plugin through the control channel.
3. OBS publishes the encoded Vertical Canvas Portrait feed to MediaMTX using the publisher credential.
4. Relay reads the authenticated path through FFmpeg/libavformat using the reader credential.
5. Relay transforms the H.264/HEVC packet metadata/SEI as required by the TikTok protocol.
6. Relay forwards the transformed packets to the TikTok ingest endpoint.
7. Stop closes the OBS publisher, relay reader, MediaMTX process and TikTok LIVE session; cleanup must be idempotent.

### Dual LIVE

- Vertical Canvas sends two separate feeds: `portrait` and `landscape`.
- Portrait keeps its existing Vertical Canvas encoder configuration.
- Landscape is intended to reuse the active Main Canvas streaming video/audio encoder objects.
- MediaMTX enforces a distinct path and publisher/reader credential pair for each feed.
- The relay associates both local feeds with the same TikTok LIVE session and maps them to their respective remote ingest endpoints.
- Each feed has an independent packet-transform state. A terminal failure in either feed follows one documented pair-failure policy.

No assumption is made that OBS can publish to the MediaMTX URL format without changes. The exact server/key/query construction and reuse of existing OBS encoder objects must be tested in OBS 33.0.1 on Windows.

## Proof-of-concept gates

Do not integrate this into the user's live OBS profile until all of these pass:

1. Download and verify the official MediaMTX v1.21.4 Windows x64 release and checksum.
2. Generate a minimal temporary config with API/metrics/pprof/playback/HLS/WebRTC/SRT disabled and RTMP bound only to loopback.
3. Start MediaMTX and verify the actual socket is bound only to `127.0.0.1`.
4. Publish with a valid OBS-style RTMP URL and verify the feed is accepted.
5. Publish with a wrong username, wrong password, wrong path and missing credentials; assert that MediaMTX rejects each before the relay receives media packets.
6. Verify exact RTMP URL syntax from the OBS output implementation, including query parameters and multitrack video if applicable.
7. Read the valid path with FFmpeg/libavformat using a separate reader credential.
8. Verify H.264, HEVC and AAC separately, including codec configuration/extradata, packet timestamps and A/V synchronization.
9. Pass the received AVPacket representation into the signer/SEI transformer in the helper and verify SEI placement and output packet validity.
10. Verify two independent authenticated feeds can be read concurrently for Dual LIVE.
11. Test publisher reconnect, incorrect credentials, stream-path collision, early disconnect, helper crash, MediaMTX crash, stop/restart and secret redaction.
12. Repeat on Windows with the exact binaries/dependency hashes intended for packaging.

## What has not been verified

- MediaMTX v1.21.4 has **not** been executed in this environment; the binary download was unavailable from the isolated runtime.
- Publisher auth failure has not yet been observed directly in a running MediaMTX test.
- OBS 33.0.1 Windows publishing to MediaMTX has not been tested.
- FFmpeg C API reading from MediaMTX and preserving H.264/HEVC packet representations has not been implemented.
- No TikTok remote-forwarding, SEI transformation, Dual LIVE or failure/recovery path runs through the relay yet.
- No binary has been integrated into the plugin installer.

The existing `tiktok-relay` target remains a JSON-lines control-protocol stub. This document chooses the next transport candidate; it is not evidence that the relay is ready to use.
