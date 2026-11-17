# Licensing and architecture options

This is an engineering decision record, not legal advice. Its purpose is to avoid changing license labels while preserving as much of the tested functionality as the source rights allow.

## Current facts

The existing WEST Vertical Canvas branch compiles TikTok-specific modules into the same OBS plugin module as the Aitum-derived GPL-2.0 code. The source audit identifies at least 24 integration files derived from GPL-3.0-only sources and a separate device-registration path ported from a repository with no published license. See [LICENSING-INVENTORY.md](LICENSING-INVENTORY.md).

The current single-module build is not cleared for distribution. Replacing the root LICENSE does not solve that.

## Option A — Independently reimplement inside Vertical Canvas

Keep one OBS plugin under the inherited Aitum GPL-2.0 terms. Replace every GPL-3.0-only-derived and unlicensed component with independently authored, GPL-2-compatible implementations.

Advantages:
- One plugin and the cleanest installation story.
- No helper process or additional service management.
- The entire user-facing Vertical Canvas flow remains in one project.

Costs and risks:
- Large scope: signer, LIVE session, device registration, QR, frame signing, SEI metadata, OBS packet adapter, Streamlabs token discovery and account/topic/safety features.
- Some TikTok mechanisms are private/undocumented and may be unstable.
- Device-registration code currently comes from a repository with no published license. Its replacement may be particularly difficult.
- There is no assurance that every existing feature, especially Dual LIVE, can be preserved.

## Option B — Separate TikTok relay process (candidate to preserve more functionality)

Split the system into two independent programs:

1. **WEST Vertical Canvas plugin (GPL-2.0):** remains an OBS plugin. It renders the existing portrait/landscape canvases and sends normal RTMP/FLV streams to a local loopback endpoint. It must not link against or embed the TikTok signer/session implementation.
2. **WEST TikTok Relay (GPL-3.0):** a separate executable/process that owns the TikTok account/session, signing, frame metadata, remote ingest connections and room cleanup. It listens for ordinary RTMP/FLV input from the local plugin and forwards it to TikTok.

For Dual LIVE, the plugin would open two local RTMP inputs (portrait and landscape); the relay would pair them with the two remote TikTok ingest feeds from one LIVE session.

### Why this boundary may help

The GNU GPL FAQ says the distinction between separate programs and one combined work depends on both the communication mechanism and the semantics. It identifies pipes, sockets and command-line arguments as mechanisms normally used between separate programs, while warning that exchanging complex internal data structures may still indicate one larger program. See [GNU GPL FAQ — What is the difference between an aggregate and a combined work?](https://www.gnu.org/licenses/gpl-faq.html#MereAggregation) and [GNU GPL FAQ — When are programs and plugins a single combined program?](https://www.gnu.org/licenses/gpl-faq.html#GPLPlugins).

The design must therefore enforce an actual process boundary:
- Separate executable and address space.
- Standard RTMP/FLV media protocol over localhost, not shared memory.
- No linked libraries or shared internal C++ structs between the programs.
- A small, documented control protocol for start/stop/status only; do not exchange internal OBS objects.
- Separate build targets, source trees, license files and package manifests.
- The installer may bundle the two programs as an aggregate, but must not fuse them into one executable or shared module.

This is a plausible engineering route, not a legal conclusion. The exact control/media protocol and bundled distribution should be reviewed by qualified counsel before release.

### Remaining blocker

The relay cannot simply include the current device-registration port from `Loukious/TikTokStreamKeyGenerator`: that source has no published license. That device path still needs a separately licensed replacement or must be disabled. A relay split resolves the GPL-2/GPL-3 module conflict only if the programs remain separate; it does not grant rights to unlicensed source.

## Option C — Reuse a compatible MIT web-session provider

The project [aemon1977/tiktok-live-native](https://github.com/aemon1977/tiktok-live-native) is MIT-licensed and demonstrates browser-cookie session reuse, LIVE room creation, RTMP push URL retrieval and room shutdown.

It could be adapted as a simpler single-feed provider in the GPL-2.0 plugin with its MIT notices preserved. The inspected implementation is Linux-only and does not provide the current LIVE Studio Desktop/Mobile flow or Dual LIVE. See [COMPATIBLE-TIKTOK-ALTERNATIVES.md](COMPATIBLE-TIKTOK-ALTERNATIVES.md).

## Option D — Change the whole Vertical Canvas fork to GPL-3.0

Not available based on the rights currently established. WEST Gaming cannot change Aitum's inherited GPL-2.0 code to GPL-3.0 without a valid later-version grant or the relevant copyright holders' permission. Do not pursue this by replacing the root LICENSE.

## Current recommendation

Do not rewrite everything or remove features yet. Preserve the tested branch and prototype **Option B** at the architecture level first, because it offers a potential way to retain the GPL-3.0 TikTok session/signing implementation without compiling it into the GPL-2.0 Vertical Canvas module. In parallel, evaluate a lawful replacement for the no-license device-registration path.

If a true process boundary cannot be implemented and reviewed, fall back to Option A or a reduced Option C provider. Do not publish a combined binary release until the chosen architecture and all source provenance are cleared.
