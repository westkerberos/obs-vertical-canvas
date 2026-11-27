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
| Vertical Canvas 1.6.8 | [#46036075188](https://github.com/westkerberos/obs-vertical-canvas/actions/runs/46036075188) | merged main `8c64a52994fd10fbc2414ea7bf98e7ee02975f42` | vertical-canvas-1.6.8-windows-x64-8c64a5299 (artifact ID `15217901109`) | **Success** |
| Multistream 1.0.10 | [#46036486335](https://github.com/westkerberos/obs-aitum-multistream/actions/runs/46036486335) | merged main `6d4c0922f60ddd0012778c69dc7584a0cef55d15` | aitum-multistream-1.0.10-windows-x64-6d4c0922f (artifact ID `15219040196`) | **Success** |
| OBS 33.0.3 win-dshow.dll patch | [#46035805651](https://github.com/westkerberos/ChatGPT/actions/runs/46035805651) | source head `80c45786d1cb4659a364b17580d937aaeaa518dc`; merged main `ae7cc2dbcf71a3f6be74579577d47fd90fa452b3` | obs-33.0.3-native-mjpeg-win-dshow (artifact ID `15215599998`) | **Success** |

The Aitum workflows are Windows-only; formatting and non-Windows jobs are skipped. The DirectShow run builds the OBS win-dshow target separately. These results establish compile/package success only, not complete runtime validation.

### Verified artifact hashes

- Vertical Canvas 1.6.8 outer Actions ZIP: `be84f005d1ad0046a24313153c36ed8428124e898d8ca7e206e63fb95600eee1`; ProgramData ZIP: `58b617539bb7c2c1dee53db2e9a719f49f8453b68fb35e7a97cb94f39effe813`; DLL: `3636abb099d425373911b507a8da344bc3169bd0e30d41827d6fbe735a8845db`.
- Multistream 1.0.10 outer Actions ZIP: `f2d08b6b0832c336e80e4e65eb5a9dfc6f7f679f4c2ae3104d1273441c440d04`; ProgramData ZIP: `5b16036008d5cb6781880c0dfd493e84a5c8c5512f2aeaec7b19b6bd6a1e69c3`; DLL: `47a39f4236282b1bfbd58d28b22dd74ed6f1072446d63baabf4d1a891ddbe0a3`.
- DirectShow win-dshow Actions ZIP: `88dff9f2b12b735370d5f7928fef91a8c476627c67a43f4b2e813b3777cd9cc6`; DLL: `ebd7b77e9da422f9a6ed72bc4661c8387e8889d1bcc1d5efd1bac2fcf1ecf9ad`.

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

This is short-session runtime evidence for Multistream 1.0.10 + Vertical Canvas 1.6.7. It is **not** yet runtime evidence for the exact 1.6.8 + 1.0.10 pair, a long-duration stream or all destinations/methods.

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
- [x] Missing EBC/Main Canvas pre-flight prompts included in the Vertical Canvas 1.6.8 Windows x64 build (runtime prompt behaviour still pending).
- [ ] Runtime check of the exact 1.6.8 Safe Areas build in OBS.
- [ ] Runtime check of YouTube Vertical and Instagram in the same EBC session.
- [ ] Full TikTok methods/Dual LIVE regression and failure/reconnect tests on the exact 1.6.8 + 1.0.10 pair.
- [ ] Real-camera DirectShow 4K MJPEG recording and A/V sync.
- [ ] Long-duration soak, ten-cycle restart tests and NVENC baseline comparison.
- [ ] Source-license/provenance blockers independently cleared or remediated.

No stable public WEST Gaming combined binary release is claimed while the licensing blockers remain open.
