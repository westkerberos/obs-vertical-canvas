# Build and verification — WEST Gaming Vertical Canvas fork

## Scope and matched companion

This guide applies to the combined WEST Gaming branch:

- Vertical Canvas: `feature/tiktok-native-ebc-combined` — [PR #6](https://github.com/westkerberos/obs-vertical-canvas/pull/6).
- Companion Multistream: `feature/aitum-twitch-enhanced-broadcasting` — [PR #2](https://github.com/westkerberos/obs-aitum-multistream/pull/2).

The complete architecture and version matrix are maintained in [WEST Gaming Streaming Stack — Source of Truth](WEST-GAMING-STREAMING-STACK.md). These two branches must be tested together. The feature branch for Vertical Canvas includes TikTok-native runtime, TikTok Dual LIVE, Twitch EBC portrait-encoder reuse, and lifecycle/crash protections; the Multistream branch owns Twitch EBC negotiation/output and the TikTok configuration bridge.

## Build configuration

This Windows x64 fork is now pinned to the OBS **33.0.3** source archive and the matching Windows dependency/Qt snapshots dated 2026-10-23. The current plugin version remains **1.6.7** deliberately: the branch already contains Aitum 1.6.8's scene-list loading/order fix and the OBS 33 ProgramData installer path, but it does not include Aitum 1.6.8's new Safe Areas preview option. That optional UI/rendering feature is excluded to preserve the existing WEST feature set without adding functionality.

The companion Multistream branch is pinned to OBS **33.0.3** and declares Aitum Multistream **1.0.10**. Build and test the pair from recorded commit SHAs; do not substitute either official Aitum binary because that would remove WEST's EBC/TikTok integration.

## Latest known CI

- Workflow: [#45863996075](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/45863996075)
- Branch: `feature/tiktok-native-ebc-combined`
- Commit: `407799eb58dca76eb4fa55946c8e177810b4726b`
- Result: Windows x64 build **success** against the current OBS 33.0.3 buildspec; Ubuntu/macOS **skipped**; formatting **skipped**.
- Artifact: `vertical-canvas-1.6.7-windows-x64-407799eb5` (includes the OBS 33 ProgramData package and installer).
- The test step runs `west-tiktok-relay-control-protocol`, which validates JSON-lines control protocol only. It does not connect to TikTok, carry media, test RTMP/FLV, authenticate, sign requests or test Dual LIVE.

This CI run proves Windows x64 compilation and packaging from the OBS 33.0.3 dependency pin. It does not prove runtime behavior in OBS, Twitch EBC negotiation, shared NVENC encoder identity, TikTok session lifecycle or full multi-destination streaming.

## Existing runtime evidence

The repository's earlier maintainer smoke-test record (18 November 2026) reports approximately 30 minutes in OBS 33.0.1 on Windows covering TikTok Landscape, Portrait and Dual LIVE, with the separate OBS Multi-RTMP plugin disabled. That is useful evidence for the earlier TikTok integration on one setup. It is **not** evidence that the current combined PR #6 + Multistream PR #2 pair has passed EBC/encoder-reuse regression testing on OBS 33.0.3.

## Required paired test plan

Run in a clean OBS 33.0.3 portable copy using the new plugin directory structure. Record the exact OBS commit/build, Windows build, GPU driver, branch heads and DLL hashes.

1. Verify only one copy of each plugin loads; inspect OBS startup logs for missing modules, missing dependencies or legacy duplicate modules.
2. Load Main Canvas and Vertical Canvas scenes; test scene ordering, scene switching, source transforms and canvas preview.
3. Start Twitch Enhanced Broadcasting and verify both Main Canvas landscape and Vertical Canvas portrait tracks are present.
4. For each vertical destination selected for Twitch EBC, verify the attached OBS video encoder object is the exact negotiated portrait track—not merely the same codec/resolution. Verify all opted-in targets reuse it.
5. Start YouTube, TikTok and Instagram vertical outputs individually and together. Test Start All, individual start/stop, saved settings and profile reload.
6. Test the independent vertical encoder fallback when EBC is unavailable and when a destination explicitly selects its own encoder.
7. Test TikTok Custom RTMP, LIVE Studio Desktop, LIVE Studio Mobile and Streamlabs Desktop separately. Redact cookies, RapidAPI keys and tokens from all logs.
8. Test TikTok Dual LIVE with an eligible account: Main Canvas active first; verify portrait and landscape feeds, one-feed failure, manual stop and session cleanup.
9. Force one destination's network connection to reset. Verify OBS remains alive, other independent outputs stay up, retries do not create duplicate sessions/encoders, and teardown is idempotent.
10. Measure NVENC/3D engine use with Twitch EBC plus three vertical destinations. Record encoder identity/count, not only Task Manager percentages.
11. Test restart, profile switch, OBS exit and at least ten start/stop cycles; follow with an extended soak test.

Do not run first-time failure/reconnect experiments during a critical public broadcast.

## Release gates

- [x] OBS 33.0.3 Windows x64 dependency/toolchain builds succeed for both matched plugin branches.
- [x] Aitum Vertical 1.6.8 scene-list fix is present; the Safe Areas feature is explicitly omitted to avoid adding a function.
- [x] New Windows ProgramData plugin path is packaged; portable-package layout is included in the artifact.
- [x] Multistream 1.0.10 vertical-output reload fix is present in the companion fork.
- [ ] Paired Twitch landscape + portrait EBC test passes.
- [ ] Every opted-in vertical target reuses the same EBC portrait video encoder.
- [ ] TikTok connection methods and Dual LIVE pass regression testing.
- [ ] Disconnect/reconnect/shutdown tests pass without OBS crash or unrelated output loss.
- [ ] DShow 33.0.3 test build is validated independently.
- [ ] TikTok source-license/provenance issues in [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md) are remediated or independently cleared.
- [ ] Release contains compatible matched plugin builds, exact version/commit matrix, hashes and known limitations.

No stable WEST Gaming installer or GitHub Release is published for this combined branch.
