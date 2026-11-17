# WEST TikTok Relay

This directory is a separate-process prototype. It is intentionally a separate CMake project and executable with its own GPL-3.0-only license; it is not linked into the OBS Vertical Canvas module.

## Status

**Protocol stub only.** It currently implements JSON-lines process startup, capability reporting and an explicit `not_implemented` response for LIVE commands. It does not:
- connect to TikTok;
- accept RTMP/FLV media;
- read cookies, API keys or account credentials;
- perform signing, device registration or Dual LIVE;
- launch automatically from OBS;
- get installed or included in the Windows plugin packages.

Do not enter credentials into this prototype. The existing tested TikTok integration remains in the main plugin and has not been replaced.

## Process boundary being prototyped

The target is a standalone executable with no libobs dependency. The intended architecture is:
- JSON control messages over stdin/stdout pipes.
- Standard RTMP/FLV media over loopback once the media listener is implemented.
- No shared OBS C++ types, OBS objects, shared-memory video buffers or linked runtime libraries between the two programs.

This boundary is a technical design candidate, not a legal guarantee. See [the architecture decision](../docs/LICENSING-ARCHITECTURE-OPTIONS.md) and [the implementation plan](../docs/TIKTOK-REIMPLEMENTATION-PLAN.md).

## Build

The parent CMake project includes this independent target. Build it with the normal project configuration; its executable target is `west-tiktok-relay`. The test suite checks the JSON-lines control contract.

## Licensing

This subproject's new code is licensed under GPL-3.0-only. Its license text is in this directory's `LICENSE`. The root OBS Vertical Canvas module retains its inherited Aitum GPL-2.0 terms. The two programs are not yet a finished integrated product, and the combined source/package requires review before distribution.
