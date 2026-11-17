# TikTok integration — source and license inventory

Status: remediation in progress. This document is a technical provenance inventory, not legal advice and not a statement that the current combined binary is cleared for distribution.

## Repository and revision comparison

The 38 files under `src/tiktok-native/` in the WEST branch were compared using Git blob IDs against [Loukious/obs-multi-rtmp](https://github.com/Loukious/obs-multi-rtmp) at commit `1a57e1ec410695b0d46414787dc8e8a7fb87f72b` (14 November 2026). All 38 WEST files have the same Git blob ID as the corresponding file in that source revision. This is an exact byte-for-byte match, not a similarity estimate.

The Loukious repository root has a GNU GPL version 2 LICENSE and GitHub classifies the repository as GPL-2.0. The Smile-4-Me source repository, [tiktok-live-obs](https://github.com/Smile-4-Me/tiktok-live-obs), declares GPL-3.0-only in its README and root license. Individual files from that source carry GPL-3.0-only SPDX notices. The Loukious fork preserves those per-file notices on the corresponding adapted files.

## Files carrying an explicit GPL-3.0-only notice

The following 22 files in WEST's `src/tiktok-native/` carry an explicit `SPDX-License-Identifier: GPL-3.0-only` notice in the matching Loukious source revision:

- `frame_signing.cpp`
- `frame_signing.hpp`
- `hosted_signing_service.hpp`
- `live_client.cpp`
- `live_client.hpp`
- `localization.hpp`
- `qr_login.hpp`
- `sei_metadata.cpp`
- `sei_metadata.hpp`
- `tiktok_request_signer.cpp`
- `tiktok_request_signer.hpp`
- `tiktok_studio_account.hpp`
- `tiktok_studio_device.cpp`
- `tiktok_studio_device.hpp`
- `tiktok_studio_game_tags.cpp`
- `tiktok_studio_game_tags.hpp`
- `tiktok_studio_qr.cpp`
- `tiktok_studio_qr.hpp`
- `tiktok_studio_session.cpp`
- `tiktok_studio_session.hpp`
- `tiktok_studio_topics.cpp`
- `tiktok_studio_topics.hpp`

These files are not eligible to be relicensed by WEST Gaming as GPL-2.0 merely by editing the root LICENSE or retaining a second license text. Their current notices and source attribution must be preserved until the affected implementation is replaced or a valid separate distribution path is established.

## Remaining files in the native directory

The other 16 files in `src/tiktok-native/` do not carry the explicit GPL-3.0-only SPDX marker in their current source headers:

- `api_error.hpp`
- `domain_routing.cpp`
- `domain_routing.hpp`
- `http_fixture.hpp`
- `http_retry.hpp`
- `live_safety.cpp`
- `live_safety.hpp`
- `live_settings.hpp`
- `localization.cpp`
- `mobile_device.cpp`
- `mobile_device.hpp`
- `mobile_profile.cpp`
- `mobile_profile.hpp`
- `rapidapi_quota.cpp`
- `rapidapi_quota.hpp`
- `request_cancellation.hpp`

Their identical blobs in the Loukious repository indicate that they came from that fork. The initial classification is that they are covered by the source repository's root GPL-2.0 terms, but their individual provenance and any incorporated material must still be checked before the final license audit. Absence of a GPL-3.0-only header is not, by itself, proof that a file is independently authored.

## License facts and conservative interpretation

- Aitum Vertical Canvas's root `LICENSE` is the GNU GPL, version 2. GitHub's repository metadata labels it GPL-2.0, and the Flathub metainfo also states GPL-2.0.
- Nixpkgs currently classifies the packaged Aitum plugin as GPL-2.0-or-later. This conflicts with the source repository's visible license metadata; until a primary, authoritative project declaration resolves the discrepancy, this fork uses the conservative GPL-2.0-only assumption.
- Loukious OBS Multi-RTMP's root `LICENSE` is also the GNU GPL, version 2; GitHub labels that repository GPL-2.0.
- Smile-4-Me TikTok Live OBS declares GPL-3.0-only, and the 22 files listed above carry matching file-level notices in the Loukious fork.

GNU's compatibility FAQ says GPL-2-only and GPL-3-only cannot be combined into one distributed program; GPL-2-or-later code can be combined with GPL-3. See [GNU GPL FAQ — GPLv2 and GPLv3 compatibility](https://www.gnu.org/licenses/gpl-faq.html#v2v3Compatibility).

## Chosen remediation path

The maintainer has chosen not to request a special license exception or permission to relicense third-party code. The planned route is:

1. Keep the Aitum-derived plugin under its inherited GPL-2.0 terms unless primary evidence establishes a later-version grant.
2. Replace the 22 GPL-3.0-only-derived files with new GPL-2-compatible implementations, or remove the affected features if an independent replacement cannot be completed.
3. Preserve behavior through documented requirements, public API/protocol documentation, OBS-level tests and observable integration tests. Do not copy, mechanically translate, rename or superficially refactor the GPL-3.0-only source.
4. Confirm the remaining 16 files and every related file outside `src/tiktok-native/` are correctly attributed and compatible with the project's license.
5. Re-run build, runtime and packaging tests after replacement.
6. Keep the feature PR in draft and do not publish a combined stable binary Release until the source-level audit is complete.

A clean implementation is not established merely because files are rewritten. The final work must avoid copying protected implementation expression from the GPL-3.0-only code. Where the provenance or legal status remains uncertain, obtain qualified software-licensing review before distribution.

## Maintainer verification of runtime behavior

On 17 November 2026, the repository owner tested Landscape, Portrait and Dual LIVE for approximately 30 minutes in OBS 33.0.1 on Windows and reported all three modes working. The separate OBS Multi-RTMP plugin was disabled during this test. This confirms the current WEST implementation does not require that plugin to be enabled in OBS. It does not resolve the source-license compatibility issue and is not a cross-platform compatibility matrix.

## Build-system integration

The current `CMakeLists.txt` creates one plugin MODULE and adds the TikTok-native implementation sources to that same target through `TIKTOK_NATIVE_SOURCES` and `target_sources`. The files with GPL-3.0-only notices are therefore compiled into the same Vertical Canvas module as the Aitum-derived GPL-2.0 code; they are not separate programs or mere aggregation. The license incompatibility is consequently relevant to the current combined plugin.

The source list compiles the native implementation units for domain routing, safety, frame signing, LIVE client, mobile device/profile, quota handling, SEI metadata, request signing, device registration, game tags, QR, session, topics and localization. Several GPL-3.0-only headers are included by those translation units even though headers are not listed as standalone CMake sources.

## Related source outside `src/tiktok-native/`

The rest of the TikTok UI/output integration contains both adapted files from the GPL-2.0 Loukious fork and new WEST/Aitum integration code. Files such as `src/tiktok-game-info.cpp`, `src/tiktok-game-info.h` and `src/tiktok-live-settings.h` are exact matches to the Loukious source tree; several other account/Streamlabs files are modified versions. These are not among the 22 files identified above as carrying a GPL-3.0-only notice, but the final audit must still preserve their Loukious/Aitum provenance and ensure no GPL-3.0-only implementation was copied into them.


## Additional provenance blocker: source without a published license

A separate source path is explicitly referenced by the current device-registration code:

- `src/tiktok-native/tiktok_studio_device.hpp` says the desktop registration envelope is ported from `Loukious/TikTokStreamKeyGenerator`.
- `src/tiktok-native/mobile_device.hpp` says the Android identity generator is a native port of `TiktokStreaming/Libs/mobile_device_gen.py`; the matching file exists in the public `Loukious/TikTokStreamKeyGenerator` repository.
- The current implementation files are `src/tiktok-native/tiktok_studio_device.cpp`, `src/tiktok-native/mobile_device.cpp` and `src/tiktok-native/mobile_device.hpp`.

The GitHub repository API reports no license for [Loukious/TikTokStreamKeyGenerator](https://github.com/Loukious/TikTokStreamKeyGenerator), and its recursive repository tree contains no LICENSE file. Public visibility is not a license grant. The current C++ file-level GPL-3.0-only header cannot grant rights to code ported from a source with no published license.

**Decision:** do not carry those device-registration/identity implementations into a distributable build by changing their headers. Replace them with independent implementations based on documented protocol requirements, or disable the affected device-registration mode until a lawful implementation exists. No request for special permission is planned.

`src/tiktok-native/mobile_profile.cpp/.hpp` also consume the profile/device data model. Their detailed relationship to the no-license Python source remains an audit item; do not treat them as cleared solely because they lack a GPL-3.0-only marker.

## Updated scope

The current confirmed replacement scope therefore includes:
- 22 files carrying explicit GPL-3.0-only notices; and
- the device-registration/identity code above, whose cited source repository has no published license.

The other unmarked files still need a file-level provenance check. The absence of a GPL-3.0-only marker does not settle their status.


## Additional GPL-3.0-derived integration files outside `src/tiktok-native/`

The pull-request patch contains two additional files with explicit source-provenance comments:

- `src/tiktok-live-session.cpp` begins with: `Native OBS packet integration adapted from TikTok Live OBS (GPL-3.0-only).` It is compiled into the Vertical Canvas module by CMake.
- `src/streamlabs-desktop.cpp` states that it is adapted from both `StreamLabsTikTokStreamKeyGenerator` and `Smile-4-Me/tiktok-live-obs/src/streamlabs_desktop.cpp`. The Smile-4-Me source file carries an explicit GPL-3.0-only header. The separate [Loukious/StreamLabsTikTokStreamKeyGenerator](https://github.com/Loukious/StreamLabsTikTokStreamKeyGenerator) repository has a GPL-3.0 LICENSE (GitHub metadata: GPL-3.0).

These files do not have a root-level `SPDX-License-Identifier: GPL-3.0-only` marker in the WEST tree, but their own source comments disclose adaptation from GPL-3.0-only material. They must be included in the clean replacement/review scope. This means the inventory now identifies **at least 24 files** with GPL-3.0-only source provenance: 22 marked files under `src/tiktok-native/`, plus these two integration files.

## No-license device source remains a separate blocker

The device-registration source path described above remains separate from the GPL-3.0-only count. The current desktop and Android identity code cites `Loukious/TikTokStreamKeyGenerator`, whose inspected repository head has no LICENSE file and no GitHub license metadata. A public source repository without a license grant must not be treated as redistributable.
