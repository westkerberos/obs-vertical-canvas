# Licensing review and remediation plan

This document records the repository's current source-based licensing findings and the maintainer's chosen remediation path. It is not legal advice. See [the file-level source inventory](LICENSING-INVENTORY.md) for the auditable lists and source revision IDs.

## Current license facts

- The project is a derivative fork of [Aitum/obs-vertical-canvas](https://github.com/Aitum/obs-vertical-canvas), whose root LICENSE is the GNU General Public License, Version 2.
- The WEST TikTok-native files include source notices identifying components as `GPL-3.0-only`, including `src/tiktok-native/tiktok_request_signer.cpp` and `src/tiktok-native/live_client.cpp`.
- The direct source attribution and matching GPL-3.0 license text are recorded in `data/licenses/tiktok-live-obs.txt` and `data/licenses/tiktok-live-obs-GPL-3.0.txt`.
- The QR-code generator dependency has its own attribution in `data/licenses/qrcodegen.txt`.

The GNU project's compatibility guidance says GPL-2.0-only and GPL-3.0-only code cannot be combined into one distributed program. A GPL-2.0-or-later grant can be compatible with GPL-3.0, but this repository must not assume an `or-later` grant where the inherited project license does not provide one. See the [GNU GPL FAQ](https://www.gnu.org/licenses/gpl-faq.html#v2v3Compatibility) and [GNU license compatibility guidance](https://www.gnu.org/licenses/license-compatibility.html).

## Why a new root LICENSE does not fix this

WEST Gaming cannot change the license of code it did not author or receive relicensing rights for. Replacing the root `LICENSE` with GPL-3.0, MIT, Apache-2.0, or a custom WEST license would not remove the inherited Aitum GPL-2.0 terms. Keeping two license files in the package also does not make the two copyleft licenses compatible.

The existing attribution and license files must stay in place while the source is being audited. They document provenance; they do not grant additional permissions.

## Maintainer decision — 18 November 2026

The maintainer does not want to request special license exceptions or third-party relicensing permissions. The preferred technical experiment is therefore a **separate TikTok relay process**, while preserving the tested OBS plugin branch.

1. Keep the Aitum-derived Vertical Canvas plugin under its inherited GPL-2.0 terms.
2. Move only code that is verified to be GPL-3.0-compatible into a separate GPL-3.0 executable, not into the OBS plugin module.
3. Communicate through standard RTMP/FLV over loopback for media and a small documented start/stop/status protocol for control. No shared DLL, C++ ABI, OBS objects or shared-memory frames.
4. Do not blindly move unmarked helper files into the GPL-3.0 relay. Their provenance is unresolved and some may be GPL-2.0 code from the Loukious fork; independently replace them where the relay needs equivalent behavior.
5. Independently replace or disable device-registration/identity code whose source repository publishes no license grant.
6. Keep all third-party notices and use separate license files, build targets and package manifests for the two programs.
7. Review whether the actual process and distribution boundary is sufficient before bundling a public installer. Process separation is a stronger technical boundary, not a legal guarantee.

If the relay cannot be separated without intimate internal coupling, the fallback is to independently reimplement the affected parts under GPL-2-compatible terms or offer a reduced MIT browser-session provider. Do not change the root LICENSE to disguise the current source mix.

A superficial rewrite, renaming files, changing comments, or copying the same implementation through another fork does not make an independent implementation.

## Packaging and release status

A previous Windows x64 package was inspected and contained the license texts:

- `GPL-2.0.txt`
- `qrcodegen.txt`
- `tiktok-live-obs.txt`
- `tiktok-live-obs-GPL-3.0.txt`

This verified that notices were packaged. It did **not** resolve the license compatibility issue.

Until the source remediation and full audit are complete:

- Do not publish a stable combined binary Release.
- Do not describe the current combined integration as license-cleared.
- Keep PR #1 in draft and keep the licensing blocker visible.
- Recheck the Multistream bridge and any combined installer after the Vertical Canvas license remediation.
- Do not remove or rewrite third-party attribution notices simply to make the repository appear to have one license.

## Meaning of “free”

GPL software is free in the sense of user freedoms, not necessarily zero price. GPL permits redistribution and commercial sale, subject to its conditions. WEST Gaming can choose to provide its own official downloads at no charge while preserving the rights users receive under the GPL. See the [GNU explanation of free software and GPL permissions](https://www.gnu.org/licenses/gpl-faq.html#GPLCommercially).

This is a source-based compatibility assessment, not legal advice. A qualified software-licensing lawyer should review the final implementation and distribution model before the first public Release.


## Second source blocker: code from a repository with no published license

The source audit also found a distinct provenance issue, separate from GPL-3.0-only:

- `src/tiktok-native/tiktok_studio_device.hpp` identifies the desktop registration-envelope implementation as ported from [Loukious/TikTokStreamKeyGenerator](https://github.com/Loukious/TikTokStreamKeyGenerator).
- `src/tiktok-native/mobile_device.hpp` identifies the Android identity generator as a native port of `Libs/mobile_device_gen.py` from that repository.
- The inspected repository head is `ba5041bf2e02c3802b0b457fcad03803c8dc29c2` (9 November 2026).
- The public repository tree has no `LICENSE` file and the GitHub repository metadata reports `license: null`.

Public access to source code does not grant redistribution or derivative-work rights. The files `tiktok_studio_device.cpp`, `mobile_device.cpp/.hpp` and dependent profile logic cannot be treated as clear merely by changing their SPDX headers. Replace these implementations independently or disable the affected registration mode until a permitted implementation exists. The provenance of `mobile_profile.cpp/.hpp` must be audited together with the device-registration path.

## Current release blockers

1. **GPL compatibility:** at least 24 integration files are identified as derived from GPL-3.0-only projects: 22 native files with explicit SPDX notices, plus the OBS packet/session adapter and Streamlabs token-discovery file. They are compiled into the same Vertical Canvas module as Aitum-derived GPL-2.0 code.
2. **No published license:** desktop/mobile device identity code cites a public repository with no LICENSE file or GitHub license metadata.

A potentially useful MIT-licensed web-session alternative has also been documented in [compatible TikTok alternatives](COMPATIBLE-TIKTOK-ALTERNATIVES.md). It is not a drop-in replacement: the inspected version is Linux-only and does not implement the current Dual LIVE or LIVE Studio flows.

All findings and source revision IDs are recorded in the [detailed source inventory](LICENSING-INVENTORY.md) and [independent replacement plan](TIKTOK-REIMPLEMENTATION-PLAN.md). Neither blocker is fixed by adding more license text to the ZIP.
