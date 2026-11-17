# TikTok implementation replacement plan

Status: planning baseline completed; replacement code not started.

## Objective

Preserve the current WEST Gaming user experience while removing source files explicitly marked GPL-3.0-only from the single GPL-2.0 Aitum-derived Vertical Canvas module. The intended output remains a GPL-2.0-compatible project. This is a source replacement, not a relabeling exercise.

The current implementation must remain untouched on `feature/tiktok-native-output` while replacements are developed on `licensing-remediation`. Do not merge, release or claim license clearance during this work.

## Source boundary

See [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md) for the exact 22-file list, the 38-file blob comparison and the current source-license findings.

The current build links the native sources directly into the Vertical Canvas module. The GPL-3.0-only files therefore have to be replaced before a combined plugin binary is distributed under GPL-2.0.

Replacement code must be independently authored. Do not copy blocks, preserve the original file structure mechanically, translate the original line by line, or merely rename/refactor source files. Use observable product requirements, public API/protocol documentation, standards, and independent tests as the implementation basis. If a particular function cannot be implemented without copying protected source expression or relying on an undocumented mechanism that cannot be independently specified, remove that feature from the GPL-2.0 build until there is a compliant alternative.

## Functional requirements to preserve

### Connections and account setup

- Custom RTMP with user-provided server URL and stream key.
- TikTok LIVE Studio Desktop, using the documented user-facing connection flow, cookie-backed account session and hosted signing service credentials.
- TikTok LIVE Studio Mobile Camera LIVE, including the saved device/install profile.
- Streamlabs Desktop token flow.
- QR login and local cookie persistence for the supported native account modes.
- Refresh account status, username and Dual LIVE eligibility.
- Protect cookies, tokens, signing keys and device/profile identifiers; never log or commit secrets.

### LIVE session and metadata

- Create or resume a TikTok LIVE room.
- Obtain the appropriate ingest endpoint and stream key for the selected mode.
- Select a topic and game using TikTok-returned identifiers.
- Apply title, gifts/comments, content disclosure and mature-content options where the selected provider supports them.
- Show hosted signing service quota/errors without exposing credentials.

### Video/audio and Dual LIVE

- Support single-feed Portrait and Landscape destinations.
- Support the current automatic Dual LIVE behavior:
  - Portrait uses the existing Vertical Canvas TikTok output video/audio encoder objects.
  - Landscape reuses the active OBS Main Canvas streaming output video/audio encoder objects.
  - Main Canvas must be streaming before automatic Dual LIVE starts.
  - If one feed fails terminally, stop and clean up the pair.
- Keep the manual layout/encoder override if it can be maintained without the incompatible source implementation.
- Preserve the requirement that TikTok itself reports the account eligible for Dual LIVE.

### Safety and shutdown

- Cleanly stop the LIVE session and both feeds.
- Do not claim that appeals or safety operations succeeded unless TikTok confirms the result.
- Preserve manual violation/appeal lookup and monitoring only where the implementation and service endpoints can be independently verified.
- Cancel pending requests and release cookies/session state when an output stops or OBS unloads.

## Replacement work packages

| Package | Explicitly GPL-3.0-only files | New implementation responsibility |
| --- | --- | --- |
| A. Request signing and hosted signer | `hosted_signing_service.hpp`, `tiktok_request_signer.cpp/.hpp` | Define a small typed client for the hosted signing service; validate HTTP status, response shape, quota headers, cancellation and redaction. |
| B. LIVE session client | `live_client.cpp/.hpp`, `tiktok_studio_session.cpp/.hpp`, `tiktok_studio_account.hpp` | Model account/session state and implement the documented request/response contract without copying the old implementation. |
| C. Device and login lifecycle | `tiktok_studio_device.cpp/.hpp`, `qr_login.hpp`, `tiktok_studio_qr.cpp/.hpp` | Implement desktop/mobile profile setup, QR polling, cookie export and persistence under a new data model. |
| D. Signed video metadata | `frame_signing.cpp/.hpp`, `sei_metadata.cpp/.hpp` | Implement packet signing integration and standards-based SEI/NAL insertion with isolated unit tests. |
| E. Topic and game metadata | `tiktok_studio_game_tags.cpp/.hpp`, `tiktok_studio_topics.cpp/.hpp` | Query, parse and validate provider-returned topic/game identifiers. |
| F. Localization adapter | `localization.hpp` | Replace OBS-coupled native localization helper with a new adapter to WEST's supported translation interface. |

The 16 unmarked files are not automatically accepted as clear. Their origin and dependencies must be checked against the source inventory before the final audit. If any are found to contain GPL-3.0-only-derived implementation, add them to the replacement scope.

## Engineering safeguards

1. Put replacement modules in a new namespace/directory so old source cannot be mixed in accidentally during development.
2. Add explicit unit tests for parsing, retries, cancellation, quotas, signing-service responses, cookie validation and SEI generation.
3. Use fixture data with all secrets removed. Do not use real cookies, API keys or account IDs in the repository.
4. Make the build use only the replacement modules; fail CI if any old GPL-3.0-only file is included in the target.
5. Run code formatting, static checks and a full Windows x64 build.
6. Repeat the maintainer's 17 November 2026 runtime smoke test: Landscape, Portrait and Dual LIVE in OBS 33.0.1 with the separate OBS Multi-RTMP plugin disabled.
7. Re-test QR login, account refresh, reconnects, stop/cleanup, failure paths and the Advanced Dual LIVE override before a general Release.
8. Audit every dependency and license notice in the final package. Keep the root Aitum-derived license unchanged unless primary evidence clearly establishes a different grant.

## Release gate

The current feature is not license-cleared. The work is complete only when:
- no GPL-3.0-only-derived source remains inside the GPL-2.0 Vertical Canvas module;
- no unresolved GPL-3.0-only dependency is linked into the same module;
- all remaining source provenance and copyright notices are documented;
- clean Windows builds and runtime tests pass;
- the documentation, PR description and installer prompts match the final license reality.

This plan does not provide legal advice. A qualified software-licensing review is still appropriate before the first public binary Release.
