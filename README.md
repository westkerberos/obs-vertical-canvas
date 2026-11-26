# WEST Gaming Vertical Canvas for OBS Studio

A WEST Gaming-maintained fork of [Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas) for OBS Studio. This fork is one part of a paired streaming stack with the WEST Gaming Aitum Multistream fork.

**Status: experimental development — no stable WEST Gaming release is published.**

## System architecture and required behavior

The complete feature inventory, component ownership, encoder routing, update policy, compatibility matrix and paired test plan are maintained in the [WEST Gaming Streaming Stack — Source of Truth](docs/WEST-GAMING-STREAMING-STACK.md).

The integrated system is intended to preserve:

- Twitch landscape + portrait tracks through Aitum Multistream's Twitch Enhanced Broadcasting (EBC) output.
- Vertical Canvas destinations that can reuse Twitch EBC's negotiated portrait video encoder, with an independent vertical encoder fallback.
- Native TikTok LIVE connection/runtime, including Custom RTMP, LIVE Studio Desktop/Mobile, Streamlabs and eligible Dual LIVE workflows.
- TikTok configuration from the Multistream vertical-destination workflow, with the actual TikTok runtime owned here.
- Independent recording/replay paths.

Do not replace this fork with the upstream Aitum Vertical binary: that would discard WEST Gaming's TikTok and EBC encoder-reuse changes.

## Current development status

- Current combined branch: [`feature/tiktok-native-ebc-combined`](https://github.com/westkerberos/obs-vertical-canvas/tree/feature/tiktok-native-ebc-combined)
- Current integration PR: [PR #6](https://github.com/westkerberos/obs-vertical-canvas/pull/6) — open and draft.
- Companion Multistream branch: [`feature/aitum-twitch-enhanced-broadcasting`](https://github.com/westkerberos/obs-aitum-multistream/tree/feature/aitum-twitch-enhanced-broadcasting), [PR #2](https://github.com/westkerberos/obs-aitum-multistream/pull/2).
- Latest Windows x64 CI build for this Vertical Canvas branch: [run #45851841060](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/45851841060) — passed. It is a compile/package check, not a full OBS/TikTok/EBC runtime test.
- Current fork buildspec still pins OBS 31.1.0. OBS 33.0.3 compatibility is not yet established by this CI run.
- No stable installer or GitHub Release exists.

## TikTok and licensing

The native TikTok session/output implementation belongs to this repository. The Multistream fork provides the configuration bridge and destination management; it is not a second TikTok runtime.

The current development branch still has unresolved source-license/provenance issues documented in [LICENSING-INVENTORY.md](docs/LICENSING-INVENTORY.md). Do not redistribute a combined binary or describe it as license-cleared until those issues are remediated or independently cleared.

The experimental `tiktok-relay` folder is a control-protocol prototype only; it does not implement TikTok media transport, session authentication, request signing or Dual LIVE.

## Development documentation

- [Complete stack architecture, version matrix and test plan](docs/WEST-GAMING-STREAMING-STACK.md)
- [TikTok LIVE setup, connection methods and Dual LIVE](docs/TIKTOK-LIVE.md)
- [Build and testing status](docs/BUILD-AND-TESTING.md)
- [Upstream differences](docs/UPSTREAM-DIFFERENCES.md)
- [Source and license inventory](docs/LICENSING-INVENTORY.md)
- [Licensing architecture options](docs/LICENSING-ARCHITECTURE-OPTIONS.md)
- [TikTok relay reimplementation plan](docs/TIKTOK-REIMPLEMENTATION-PLAN.md)

## Upstream

- [Aitum Vertical Canvas](https://github.com/Aitum/obs-vertical-canvas)
- [OBS Studio](https://github.com/obsproject/obs-studio)
- [WEST Gaming Aitum Multistream fork](https://github.com/westkerberos/obs-aitum-multistream)
