# Compatible TikTok integration research

Research status: candidate review, not a selected implementation and not a compatibility claim.

## Best candidate found so far: aemon1977/tiktok-live-native

Repository: https://github.com/aemon1977/tiktok-live-native
License: MIT (repository metadata and root LICENSE).
Inspected revision: `85a9aa56155734ba2b89bfa957b017340bcc3c3f` (2026-10-12T14:06:31Z).
License text: https://github.com/aemon1977/tiktok-live-native/blob/main/LICENSE

The project is a Linux native app, not an OBS plugin. Its `tiktok.py` module provides a different, simpler TikTok web-session path:

1. Reuses an already-authenticated browser session from Firefox or Chromium.
2. Reads TikTok cookies locally; the code makes a temporary copy of the browser cookie database and removes the temporary copy after reading.
3. Uses TikTok's web LIVE endpoints, including a CSRF handshake, to create a LIVE room.
4. Reads `data.stream_url.rtmp_push_url` from the room-creation response.
5. Uses `/webcast/room/ping/anchor/` to finish the room when the stream stops.

The project's README says it can stream in vertical or landscape formats and can hand the RTMP server/key to its bundled OBS. It does not use the TikTok LIVE Studio native-device flow or the RapidAPI signer in the inspected implementation.

### Why it is useful

- MIT is a permissive license and is compatible with GPL-2.0 when the MIT copyright and license notice are retained. See the [GNU license compatibility guide](https://www.gnu.org/licenses/license-compatibility.html).
- It provides a real, independently maintained example of the browser-cookie → LIVE-room → RTMP-push-URL → finish-room lifecycle.
- It may allow a first compatible single-feed TikTok provider without carrying over the GPL-3.0-only signing/session implementation or the unlicensed device-registration port.

### What it does not solve

- The current project is Linux-only (PipeWire, GTK/libadwaita, GStreamer, Firefox/Chromium Linux profiles). It is not a Windows drop-in.
- Its current documented flow creates one room and retrieves one RTMP push URL. It does not implement the Dual LIVE two-feed session our current integration supports.
- It does not implement the current LIVE Studio Desktop/Mobile device registration, QR login, RapidAPI quota UI, Streamlabs provider, gift/comment options, topic/game controls or safety/appeal UI in our plugin.
- TikTok's `/webcast/` endpoints are private web protocol, not an official public API; the project itself says this. Endpoint behavior may change and must be tested against a real account.
- The browser-cookie import path needs a Windows design. Firefox's profile database may be the lowest-complexity starting point; Chromium app-bound cookie protection needs separate analysis. Never read, export or log browser credentials without explicit user action and clear UI disclosure.

### Recommended experiment

Keep the existing tested integration untouched. Build a separate provider prototype on `licensing-remediation`:

- Start with an explicit user-selected Netscape cookie file or a documented browser-session import flow; never silently scrape every installed browser.
- Use the MIT project as a licensed reference/source for its browser web-session room lifecycle, preserving its LICENSE and copyright notices if any code is incorporated.
- Implement a single RTMP feed first and bind it to the existing Vertical Canvas output. Let the existing portrait/landscape canvas determine the picture shape.
- Add Windows-only tests for cookie import, expired/invalid session, room creation, stream URL parsing, stop/finish cleanup, cancellation and secret redaction.
- Do not claim Dual LIVE parity. Investigate a second-feed capability only after the single-feed path works and only using documented/independently verified behavior.
- Compare the new provider against the current user's required workflows before deciding whether it is an acceptable replacement.

This candidate is a potential route out of the licensing conflict, not proof that all existing features can be preserved. No code has been copied from it yet.
