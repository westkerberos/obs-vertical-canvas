# RTMP media transport decision — TikTok Relay

Status: selected for a technical proof of concept; not implemented or runtime-validated yet.

## Decision

Use the FFmpeg libraries through their C API **inside the separate WEST TikTok Relay process** for the first media-transport prototype.

Do not link FFmpeg or GPL-3.0 TikTok runtime code into the Aitum-derived OBS plugin. The Vertical Canvas plugin remains the OBS-facing GPL-2.0 component. The standalone TikTok Relay remains a separate GPL-3.0 program, controlled by a small JSON-lines protocol and receiving media over ordinary RTMP/FLV on loopback.

## Why FFmpeg is the first candidate

The official FFmpeg RTMP protocol documentation exposes a `listen` mode for incoming RTMP connections. The RTMP protocol implementation also exposes the `rtmp_listen` AVOption. FFmpeg can therefore serve as the RTMP input side while the same libavformat API writes the processed packets to a remote RTMP destination.

Official references:

- [FFmpeg RTMP protocol options](https://ffmpeg.org/ffmpeg-protocols.html)
- [FFmpeg RTMP protocol implementation and `rtmp_listen` option](https://ffmpeg.org/doxygen/trunk/rtmpproto_8c.html)
- [FFmpeg legal and licensing considerations](https://www.ffmpeg.org/legal.html)
- [GNU license compatibility FAQ](https://www.gnu.org/licenses/gpl-faq.html)

This avoids introducing a second RTMP implementation or a new Go runtime/toolchain into the initial C++ relay prototype.

## Intended data flow

### Single-feed LIVE

1. The OBS plugin creates an ordinary OBS RTMP output whose destination points to a relay-owned loopback listener, for example `rtmp://127.0.0.1:<port>/live/<random-path-token>`.
2. The relay accepts the incoming RTMP/FLV connection through FFmpeg's RTMP listen mode.
3. The relay demuxes the encoded H.264/HEVC video and AAC audio packets.
4. The relay applies TikTok's existing request/frame-signing and SEI transformation in the GPL-3.0 relay process only.
5. The relay writes the transformed packets to TikTok's remote RTMP ingest URL/key through libavformat.
6. Stop and error events are returned to the OBS plugin through the JSON control channel.

### Dual LIVE

- Vertical Canvas sends two separate local RTMP feeds to the relay: `portrait` and `landscape`.
- Both feeds belong to one TikTok LIVE session created by the relay.
- The relay maps each feed to its corresponding remote TikTok ingest endpoint.
- Portrait continues using the Vertical Canvas output's existing encoder settings.
- Landscape continues reusing the active OBS Main Canvas encoder settings.
- Each feed has its own packet-transform state; the remote session lifecycle and coordinated stop/failure policy are shared.

## Licensing and build constraints

FFmpeg's default license is LGPL 2.1-or-later. Optional GPL components are enabled only when explicitly selected at build time. The GNU compatibility table identifies LGPL 2.1-or-later as compatible with GPL 3, so an independently distributed GPL-3.0 relay can use an appropriately licensed FFmpeg build while satisfying the FFmpeg license obligations.

For the first distributable build, prefer a dedicated FFmpeg build with:

- `--disable-gpl`
- `--disable-version3`
- `--disable-nonfree`

Only enable the minimum components needed for RTMP/FLV demuxing/muxing, H.264/HEVC/AAC packet handling and network I/O. Do not assume that an arbitrary prebuilt FFmpeg binary has these options; record its exact configure flags and license manifest.

The relay must preserve FFmpeg copyright/license notices and comply with LGPL redistribution requirements for the exact dynamic/static linking model selected. The corresponding FFmpeg source and build configuration must be made available with the release.

This is a source-based engineering decision, not a legal opinion. The final binary/package still needs a licensing review.

## Required proof-of-concept checks

1. Confirm on Windows that FFmpeg RTMP listen mode can bind specifically to `127.0.0.1` (not all network interfaces) and accept an OBS RTMP publisher.
2. Confirm that the accepted stream path/application can be checked against a per-session random token before accepting media.
3. Confirm the input packets contain the expected codec IDs, time base, dimensions and configuration/extradata.
4. Confirm that the current `SignedSeiSession` transformer can handle the exact AVPacket representation from FFmpeg's FLV demuxer. Do not assume AVPacket's H.264/HEVC representation is byte-identical to OBS's current `encoder_packet`.
5. Verify unchanged DTS/PTS and audio/video synchronization after the packet transformation.
6. Publish a test feed to a controlled RTMP endpoint and verify H.264, HEVC and AAC behavior separately.
7. Verify two independent input listeners and two remote outputs operate concurrently for Dual LIVE.
8. Test early disconnect, reconnect, remote failure, stop, helper crash and cleanup. Stopping either Dual LIVE feed must follow one explicit and tested pair-failure policy.
9. Verify credentials, RapidAPI keys, cookies and remote stream keys never appear in process arguments, logs or crash reports.

## Security requirements

- Bind both listeners to loopback only.
- Use unpredictable ports and per-session stream-path tokens; do not use a fixed local key.
- Validate the incoming stream role and session token before forwarding packets.
- Never expose the remote TikTok ingest URL/key to OBS configuration or the LAN.
- Never put account cookies, RapidAPI credentials or Streamlabs tokens in command-line arguments.
- Treat all RTMP listener input as untrusted until its feed token and session state are validated.

## Alternatives not selected for this first prototype

- **ZLMediaKit:** its C API provides RTMP/media track facilities, but its repository includes an additional brand-information condition and warns that some scattered third-party code may need replacement/removal. That requires a broader source/dependency review than the current prototype.
- **gomedia:** its Go implementation provides RTMP client/server support under MIT, but integrating it with the existing C++ TikTok runtime would require either a language/runtime bridge or a substantial C++ port. Keep it as a fallback if the FFmpeg listener experiment fails.
- **MediaMTX:** it is a useful cross-platform media router, but the current documented routing/hook model does not establish the frame-mutation callback needed for TikTok's packet signing and SEI transformation.

## Current implementation status

The existing `tiktok-relay` target is still only a JSON-lines control-protocol stub. It does not link FFmpeg, accept RTMP, sign frames, connect to TikTok or implement Dual LIVE. This document selects a candidate for the next engineering spike; it is not evidence that the media path works.
