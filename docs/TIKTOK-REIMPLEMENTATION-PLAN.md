# TikTok relay implementation plan

Status: the separate executable has a tested JSON-lines control-protocol stub. No RTMP/FLV media path or TikTok runtime refactor has been implemented.

## Objective

Preserve as much of the tested WEST Gaming integration as possible without compiling GPL-3.0-only code into the GPL-2.0 Aitum-derived OBS plugin, and without redistributing code from an unlicensed source repository.

Preferred experiment: a separate **WEST TikTok Relay** executable. The existing OBS plugin remains a separate GPL-2.0 program; the helper is a separate GPL-3.0 program. They communicate through RTMP/FLV over localhost for media and a small documented JSON control protocol over pipes for lifecycle/status. See [LICENSING-ARCHITECTURE-OPTIONS.md](LICENSING-ARCHITECTURE-OPTIONS.md) and the [RTMP transport decision](RTMP-TRANSPORT-DECISION.md).

This is only an engineering candidate. Whether the actual programs remain separate works depends on their implementation and communication semantics; no legal clearance is asserted.

## Program responsibilities

### OBS / Vertical Canvas plugin — inherited GPL-2.0

- Own OBS UI, scenes, canvases, preview, encoder selection and OBS lifecycle.
- Keep existing Portrait and Landscape encoder ownership. Portrait uses the Vertical Canvas TikTok destination encoder. Landscape reuses the active Main Canvas streaming video/audio encoder objects.
- Launch and supervise a separate relay executable.
- Send account/provider settings to the relay through the documented control channel; never place secrets in command-line arguments.
- Create OBS output(s) whose server/key point only to the relay's local RTMP listener.
- Treat the relay status as the authority for remote LIVE session state, endpoint readiness and shutdown result.
- Never link to, load, or share C++ classes/data structures with the relay's TikTok runtime.

### TikTok Relay — separate GPL-3.0 executable

- Own TikTok account/session/network operations, signing and remote ingest connections.
- Receive local RTMP/FLV media from OBS.
- For normal LIVE, accept one feed. For Dual LIVE, pair the local `portrait` and `landscape` feeds with two remote ingest targets belonging to the same TikTok LIVE session.
- Own the remote TikTok room lifecycle, stream metadata/SEI operations, reconnect/error reporting and cleanup.
- Avoid OBS headers, libobs links, Qt plugin APIs and Vertical Canvas internal types wherever possible. Its dependency graph must be independently buildable and auditable.

## Proposed process protocol

Use a deliberately small contract:

- Control channel: JSON request/response messages over stdin/stdout pipes (or an authenticated loopback control socket if pipe supervision proves unreliable).
- Candidate commands: `hello`, `capabilities`, `account_status`, `dual_live_eligibility`, `get_topics`, `get_games`, `start_live`, `stop_live`, `get_status`, `get_safety_status`.
- A successful `start_live` returns only local RTMP feed URLs/keys, a session correlation ID, and the feed roles required by OBS. Do not return remote TikTok ingest URLs/keys to the plugin.
- Media: ordinary RTMP/FLV over loopback. The selected proof-of-concept is FFmpeg libavformat RTMP listen/input plus RTMP output inside the relay process; see [RTMP-TRANSPORT-DECISION.md](RTMP-TRANSPORT-DECISION.md). One listener/feed for Portrait; a second listener/feed for Landscape during Dual LIVE.
- Bind listeners to 127.0.0.1 only. Generate unpredictable local stream keys. Never expose the listener to the LAN.
- Do not send OBS scene objects, encoder objects, raw OBS packets as private structs, callback pointers, shared memory or custom in-process ABI across the boundary.
- Pass cookies, RapidAPI keys, Streamlabs tokens and other credentials only through the local control channel. Never place them in command-line arguments, status text, persistent helper logs or crash reports.
- Helper shutdown and reconnect operations must be idempotent. Stopping one Dual LIVE feed must follow an explicitly documented pair-failure policy.
- The process boundary and aggregate installer require a software-license review before public binary publication.

This contract is intentionally limited. If the helper needs private OBS/Vertical Canvas structures or a direct shared library to work, stop and reassess the boundary.

## Source partition

See [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md) for exact file paths and upstream revisions. The source audit currently identifies at least 24 GPL-3.0-derived integration files: 22 under `src/tiktok-native/` with explicit GPL-3.0-only headers, plus `src/tiktok-live-session.cpp` and `src/streamlabs-desktop.cpp` with explicit source-provenance comments.

The helper must not blindly import the 16 unmarked files from the Loukious fork. Their root repository is GPL-2.0, their individual provenance has not been conclusively classified, and several are dependencies of the marked GPL-3.0 modules. Until they are audited, assume they cannot be moved into a GPL-3.0 helper. Reimplement only the required utility behavior independently inside the helper, or keep an independently verified compatible implementation on the GPL-2.0 side.

The device-registration code is a separate blocker:
- `tiktok_studio_device.hpp/.cpp` identifies the desktop registration envelope as ported from `Loukious/TikTokStreamKeyGenerator`.
- `mobile_device.hpp/.cpp` identifies the Android identity/registration path as a port from that repository's `Libs/mobile_device_gen.py` and TTEncrypt codec.
- The cited public repository has no LICENSE file or GitHub license metadata.
- The G3 header on one WEST file does not grant rights to the unlicensed source from which that implementation was ported.

The desktop/mobile identity and registration algorithms therefore need independently authored replacements or must be disabled. `mobile_profile.cpp/.hpp` must be audited with them.

Other files that have explicit G3 provenance, including the OBS packet/session adapter and Streamlabs token discovery, may be moved to a G3 helper only if their full dependency graphs are clean. The current OBS packet adapter itself must be replaced by the local RTMP/FLV process boundary; it must not keep shared OBS state inside the relay.

## Compatible alternatives examined

- [TikTok LIVE Native](COMPATIBLE-TIKTOK-ALTERNATIVES.md): MIT, browser-cookie web session, room creation and one RTMP push URL. Linux-only and no Dual LIVE parity.
- [iqbalmh18/tiktok-signer](COMPATIBLE-TIKTOK-ALTERNATIVES.md): MIT, but documents version 37.0.4 and a different TTEncrypt variant. Not a proven replacement for our 47.x Android path.
- [9xz/reversetok](COMPATIBLE-TIKTOK-ALTERNATIVES.md): documents newer 47.7.2 behavior, but publishes no license. Do not copy or package its code, fixtures or tests.

No inspected permissively licensed implementation is a proven drop-in replacement for the current LIVE Studio/Android Dual LIVE runtime.

## Verification plan

1. Build the relay as an independent process with no OBS/Vertical Canvas link dependencies.
2. Add contract tests for each control message, credential redaction, invalid/expired cookie handling, start/stop idempotency and helper crash/restart.
3. Add local media tests for one RTMP feed, two simultaneous feeds, backpressure, reconnect, audio/video timestamp handling and orderly shutdown.
4. Add protocol fixtures with secrets removed. Do not use real cookies, account IDs, signing keys or stream keys in the repository.
5. Build the OBS plugin without any GPL-3.0-only source or helper library linked into its module.
6. Run the existing maintainer smoke test again: OBS 33.0.1 on Windows, Landscape, Portrait and Dual LIVE, with the separate OBS Multi-RTMP plugin disabled.
7. Test both feed directions, Main Canvas encoder reuse, one-feed failure, relay crash, stop/restart, QR login and all supported connection modes.
8. Inspect the final Windows package and its separate license manifests/notices. Add cross-platform builds only after the process and media paths are implemented/tested there.

## Release gate

The project remains not cleared for public binary distribution until:
- the OBS plugin and relay are separate executables with no shared internal code/ABI;
- only source with compatible, documented terms is included in each program;
- the unlicensed device-registration implementation has been removed/replaced or the affected mode has been disabled;
- no GPL-3.0-only code is compiled or linked into the GPL-2.0 OBS plugin;
- dependency graphs, source notices and package manifests are audited;
- clean builds and actual LIVE tests pass;
- the process boundary and bundled distribution have received appropriate legal review.

This plan does not provide legal advice. Preserve the currently tested feature branch unchanged until the relay prototype and tests are ready.
