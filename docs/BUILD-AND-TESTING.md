# Build and verification — WEST Gaming Vertical Canvas

**Status date:** 27 November 2026  
**Target:** OBS Studio 33.0.3 · Windows x64

## Matched source branches

- Vertical Canvas 1.6.8: [main](https://github.com/westkerberos/obs-vertical-canvas/tree/main)
- Aitum Multistream 1.0.10: [main](https://github.com/westkerberos/obs-aitum-multistream/tree/main)
- OBS DirectShow patch: [obs-33.0.3-native-mjpeg](https://github.com/westkerberos/ChatGPT/tree/obs-33.0.3-native-mjpeg)

The two Aitum components must be installed and tested as a pair. Multistream owns Twitch EBC negotiation and its Main Canvas output; Vertical Canvas owns portrait destinations, EBC portrait-video encoder reuse and TikTok runtime/Dual LIVE.

## Build configuration

The Vertical Canvas and Multistream fork buildspecs are pinned to OBS source **33.0.3** and the Windows obs-deps/Qt6 snapshots dated **2026-10-23**. Vertical Canvas declares **1.6.8** and includes the optional upstream Safe Areas preview toggle, disabled by default. Multistream declares **1.0.10** and includes the upstream Vertical destination reload after OBS finishes loading.

Do not replace either fork with the upstream Aitum binary; it would remove WEST custom code. Do not copy upstream's old OBS 31.1.0 buildspec over the OBS 33.0.3 dependency pin.

## Latest Windows x64 CI

| Component | Run | Commit | Artifact | Result |
|---|---|---|---|---|
| Vertical Canvas 1.6.8 | [#46034312506](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/46034312506) | 6a05987169096d4c9c00641706766f0c6f90e6a6 | vertical-canvas-1.6.8-windows-x64-6a0598716 | **Success** |
| Multistream 1.0.10 | [#45899264700](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/45899264700) | 6a5578990456c9c55403ea9cdc51d18d3af3d5b1 | aitum-multistream-1.0.10-windows-x64-6a5578990 | **Success** |
| OBS 33.0.3 win-dshow.dll patch | [#45899269116](https://github.com/westkerberos/ChatGPT/actions/runs/45899269116) | 44d9713aa7752a07e2414edbc16328db1f176b8c | obs-33.0.3-native-mjpeg-win-dshow | **Success** |

The Aitum workflows are Windows-only; formatting and non-Windows jobs are skipped. The DirectShow run builds the OBS win-dshow target separately. These results establish compile/package success only, not complete runtime validation.

## Runtime evidence from 27 November

The user supplied two OBS 33.0.3 logs.

**12:50 session (Multistream 1.0.9, Vertical Canvas 1.6.7):**
- A TikTok start without Main Canvas failed with the explicit message that the OBS Main Canvas streaming output was unavailable.
- A later attempt before Twitch EBC was active used the independent vertical-encoder fallback.
- After Twitch EBC started, Vertical Canvas found/reused the 1080×1920 portrait track; TikTok Portrait and Landscape connected and stayed active until manual stop.
- Earlier Landscape attempts saw RTMP socket resets. The logs do not prove those resets were caused solely by the fallback encoder.

**13:01 session (Multistream 1.0.10, Vertical Canvas 1.6.7):**
- Twitch EBC started first and created five H.264 video tracks including 1080×1920.
- YouTube horizontal/Main Canvas started after Twitch.
- TikTok then reused the negotiated EBC portrait encoder; both TikTok feeds connected and remained active until manual stop.
- No TikTok RTMP send errors were recorded in that final session.

This is short-session runtime evidence for the 1.0.10 + 1.6.7 pair. It is **not** yet runtime evidence for the new 1.6.8 safe-area code, a long-duration stream or all destinations/methods.

## Required paired test plan

Use a clean OBS 33.0.3 portable profile first. Record the exact OBS build, Windows build, GPU driver, plugin commits, artifact hashes and redacted log.

1. Confirm one copy of each module loads from the OBS 33 ProgramData/portable plugin directory. No duplicate legacy copies.
2. Verify Main Canvas and Vertical Canvas scenes, switching, transforms and preview.
3. Start Twitch EBC. Confirm the Main Canvas and portrait tracks are present.
4. Start YouTube Vertical. Confirm its actual encoder object is the same negotiated 1080×1920 EBC video encoder.
5. Start YouTube Horizontal/Main Canvas.
6. Start Instagram Vertical.
7. Start TikTok Dual LIVE; confirm Portrait and Landscape, with Main Canvas already active.
8. Test missing EBC and missing Main Canvas prompts, both individual destination start and Start All. Verify cancel and explicit independent-encoder fallback.
9. Test independent encoder override and no-EBC fallback.
10. Test Custom RTMP, LIVE Studio Desktop, LIVE Studio Mobile (Camera LIVE and screen-capture restriction) and Streamlabs.
11. Disconnect one destination; confirm other outputs remain alive, retries are not duplicated and stop/shutdown cleanup is idempotent.
12. Measure NVENC/3D engine use and actual encoder counts; compare with the pre-update baseline.
13. Test profile save/reload, OBS restart, profile switch and at least ten start/stop cycles; run a long soak test.
14. Test DirectShow 4K MJPEG video-only and video+audio, MOV integrity, timing, source hide/show/deactivation/reactivation and long-duration recording.

Do not perform first-time failure injection during a critical public stream.

## Release gates

- [x] OBS 33.0.3 Windows x64 build/package: Vertical Canvas 1.6.8.
- [x] OBS 33.0.3 Windows x64 build/package: Multistream 1.0.10.
- [x] OBS 33.0.3 Windows x64 build/package: DirectShow win-dshow.dll patch.
- [x] Short-session runtime evidence: Twitch EBC, Main Canvas and TikTok Dual LIVE working with the previous 1.6.7 pair.
- [ ] Runtime check of the exact 1.6.8 Safe Areas build in OBS.
- [ ] Runtime check of YouTube Vertical and Instagram in the same EBC session.
- [ ] Full TikTok methods/Dual LIVE regression and failure/reconnect tests on the exact 1.6.8 + 1.0.10 pair.
- [ ] Real-camera DirectShow 4K MJPEG recording and A/V sync.
- [ ] Long-duration soak, ten-cycle restart tests and NVENC baseline comparison.
- [ ] Source-license/provenance blockers independently cleared or remediated.

No stable public WEST Gaming combined binary release is claimed while the licensing blockers remain open.
