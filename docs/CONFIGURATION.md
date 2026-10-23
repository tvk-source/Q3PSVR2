# Q3PSVR2 configuration guide (`autoexec.cfg`)

**Target:** `PPSA99431` / Quake III PSVR2, native PS5 firmware 7.51, DualSense gameplay with head-tracked stereo VR. This guide documents cvars used by the current experimental build and standard Quake III. **Some `ps5vr_*` options require the patched native EBOOT; others require the custom `vm/cgame.qvm` in `zzzzz-ps5vr-cgame-aim-test.pk3`.** If a cvar appears unknown or has no effect, check which build and PK3 you actually launched. Settings are not evidence of working Sense controller tracking.

## Where to place the file

Save `autoexec.cfg` under the installed application's `baseq3/` directory, e.g. `/data/homebrew/PPSA99431/baseq3/autoexec.cfg`. For the reproducible source repository, use **`config/autoexec.example.cfg`** and keep your private installation's `autoexec.cfg` untracked. Verify the file is loaded by checking the game klog for `execing autoexec.cfg` at startup. After changes, restart Quake or run `exec autoexec.cfg` from the Quake console. `seta` saves an archived cvar setting; some renderer settings only apply on restart.

## Suggested default (weapon visible)

This matches the desired release appearance. The first-person gun is rendered in both eyes, but costs FPS during combat. The laser is off because it doesn't visually emerge from the gun muzzle.

```cfg
seta cg_drawGun "1"
seta ps5vr_vmGunMode "1"
seta ps5vr_pcvrGun "1"
seta ps5vr_laserSight "0"
seta ps5vr_vmReticle "1"
seta ps5vr_crosshair "0"
seta cg_drawCrosshair "0"
```

## FPS-oriented alternative (weapon hidden)

The developer observed a significant FPS improvement with the first-person gun disabled. The **optional** laser was tested and looks correct in this configuration.

```cfg
seta cg_drawGun "0"
seta ps5vr_laserSight "1"     // change to 0 for no laser
```

## Available VR and QVM controls

| Setting | Example / known baseline | Function and cautions |
|---|---|---|
| `cg_drawGun` | `1` (default) / `0` | Show or hide first-person model. `0` gave the largest observed combat FPS improvement. |
| `ps5vr_vmGunMode` | `1` | Use the stereo-correct weapon path (without original depth hack). Don't change casually. |
| `ps5vr_pcvrGun` | `1` | Enable PCVR-inspired weapon-specific visual profiles (not tracked Sense movement). |
| `ps5vr_vmGunScale` | `0.70`; test `0.60`–`0.90` | Global first-person weapon geometry scale. Larger value makes gun larger. |
| `ps5vr_vmGunForward` | `0` | Legacy gun visual forward adjustment; leave at zero with PCVR profile as baseline. |
| `ps5vr_vmGunRight` | `0` | Legacy right adjustment; leave at zero as baseline. |
| `ps5vr_vmGunDown` | `0` | Legacy down adjustment; leave at zero as baseline. |
| `ps5vr_pcvrHandForward` | `18`; optional `22` | Synthetic hand forward positioning. Can alter stereo depth and near-eye comfort. |
| `ps5vr_pcvrHandRight` | `7`; optional `12` | Synthetic hand right offset. |
| `ps5vr_pcvrHandDown` | `8`; optional `12` | Synthetic hand down offset. |
| `ps5vr_pcvrAimFollow` | `1`; test `0` | Turn the *visual model* toward the aim target; **does not change bullet trajectory**. May add client overhead. |
| `ps5vr_laserSight` | `0` default; `1` optional | Gameplay-ray laser. **Use with `cg_drawGun 0`** for correct-looking alignment. Requires laser v2 custom cgame QVM. |
| `ps5vr_vmReticle` | `1` | Use QVM gameplay-aware 3D reticle. Requires matching custom QVM and reticle assets. |
| `ps5vr_vmReticleAngle` | `2.5` | Size of reticle. Try `2.0` or `3.0` if too small or big. |
| `ps5vr_vmReticleRange` | `4096` | Max reticle trace distance (Quake world units). |
| `ps5vr_vmReticleOffset` | `2.0` | Fine adjustment of reticle off the impact surface. |
| `ps5vr_vmReticleDepthHack` | `0` | Leave depth hack off for credible scene occlusion; `1` was experimental. |
| `ps5vr_crosshair` | `0` | **Old native reticle**. Must be off if the QVM reticle is on, to avoid duplicates. |
| `ps5vr_crosshairTrace` | `0` | Earlier renderer-only static collision trace, rejected in headset test. |
| `cg_drawCrosshair` | `0` | Classic 2D Quake crosshair, disabled in favor of VR reticle. |
| `ps5vr_eyeScale` | `1.0`; optional `0.90` | Relative eye-position offset. `1.0` was preferred; `1.10` felt too strong in testing. |
| `ps5vr_uiPanel` | `1` | Experimental head-locked stereo interface panel; keep enabled in the working build. |
| `ps5vr_uiScale` | `0.45` | 2D VR UI safe-area scale; `0.35` is the known clamp minimum in the tested native backend. **Does not fix missing 3D menu logo**. |
| `ps5vr_pcvrHud` | `1` | Compact gameplay HUD approximation (not the PCVR in-world HUD). |
| `ps5vr_pcvrHudScale` | `0.75` | Compact HUD scale. |
| `ps5vr_pcvrHudY` | `423` | HUD vertical position in the traditional 640x480 UI space. |
| `cg_draw3dIcons` | `0` | Switch off 3D HUD icons; avoids previous single-eye display issue. |
| `cg_drawIcons` | `1` | Keep 2D HUD icons visible. |
| `cg_drawStatus` | `1` | Show status bar including health/armor/ammo. |

These values are project-specific or build-dependent; not every branch of Q3PSVR2 will include every cvar. The sample reflects the tested configuration discussed during development, not guaranteed defaults in all future releases.

## Graphics, combat effects and performance

| Cvar | Typical / test value | Use |
|---|---|---|
| `cg_drawFPS` | `0` release / `1` test | On-screen Quake FPS. Doesn't directly measure actual HMD submit rate. |
| `com_speeds` | `0` | Detailed client/server frame timings; can produce huge logs when enabled. |
| `r_speeds` | `0` | Renderer counters; may be cheat-protected, not recommended for normal play. |
| `com_blood` | `1` / test `0` | Show or disable blood particles. |
| `cg_gibs` | `1` / test `0` | Show or disable gibs. |
| `cg_marks` | `1` / test `0` | Show or disable impact/bullet decals. |
| `cg_brassTime` | `2500` / test `0` | Shell casing lifetime in milliseconds. |
| `r_dynamiclight` | `1` / test `0` | Dynamic lighting from effects and weapons; test independently. |
| `r_picmip` | `0` / test `1` | Texture mip bias; may require restart and can change visual quality. |

Change **one setting at a time** and compare FPS in the same map/bot configuration. Removing the optional HD textures alone had little benefit in the developer's heavy-combat test, whereas hiding the gun helped substantially.

## Menu controls: known limitation

The 3D Quake III banner/logo is still missing/clipped. The following two cvars **belong to a rejected experimental `ui.qvm`** and must not appear in release configs:

```cfg
// DO NOT USE: experimental UI QVM failed on headset
// ps5vr_menuBannerViewportY "12"
// ps5vr_menuBannerModelDrop "24"
```

Do **not** ship `zzzzzz-q3psvr2-ui-menu-TEST.pk3`: it brought back the CD-key prompt, made menu options oversized, and still did not restore the logo. Work instead on the native 3D UI viewport/scissor/projection while retaining the last working UI.

## Suggested weapon placement experiment

Only after saving the working config, try:

```cfg
seta ps5vr_vmGunScale "0.90"
seta ps5vr_pcvrHandForward "22"
seta ps5vr_pcvrHandRight "12"
seta ps5vr_pcvrHandDown "12"
```

This enlarges the gun and moves the synthetic hand right/down. It may change stereo comfort: test with the headset, not only screenshots. Restore `0.70 / 18 / 7 / 8` if uncomfortable. **No Sense controller poses or physical muzzle aiming are implemented** in this version.

## Troubleshooting

- **Changes seem ignored:** Confirm `execing autoexec.cfg` in klog; verify whether the setting lives in native EBOOT or in the custom cgame QVM, and confirm `vm/cgame.qvm` from the expected PK3 is loaded. Check that PK3s are staged into the new **PPSA99431**, not the legacy app directory.
- **Laser invisible:** Requires corrected laser v2 cgame QVM, not only a config line. The original laser attempt had a rail-shader registration issue.
- **Gun appears doubled:** Verify `ps5vr_vmGunMode 1` and working stereo EBOOT/QVM pairing.
- **FPS drops:** First compare `cg_drawGun 1` vs `0`; then A/B dynamic lighting and combat effects. Keep verbose performance logging disabled.
- **Menu unusable / asks for CD key:** Remove `zzzzzz-q3psvr2-ui-menu-TEST.pk3` and restart. Do not replace working UI with vanilla `ui.qvm`.
- **TV blank on quit:** Still an unresolved exit/HDMI ownership issue. Report a failed-quit klog; no proven fix yet.
- **Automatic PPM captures:** A separate native build patch disables diagnostic captures, but confirm the installed EBOOT is rebuilt and no captures appear during play.
