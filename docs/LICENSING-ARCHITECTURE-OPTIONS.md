# Licensing and architecture options

This is an engineering decision record, not legal advice. Its purpose is to avoid changing license labels while preserving as much tested functionality as the source rights allow.

## Current facts

The current WEST Vertical Canvas feature branch compiles TikTok-specific code into the same OBS plugin module as the Aitum-derived GPL-2.0 code. The source inventory identifies at least 24 files derived from GPL-3.0-only projects, plus device-registration code that cites a source repository with no published license. See [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md).

Replacing the root LICENSE does not solve this. The tested single-module integration is not cleared for distribution.

## Option A — Independently reimplement inside Vertical Canvas

Keep one OBS plugin under the inherited Aitum GPL-2.0 terms. Replace every GPL-3.0-only-derived and unlicensed component with independently authored, GPL-2-compatible implementations.

Advantages:
- One plugin and the cleanest installation story.
- No helper process or additional service management.

Costs and risks:
- Large scope: signer, LIVE session, device registration, QR, frame signing, SEI metadata, OBS packet adapter, Streamlabs token discovery and account/topic/safety features.
- TikTok's private protocols change and are not guaranteed stable.
- There is no guarantee that every existing capability, especially Dual LIVE, can be preserved.

## Option B — Separate TikTok relay process (preferred full-feature experiment)

Split the product into two genuinely separate programs:

1. **WEST Vertical Canvas plugin — inherited GPL-2.0 terms.** Owns OBS UI, canvas/scene configuration and encoder ownership. It does not link to TikTok signing/session code. It publishes ordinary RTMP/FLV to a loopback endpoint exposed by the relay.
2. **WEST TikTok Relay — GPL-3.0.** A separate executable/process that owns TikTok account/session state, request signing, stream metadata/SEI signing, remote TikTok ingest connections, LIVE room lifecycle and cleanup. It accepts the standard RTMP/FLV media streams from the local OBS plugin and forwards them to TikTok.

For Dual LIVE, OBS would send two ordinary local RTMP feeds, one identified as `portrait` and one as `landscape`. The relay would associate both feeds with the same TikTok LIVE session and send each to its own remote ingest endpoint. Portrait and Landscape still use the user's existing OBS/Vertical Canvas encoder settings.

### Proposed process contract

The boundary must be more than two DLLs. The programs must have separate process address spaces and a protocol contract:

- Launch the relay as a separate executable. Do not dynamically load its code into OBS.
- Use a minimal control channel for `start`, `stop`, `status`, `capabilities` and error events. Use documented JSON messages over stdin/stdout pipes or a local authenticated socket. Do not exchange OBS objects, internal C++ classes, callbacks, or private shared structures.
- Use standard RTMP/FLV over loopback for media, not custom packet structs or shared-memory frame buffers.
- Bind media listeners to loopback only. Generate unpredictable per-session local stream keys; never expose these local credentials to the LAN.
- Keep cookies, RapidAPI keys, Streamlabs tokens and local stream keys out of command-line arguments, logs and crash reports. Send secrets only through the local control channel and store them in the user's protected OBS/plugin configuration or the helper's protected store.
- The relay owns remote TikTok URLs/keys and session state. The OBS plugin only knows its temporary local RTMP destination.
- Stop and cleanup must be idempotent. Stopping the OBS output closes the matching relay input and remote feed; stopping either Dual LIVE feed must follow an explicitly tested pair-failure policy.
- Give each program a separate build target, source boundary, license notice and package manifest. The installer can distribute both as an aggregate but must not fuse the programs into one executable or shared module.
- The helper must not require the OBS process to expose internal objects or link against the Vertical Canvas DLL.

### Source partition still required

A process split does not mean we can blindly move the current source directory into a GPL-3.0 executable.

- The 22 files with explicit GPL-3.0-only notices, plus `src/tiktok-live-session.cpp` and `src/streamlabs-desktop.cpp`, are candidates for the GPL-3.0 relay only after their imports/dependencies have been checked.
- The 16 other files in `src/tiktok-native/` have no explicit GPL-3.0-only marker and exactly match the Loukious GPL-2.0 fork. Their provenance must be resolved; do not place them in a GPL-3.0 helper on assumption.
- The GPL-3.0-labelled desktop registration module also identifies a source with no published license. The Android device modules cite the same unlicensed source family. Those registration/identity algorithms must be independently replaced or disabled; a GPL-3.0 file header does not cure an upstream source with no license grant.
- Generic helpers such as retry, cancellation, quota, HTTP fixtures, routing and profile data must be independently reimplemented inside the relay if they cannot be shown to be GPL-3-compatible. Their current unmarked files should not be copied into the relay until this audit is complete.

### First media-transport implementation candidate: FFmpeg

For the first prototype, use FFmpeg's C API from inside the separate relay process rather than add a new RTMP server or a second runtime:

- FFmpeg's RTMP protocol documents a `listen` mode for incoming streams and exposes the `rtmp_listen` AVOption.
- The relay can accept one ordinary local RTMP/FLV feed per layout, demux encoded H.264/HEVC/AAC packets, apply the existing GPL-3.0 signing/SEI logic within the relay, then publish to the TikTok RTMP ingest endpoint.
- Dual LIVE uses two local listeners and two remote outputs, both bound to one TikTok LIVE session.
- The Windows OBS dependency package we inspected actually builds FFmpeg 7.1.1 with `--enable-gpl`, `--enable-version3`, `--enable-libx264` and shared libraries. That is not an LGPL-only build. It is a possible dependency for the separate GPL-3.0 relay, subject to bundling the complete source, applicable GPL-3.0 text, x264 and other third-party notices, configure flags, transitive DLLs and build scripts. It must never be linked into the Aitum-derived GPL-2.0 OBS plugin.
- The detailed finding and outstanding package checks are recorded in [RTMP-TRANSPORT-DECISION.md](RTMP-TRANSPORT-DECISION.md).

Official references and the proof-of-concept gates are recorded in [RTMP-TRANSPORT-DECISION.md](RTMP-TRANSPORT-DECISION.md). This is a selected candidate, not a completed integration. Windows loopback binding, packet representation, timestamp preservation, H.264/HEVC signing and the two-feed failure policy still need working tests.

### Why this boundary may help

The GNU GPL FAQ says the distinction between separate programs and one combined work depends on both the communication mechanism and the semantics. It identifies pipes, sockets and command-line arguments as mechanisms normally used between separate programs, while warning that exchanging complex internal data structures may still indicate one larger work. See [GNU GPL FAQ — aggregate vs combined work](https://www.gnu.org/licenses/gpl-faq.html#MereAggregation) and [GNU GPL FAQ — plugins](https://www.gnu.org/licenses/gpl-faq.html#GPLPlugins).

Standard RTMP/FLV media over loopback plus a small control protocol gives a clearer boundary than a GPL-3 OBS module that shares process memory and OBS C++ structures with the GPL-2 plugin. It is still not a legal guarantee; the exact architecture and combined installer should be reviewed by a qualified software-licensing professional before public binary distribution.

## Option C — MIT browser-session provider

[aemon1977/tiktok-live-native](https://github.com/aemon1977/tiktok-live-native) is MIT-licensed. It demonstrates browser-cookie reuse, web LIVE room creation, RTMP push URL retrieval and room shutdown. It is Linux-only in the inspected revision and supports a single feed; it does not replace LIVE Studio Desktop/Mobile, Dual LIVE, Streamlabs or the current safety/account settings. See [COMPATIBLE-TIKTOK-ALTERNATIVES.md](COMPATIBLE-TIKTOK-ALTERNATIVES.md).

This is a lawful-looking route for a reduced single-feed provider, not a route to preserve all current features.

## Option D — MIT Android signer candidates

The audited [iqbalmh18/tiktok-signer](https://github.com/iqbalmh18/tiktok-signer) is MIT but documents TikTok 37.0.4 and a different TTEncrypt implementation. It is not a proven replacement for the current Android 47.1.4 registration path.

[9xz/reversetok](https://github.com/9xz/reversetok) documents TikTok 47.7.2 signing and the newer TTEncrypt envelope, but publishes no LICENSE file and has no GitHub license metadata. Do not copy or package its source code or tests. A new implementation would need to be authored independently from a separately written protocol specification and validated against TikTok.

## Option E — Change the whole Vertical Canvas fork to GPL-3.0

Not available based on the rights currently established. WEST Gaming cannot change Aitum's inherited GPL-2.0 code to GPL-3.0 without a valid later-version grant or the relevant copyright holders' permission. Do not pursue this by replacing the root LICENSE.

## Current recommendation and status

Preserve the currently tested integration untouched. Prototype **Option B** at the architecture level first, because it offers the best chance of retaining the GPL-3.0 TikTok session/signing functionality without compiling it into the GPL-2.0 OBS plugin. In parallel, isolate and independently replace the no-license device-registration code and audit the unmarked helper modules.

No helper executable, RTMP boundary or runtime refactor has been implemented yet. This document is a design proposal, not a completed technical solution. Do not merge or publish a combined binary Release until the separate-process architecture, source provenance and licenses have been reviewed and tested.

If Option B cannot be implemented without a legally questionable dependency between the two programs, fall back to Option A or the reduced Option C provider.
