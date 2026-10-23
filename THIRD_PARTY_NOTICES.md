# Third-party credits, source provenance and licenses

**Preserve all upstream per-file copyright statements and original license texts when publishing a derived source tree.** The exact current Git checkout is not available in this handoff, so this is a credits manifest to reconcile with the actual included files, not a legal determination that every dependency has the same license.

| Project | Contribution or inspiration | Source |
|---|---|---|
| **id Software / Quake III Arena** | Core Quake III code, cgame and q3_ui QVM source and assets interface; license in original GNU GPL distribution. Commercial data PAKs are separate. | https://github.com/id-Software/Quake-III-Arena |
| **Quake3e** (`ec-`) | Performance-oriented engine/renderer code and architecture used by the native PS5 port. Retain this project's notices and license files. | https://github.com/ec-/Quake3e |
| **ioquake3** | Community Quake III engine improvements represented in the Quake3e/VR-port ancestry as applicable to copied source. Cite if your final tree actually includes ioquake3 code. | https://github.com/ioquake/ioq3 |
| **RippeR37 / q3vr** | PCVR visual reference, per-weapon adjustment defaults, traced 3D crosshair/laser concepts, world-space HUD and virtual-screen design. Adapted source, if included, requires its licenses/notices. | https://github.com/RippeR37/q3vr |
| **Team Beef / Quake3Quest** | Standalone Quest Quake III VR implementation used as research/reference (weapon model scaling, HUD positioning, controller handling). Credit reference; verify and preserve notices if actual source copied. | https://github.com/Team-Beef-Studios/ioq3quest |
| **theghostonline / PS5VR** | PS5 native Hmd2/Tracker2 firmware-specific VR API research and architecture. Do not distribute decrypted firmware binaries or claim Sense poses without validation. Confirm exact source authors/license in your checkout. | https://github.com/theghostonline/PS5VR |
| **PS5 homebrew UI, PS5 OpenGL and SDK/toolchain** | Packaging, OpenGL-on-PS5 support, gamepad/platform glue and build environment. Add exact repo URLs, commits, contributor attributions and license texts from the actual `$PS5_HUI_DIR` and `$PS5_OPENGL_PREFIX` builds. | Installed external local dependencies |

## Optional replacement-texture author credit

**Paul Marshall (Kpax)** created the high-resolution Quake III texture replacement pack containing `xcsv_bq3hi-res.pk3`. ioquake3 lists the pack as Creative Commons-licensed, but the specific applicable terms should be checked against the distributed archive before redistributing or modifying it. We **link to the pack but do not ship it**: <https://ioquake3.org/extras/replacement_content/>. This asset pack is not part of Quake3e, Q3PSVR2, or the GPL engine source.

## Rights and distribution

- Preserve the full GPL license, original copyright headers, notices, source availability and corresponding build scripts as required by the actual derived components.
- No Sony PS5 SDK redistributables, proprietary `.sprx` binaries, console firmware, keys, console credentials, personal configuration or unlicensed media in a public release.
- Do not distribute commercial Quake III Arena `pak0.pk3` or other original game data. Each user must provide legally obtained assets.
- Quake III Arena, PlayStation 5 and PlayStation VR2 are trademarks of their respective owners. The independent homebrew project is not affiliated with or endorsed by id Software, Bethesda, Sony or Meta.
