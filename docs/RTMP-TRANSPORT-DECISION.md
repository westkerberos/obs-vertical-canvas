# RTMP media transport decision — TikTok Relay

Status: FFmpeg RTMP protocol CLI smoke-tested on Linux; C API integration and secure publisher authentication are not implemented or runtime-validated.

## Decision

Use the FFmpeg libraries through their C API **inside the separate WEST TikTok Relay process** for the first media-transport prototype.

The intended Windows build will use the FFmpeg 7.1.1 dependency already built by OBS's `obs-deps` release, not an arbitrary system FFmpeg. We inspected the official `obsproject/obs-deps` Windows recipe at tag `2025-05-23`: it configures FFmpeg with `--enable-gpl`, `--enable-version3`, `--enable-libx264` and `--enable-shared`. This is therefore a GPL-enabled FFmpeg build with version-3 support, **not an LGPL-only build**. It can be evaluated inside the separate GPL-3.0 relay, but only with all included FFmpeg and dependency notices/source obligations handled correctly.

Do not link FFmpeg or GPL-3.0 TikTok runtime code into the Aitum-derived OBS plugin. The Vertical Canvas plugin remains the OBS-facing GPL-2.0 component. The standalone TikTok Relay remains a separate GPL-3.0 program, controlled by a small JSON-lines protocol and receiving media over ordinary RTMP/FLV on loopback. The process boundary is an engineering architecture to be reviewed, not a legal guarantee that both programs are separate works.

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

The FFmpeg dependency recipe actually used by OBS Windows builds is not LGPL-only. The inspected [obsproject/obs-deps Windows recipe at tag 2025-05-23](https://github.com/obsproject/obs-deps/blob/2025-05-23/deps.ffmpeg/99-ffmpeg.ps1) builds FFmpeg 7.1.1 with `--enable-gpl`, `--enable-version3`, `--enable-libx264` and `--enable-shared`. It also enables additional libraries including Opus, Vorbis, VPx, RIST and SRT.

That GPL-enabled, version-3 build is a candidate only inside the **separate GPL-3.0 relay**. It must never be linked into the Aitum-derived GPL-2.0 OBS module. Whether the helper executable and OBS plugin remain separate works when distributed together requires legal review; the process boundary is not a legal safe harbor.

For distribution, we must:

- Record the exact FFmpeg commit, configure flags and OBS dependency archive hash.
- Include the applicable FFmpeg GPL-3.0 license text. The `obs-deps/licenses/FFmpeg` folder we inspected contains `COPYING.GPLv2` and `MAINTAINERS`; do not assume that folder alone satisfies the version-3 license notice requirement.
- Preserve the x264 notice/source and every other third-party license for libraries built into the FFmpeg package.
- Provide the corresponding FFmpeg source, dependency sources, patches and build scripts required by the chosen GPL distribution model.
- Bundle the FFmpeg DLLs needed by the helper and their transitive dependencies. Do not assume a child process can resolve DLLs merely because OBS has loaded them.
- Verify a clean Windows installation contains all runtime DLLs and license notices before publishing an installer or ZIP.

A dedicated minimal LGPL-only FFmpeg build could be investigated later, but it is not what the current OBS dependency package provides.

This is a source-based engineering assessment, not legal advice. The final combined source and binary distribution still needs a qualified licensing review.

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

## Local FFmpeg CLI experiment — 18 November 2026

A smoke experiment was run in an isolated Debian Linux environment using FFmpeg 7.1.5 (GPL-enabled build). This was a protocol experiment only; it was not the OBS Windows build and did not compile or exercise the C++ relay.

### What passed

- FFmpeg `-listen 1` opened an RTMP listener from a URL using the explicit host `127.0.0.1`.
- `ss -ltnp` showed the socket bound to `127.0.0.1:<port>`, not `0.0.0.0:<port>`.
- A second FFmpeg process published a short synthetic stream containing H.264 video and AAC audio to the listener.
- The listener demuxed the stream and reported the expected H.264/AAC streams and dimensions.

This confirms the FFmpeg RTMP protocol can support a local RTMP publisher/receiver experiment. It does **not** confirm the C API integration, packet transformation, remote TikTok forwarding, Windows dependency resolution or Dual LIVE.

### Security result: stream-path mismatch is not authentication

A second test configured the listener with the expected path `/live/expected-token`, then published to `/live/wrong-token` on the same port.

FFmpeg logged:

```text
Unexpected stream wrong-token, expecting expected-token
```

However, the listener still accepted the connection and demuxed the incoming media. The command completed successfully. Therefore the RTMP application/stream-name mismatch warning is **not a strict rejection** and the path token alone cannot be treated as authentication.

Consequences:

- Do not claim that a random RTMP path is an authenticated local credential.
- A random high port and random path can reduce accidental collisions and casual discovery, but they do not provide a strong access-control boundary by themselves.
- Before using FFmpeg's listener mode in the product, either add a transport layer that strictly authenticates the publishing connection before media is passed to the relay, or select a local RTMP server with verified publisher authentication.
- Keep the listener bound to loopback. Do not expose it to the LAN or public interfaces.
- Test the chosen authentication behavior with a deliberately incorrect username/password/path and assert that no media packets reach the relay. A warning in the log is not sufficient.

A MediaMTX-style authenticated local RTMP ingest is a candidate to compare against a custom strict RTMP handshake. It would add another dependency/process, so it should be evaluated only if it provides enforceable publisher authentication and can preserve the media packets needed by the signer.

## Revised next engineering gate

The FFmpeg C API listener must not be integrated into the OBS workflow until the local publisher-authentication issue has an enforceable solution. The next spike should:

1. Compare FFmpeg's direct listener against an authenticated local RTMP server such as MediaMTX.
2. Verify the server binds only to loopback and rejects a publisher with incorrect credentials.
3. Verify a valid OBS-style RTMP publisher yields H.264/HEVC and AAC packets with usable timestamps/extradata.
4. Confirm the packet transformer can process the FFmpeg-demuxed representation without breaking H.264/HEVC configuration, SEI placement or A/V timing.
5. Re-run the same checks on Windows using the exact OBS dependency package and its documented hashes.

## Current implementation status

The existing `tiktok-relay` target is still only a JSON-lines control-protocol stub. It does not link FFmpeg, accept RTMP, sign frames, connect to TikTok or implement Dual LIVE. The Linux CLI experiment above validates only basic RTMP listen/publish behavior and exposes an authentication weakness. It is not evidence that the media path is integrated or ready for use.
