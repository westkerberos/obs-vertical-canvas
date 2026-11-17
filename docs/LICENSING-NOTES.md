# Licensing review and remediation plan

This document records the repository's current source-based licensing findings and the maintainer's chosen remediation path. It is not legal advice.

## Current license facts

- The project is a derivative fork of [Aitum/obs-vertical-canvas](https://github.com/Aitum/obs-vertical-canvas), whose root LICENSE is the GNU General Public License, Version 2.
- The WEST TikTok-native files include source notices identifying components as `GPL-3.0-only`, including `src/tiktok-native/tiktok_request_signer.cpp` and `src/tiktok-native/live_client.cpp`.
- The direct source attribution and matching GPL-3.0 license text are recorded in `data/licenses/tiktok-live-obs.txt` and `data/licenses/tiktok-live-obs-GPL-3.0.txt`.
- The QR-code generator dependency has its own attribution in `data/licenses/qrcodegen.txt`.

The GNU project's compatibility guidance says GPL-2.0-only and GPL-3.0-only code cannot be combined into one distributed program. A GPL-2.0-or-later grant can be compatible with GPL-3.0, but this repository must not assume an `or-later` grant where the inherited project license does not provide one. See the [GNU GPL FAQ](https://www.gnu.org/licenses/gpl-faq.html#v2v3Compatibility) and [GNU license compatibility guidance](https://www.gnu.org/licenses/license-compatibility.html).

## Why a new root LICENSE does not fix this

WEST Gaming cannot change the license of code it did not author or receive relicensing rights for. Replacing the root `LICENSE` with GPL-3.0, MIT, Apache-2.0, or a custom WEST license would not remove the inherited Aitum GPL-2.0 terms. Keeping two license files in the package also does not make the two copyleft licenses compatible.

The existing attribution and license files must stay in place while the source is being audited. They document provenance; they do not grant additional permissions.

## Maintainer decision — 17 November 2026

The chosen route is **not to request a special license exception or relicense permission from third-party authors**. Instead, the TikTok integration will be remediated technically:

1. Identify every file and code section derived from a GPL-3.0-only source.
2. Remove those GPL-3.0-only-derived portions from the GPL-2.0 Aitum-based plugin.
3. Reimplement the required behavior independently, or use only an independently verified source whose license is compatible with the Aitum base.
4. Preserve notices for any third-party code that remains, and add accurate source/change attribution for the replacement implementation.
5. Audit the complete source tree and all linked dependencies again before declaring the combined plugin distributable.
6. Keep the project under the inherited GPL-2.0 terms unless a separate, valid relicensing right is established for the Aitum-derived code.

A superficial rewrite, renaming files, changing comments, or copying the same implementation through another fork does not make a clean independent implementation. The replacement must not reproduce protected implementation code from the GPL-3.0-only source.

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
