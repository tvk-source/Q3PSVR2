# Quake III PSVR2 development roadmap

**Target:** native PS5 firmware 7.51, working Hmd2/Tracker2 headset path, `PPSA99431` application name Quake III PSVR2. For earlier detailed research and experiment history retain the existing `PS5_Quake3e_PSVR2_TODO.md`; do not duplicate it into Git without editing personal paths and old dead ends.

## R0: DualSense release candidate (first priority)

1. **Reliability:** reliably restore normal TV/HDMI output after VR quit and app termination. Capture failed and successful exit klogs, add bounded per-stage teardown logging, then validate 10+ cycles. Do not introduce undocumented firmware calls blindly.
2. **Remove debug side effects:** automatic PPM FBO/tiled captures off by default; `r_speeds`, `com_speeds`, per-frame console logging off; manual debug capture opt-in. Run source audit and test disk write behavior in a long gameplay session.
3. **Assets & identity:** preserve new `PPSA99431`, app title Quake III PSVR2. Use v2 `build_and_stage.sh` and verified `baseq3` contents. Ship only custom overlays and config, never commercial game assets.
4. **Interaction:** preserve current working menu. **Do not ship** the rejected vanilla `ui.qvm` overlay (CD-key screen, oversized items, still no top logo). Fix top banner inside the native UI/VR compositor and verify both eyes; the 2D `ps5vr_uiScale` already reaches its 0.35 clamp and does not restore the separate 3D banner. Separate console/bot notifications from cgame/team-chat text; verify both in-game and main menu.
5. **Preserve existing wins:** working binocular world stereo/head rotation, DualSense, gameplay-aware QVM crosshair, compact HUD, and correctly fused view-weapon stereo when enabled.
6. **Performance:** keep the viewmodel visible by default, but clearly document `cg_drawGun 0` as the proven FPS improvement. Record heavy-combat frame times, dropped VR submits and screenshot/extra-trace effects independently. Keep unnecessary per-frame logging off.
7. **Licensing/docs:** GPL source offer, full history of retained third-party copyright statements, tests, credits, known issues, no system `.sprx`, no Quake retail PAK.

## R1: optional features behind tested toggles

- Gameplay-ray laser sight **already verified with gun hidden**; polish beam/occlusion and present a visible-gun compatibility warning. Preserve `ps5vr_laserSight` option. Do not force laser-on with visible gun until muzzle and gameplay direction agree.
- Gun position, per-weapon PCVR visual profiles and aim-follow optimization, with performance guardrails.
- Proper virtual cinema/menu panel and compact world-space HUD; fix UI chat/scoreboard and re-centering.
- DualSense rumble and weapon wheel, tested in bot match.
- Comfort options (vignette on turns, eye-height offset, recenter), including controls to disable them.

## R2: tracked PSVR2 Sense controls

- Obtain and validate firmware 7.51 tracking, left/right Sense IDs, timestamps, pose validity and safety fallback.
- Map Sense tracked grip pose to Quake coordinates; weapon muzzle origin/axis, dynamic hit trace, accurate actual firing direction, laser, weapon swing and smoothing.
- Buttons/sticks/triggers, per-hand haptics and safe disconnect fallback to DualSense.
- Optional true 6DoF headset translation/crouch/lean with collision-safe body model.

## R3: polish/optimization

- Native aggregated timing instrumentation separating QVM, client, GL draw submission, GPU synchronization and VR output with debug logging disabled during FPS comparisons.
- VR spatial audio, comfort/visual calibration, low-pixel reticle artifact, art, UI localization, compatibility testing across OS firmware only where personally tested.

**Versioning:** Do not mark the untested `PPSA99431` build as 1.0. First reproducible beta/tag should come after R0 release gates and verification.

## 2026-10-23 release call

- **Do not include** `zzzzzz-q3psvr2-ui-menu-TEST.pk3` or a derived experimental `vm/ui.qvm`. Restore working menus first; a missing decorative 3D logo is preferable to unusable navigation.
- **Recommended baseline:** gun visible + laser disabled (`cg_drawGun 1; ps5vr_vmGunMode 1; ps5vr_laserSight 0`). For higher FPS, `cg_drawGun 0`; optional laser `ps5vr_laserSight 1` only if gun hidden.
- Stage only known-good assets; run `tools/release_guard.py` on the final `PPSA99431` tree before ZIP packaging or transferring to PS5.
