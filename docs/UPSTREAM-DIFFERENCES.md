# WEST Gaming Vertical Canvas — upstream differences and update ledger

**Status date:** 27 November 2026  
**Upstream:** [Aitum/obs-vertical-canvas](https://github.com/Aitum/obs-vertical-canvas)  
**WEST branch:** [release/obs33.0.3-vertical-1.6.8-west-stack-20261127](https://github.com/westkerberos/obs-vertical-canvas/tree/release/obs33.0.3-vertical-1.6.8-west-stack-20261127)

## Upstream Aitum Vertical 1.6.8 changes carried

| Upstream commit | Change | WEST status |
|---|---|---|
| [fde49d2](https://github.com/Aitum/obs-vertical-canvas/commit/fde49d2339af4180cf014e4d7a23fc0ebccd9324) | Scene-list loading/order fix | Present in the prior WEST branch |
| [3751ef7](https://github.com/Aitum/obs-vertical-canvas/commit/3751ef743337b9e7635f52d4e8fc74011596d8d0) | Optional Safe Areas preview UI/rendering | Ported; disabled by default; user can enable in Vertical Canvas settings |
| [d7e88a5](https://github.com/Aitum/obs-vertical-canvas/commit/d7e88a551c7a842ccecacdd7c1b2fe5e2d076046) | Version bump to 1.6.8 and installer layout | Version/manifest bumped to 1.6.8; OBS 33 ProgramData installer layout already existed in WEST branch |

The official upstream buildspec is older (OBS 31.1.0 / 2025 dependencies). WEST retains the tested OBS 33.0.3 source pin and Windows dependency/Qt snapshots dated 2026-10-23. Do not wholesale copy upstream buildspec/CMake dependency changes.

## WEST additions retained

- Native TikTok destination/runtime and all documented connection methods.
- TikTok account/session, QR, cookies, device profiles, signing, metadata/safety and Dual LIVE UI.
- TikTok Portrait + Landscape automatic encoder reuse and advanced manual layout override.
- Twitch EBC portrait-video encoder lookup/reuse and independent vertical encoder path.
- Multistream proc-handler integration and existing target-management behaviour.
- Thread-safe output stop/session lifetime fixes and diagnostics.
- Pre-flight prompts for missing EBC portrait track and missing Main Canvas output.
- No duplicate setLayout(mainLayout) call in the dock constructor.

## Aitum Multistream 1.0.10 upstream changes carried in companion fork

- [Aitum commit 64ef323](https://github.com/Aitum/obs-aitum-multistream/commit/64ef3233a8b078268adf68ec8427d0bfba68678d): call LoadVerticalOutputs(false) after LoadSettingsFile() when OBS finishes loading.
- [Aitum commit 41329c5](https://github.com/Aitum/obs-aitum-multistream/commit/41329c56d8f1254b7b55a2821a44b08abeeab3bb): version 1.0.10 and OBS 33 ProgramData installer paths.

The WEST Multistream branch retains TikTok destination configuration bridge and Twitch EBC negotiation/output; official upstream binaries are not drop-in replacements.

## Maintenance rules

1. Preserve the paired stack architecture documented in [WEST-GAMING-STREAMING-STACK.md](WEST-GAMING-STREAMING-STACK.md).
2. Keep the TikTok runtime inside Vertical Canvas; keep Multistream as UI/config bridge plus Twitch EBC owner.
3. Preserve OBS 33.0.3 buildspec and matching dependencies unless the target OBS version changes.
4. Treat OBS output lifecycle/thread/ref changes as high-risk; review callbacks, object ownership and stop/reconnect logic.
5. Update [UPDATE-PROCEDURE.md](UPDATE-PROCEDURE.md), component version matrix, CI links and runtime evidence for every upstream port.
