# Known issues and release gates (2026-10-23)

This file distinguishes **observed failures**, **unvalidated experiments**, and **feature gaps**. The new title `PPSA99431` / Quake III PSVR2 **has launched successfully** on the developer's FW 7.51 console. The entire product is NOT release-validated.

| Priority | Issue | Evidence / workaround | Status |
|---|---|---|---|
| **Release blocker** | PS5 HDMI/TV output sometimes remains blank or routed only to the PSVR2 headset after quitting the app. | Intermittent. Existing klogs show clean exits in some runs. Collect *failed-quit* and normal-quit klogs; verify the native stop/reprojection/VideoOut ownership sequence. Do not guess 7.51 API offsets. | **OPEN** |
| **Release blocker** | Automatic `q3vr-fbo*.ppm` and `q3vr-tiled*.ppm` captures may still happen. | Capture call sites were audited in `ps5/q3vr/ps5_q3vr_gl.c`; a default-off native compile-time patch is prepared, but the *installed* rebuilt EBOOT has not been re-verified to suppress PPM files. Run `tools/audit_ppm_and_chat.py --root <checkout> --only 'PPM capture'`. Keep ordinary manual screenshot behavior. | **PATCH PREPARED; CONSOLE CHECK NEEDED** |
| High | First-person gun significantly lowers performance in combat. | Developer observed material FPS improvement with `cg_drawGun 0`. Client-heavy timing in VR17; still need isolate mesh, muzzle effects, view-model transformation and trace overhead. The release default intentionally keeps the gun visible; `cg_drawGun 0` is the documented FPS optimization. | **OPEN** |
| **High** | Main-menu Quake III 3D logo missing/clipped in headset. | The v3/v4 separate `ui.qvm` experiment caused a CD-key prompt, huge menu entries, **still no logo**. Earlier `ps5vr_uiScale 0.35` also failed; the native 2D viewport code clamps scale at 0.35, while the banner uses its own 3D viewport. **Rejected.** Remove `zzzzzz-q3psvr2-ui-menu-TEST.pk3`; keep working native UI until its projection/viewport is fixed. | **OPEN; TEST QVM MUST NOT SHIP** |
| Medium | Bot/chat notifications do not share the working gameplay HUD framing. | Standard console notifications may be native `Con_DrawNotify`; team chat is `CG_DrawTeamInfo` in `cgame.qvm`. Determine which path is affected before modifying it. Avoid applying only a cgame fix to a native-console bug. | **OPEN** |
| **Medium** | Gun-visible laser beam does not align with the displayed weapon. | Laser v2 **works in headset when `cg_drawGun 0`**. With the synthetic gun visible the beam points along gameplay trace, not the model muzzle. Keep independent option, release default is visible gun + laser off; with gun hidden, the laser may optionally be enabled using `cg_drawGun 0; ps5vr_laserSight 1`. | **LASER WORKING; GUN ALIGNMENT OPEN** |
| Low | A few pixels intermittently disappear from round crosshair. | Smoothed artwork accepted; gameplay reticle otherwise works. | **DEFERRED** |
| Feature gap | Tracked Sense controllers, actual Sense weapon aiming, 6DoF room scale, Sense haptics. | FW 7.51 native controller results and ABI unvalidated. The existing Sense bridge is an interface, not a driver. | **NOT IMPLEMENTED** |
| Feature gap | Proper stereo-projected virtual screen, cinematics, weapon wheel and comfort settings. | Experimental UI safe area working; production-quality presentation and configurable options not complete. | **ROADMAP** |

## Shipping policy

A candidate can ship a tested DualSense-only VR mode if the app restores TV output consistently, no unintended capture files are produced, first-run assets are automatically staged, **the rejected experimental UI QVM is excluded**, required GPL notices and source are published, and menu/chat interaction is usable. Do not claim Sense controller support, full room-scale 6DoF or verified laser support prematurely.

## Diagnostic commands

```sh
python3 tools/audit_ppm_and_chat.py --root . --only 'PPM capture'
python3 tools/audit_ppm_and_chat.py --root . --only 'Native console/chat drawing'
python3 tools/audit_ppm_and_chat.py --root . --only 'VR UI projection'
rg -n 'q3vr-fbo|q3vr-tiled|\\.ppm|Con_DrawNotify|Main_MenuDraw|RB_SetGL2D' ps5 vendor code 2>/dev/null
```

The audit locates source-level references and **does not itself disable** captures or establish where rendering time is spent. A native source patch must be based on the specific output.

## Verified follow-up (2026-10-23)

- **Menu:** Experimental `ui.qvm` was built/deployed and failed headset verification. The change is **not** a viable fix; remove from installed app and stage/build outputs. The CD-key prompt and oversized UI are regressions from using a replacement vanilla UI baseline; exact cause of missing logo in working native menu still to investigate.
- **Laser:** The revised laser now renders and aligns satisfactorily **with the gun hidden**; keep it available as a user-facing option but recommend gun-hidden configuration. Do not promise visual muzzle alignment with gun enabled.
- **Documented performance alternative:** `cg_drawGun 0` improves performance and avoids laser/model mismatch while retaining the compact HUD and QVM reticle. The default remains visible gun + laser off.

## 2026-10-23 presentation choice

- **Ship gun visible by default** (`cg_drawGun 1`, `ps5vr_vmGunMode 1`, `ps5vr_laserSight 0`). Document `cg_drawGun 0` as the easiest FPS improvement, supported by headset testing. The laser remains optional and is recommended only with the gun hidden until muzzle alignment is solved.
- **Do not shrink the menu further as a release fix.** `ps5vr_uiScale` reached its 0.35 clamp in previous experiments without bringing back the independently rendered 3D logo. Fix 3D menu scene bounds/viewport/scissor in the existing native renderer; reject stock/rebuilt `ui.qvm` overlays.
