# Quake III PSVR2 (`q3psvr2`)

A community, native PlayStation 5 / PlayStation VR2 adaptation of **Quake III Arena**, currently developed and headset-tested on **PS5 firmware 7.51**. **Work in progress, not affiliated with id Software, Bethesda or Sony.**

> **Release update (2026-10-23):** `PPSA99431` has passed initial headset-launch testing. The attempted `ui.qvm` menu-banner replacement is **rejected**, because it introduced a CD-key screen, oversized menus, and did not restore the Quake logo. Do not ship `zzzzzz-q3psvr2-ui-menu-TEST.pk3`. The laser now works with the gun hidden, but does not align visually with the optional gun model. Release default: visible gun (`cg_drawGun 1`) and laser off (`ps5vr_laserSight 0`). For better performance, hide the gun; the laser can then be enabled separately. See [known issues](docs/KNOWN_ISSUES.md).

This repository is intended to contain the user's *existing* PS5 Quake3e native source tree, the experimental Quake III cgame QVM source, build scripts, and documentation. The companion `q3psvr2_release_candidate` handoff provides release tooling to merge into that checkout; it is **not a replacement for the existing working source tree**.

## Working functionality and current limitations

| Feature | Status (2026-10-23) |
|---|---|
| Native PSVR2 output, binocular world stereo, head orientation | **Working on the developer's 7.51 PS5** |
| Quake III gameplay with DualSense, bots and matches | **Working** |
| World-space gameplay-aware impact reticle from Quake `CG_Trace` | **Working; accuracy requires final impact checks** |
| Smoother custom reticle texture | **Working; intermittent missing pixels remain** |
| Compact gameplay ammo/health/armor HUD | **Working in initial headset test** |
| PCVR-profiled 3D gun and correct stereo fusion | **Working visually, significant FPS cost reported** |
| Optional gameplay-aim laser | **Working on PS5 with gun hidden; does not align with displayed gun** |
| Tracked PSVR2 Sense controller pose/aim/buttons/haptics | **NOT IMPLEMENTED on 7.51; interface and porting plan only** |
| Positional HMD 6DoF, true room-scale motion | **Not verified/implemented** |
| Full VR menu/console/cinematic virtual screen | **Prototype exists, not release complete; replacement UI-QVM experiment rejected** |
| Weapon wheel, vignette, seat-height calibration, haptics | **Planned, not release complete** |

> **Release rule:** Do not label the build as supporting tracked Sense weapons until controller pose validity, coordinate conversion, button mapping, collision/shot direction, and disconnect fallback are demonstrated on FW 7.51.

## Optional high-resolution textures (recommended for visual quality)

For sharper map textures, optionally install **`xcsv_bq3hi-res.pk3`**, the high-resolution texture replacement pack by **Paul Marshall (Kpax)**, distributed through [ioquake3's high-resolution texture page](https://ioquake3.org/extras/replacement_content/). Download it separately (the upstream archive is `xcsv_hires.zip`), extract the PK3, and copy it to the PS5 installation's `PPSA99431/baseq3/` directory beside your *legally obtained* original Quake III game data. Restart the game.

**Recommended for image quality, not required for gameplay.** Larger textures can increase memory pressure and may affect performance on some configurations. If performance is poor, try removing only the HD pack and comparing the same map. The developer has reported that removing it alone did not significantly improve heavy-combat FPS; hiding the weapon had a larger effect. **Do not commit or redistribute this optional PK3** in the Q3PSVR2 source repository; its license is separate from the engine code. Credit: Paul Marshall (aka Kpax).

## Customize the game

See the **[complete autoexec.cfg guide](docs/CONFIGURATION.md)** for the available PSVR2-specific settings and Quake III graphics/UI options, recommended profiles, weapon placement tuning, performance tests, and troubleshooting. Start from [`config/autoexec.example.cfg`](config/autoexec.example.cfg) and customize it for your console; do not commit personal runtime `autoexec.cfg` files.

## Publish the source to GitHub

See **[GitHub publishing instructions](docs/PUBLISHING.md)**. Publish the actual native PS5 checkout and matching QVM source patches, not just this documentation bundle. The connected GitHub integration currently cannot see the private repository; the first push can be completed from Ubuntu via GitHub CLI without a preexisting local GitHub login. Run the privacy/license preflight first and never commit proprietary `pak*.pk3`, system firmware, decrypted `.sprx`, build artifacts or personal logs.

## Requirements

- Jailbroken PS5 with a supported homebrew loader and PSVR2 connected; **tested development target: FW 7.51**.
- Build dependencies already used by this checkout: PS5 payload SDK/toolchain, `ps5-homebrew-ui`, and PS5 OpenGL bridge as configured by `ps5/tools/build-vr.sh`.
- An original, legally obtained Quake III Arena data installation. **No proprietary `pak*.pk3` is shipped by this project.**
- For QVM building: `q3lcc` and `q3asm` from the Quake III GPL source, Python 3 and standard host build tools.

## Merge these release files into your existing checkout

The native PS5 C sources are not included in this handoff. First, after extracting the handoff ZIP/TAR under `~/Downloads/q3psvr2_release_candidate`, merge the documentation, tools and bridge without touching the running game code:

```sh
python3 ~/Downloads/q3psvr2_release_candidate/tools/merge_handoff.py \
  --root ~/Downloads/PS5_Quake3e             # dry-run
python3 ~/Downloads/q3psvr2_release_candidate/tools/merge_handoff.py \
  --root ~/Downloads/PS5_Quake3e --apply
```

Existing README/other files are backed up when replaced. The merge does not apply the laser patch, rename the app or wire Sense tracking unless you deliberately perform their individual steps. It also will not stage or push to GitHub for you.

## Building the native app

On the developer's Ubuntu build machine, from the existing merged source tree:

```sh
cd ~/Downloads/PS5_Quake3e
USE_CCACHE=0 \
  PS5_HUI_DIR="$HOME/dev/ps5-homebrew-ui" \
  PS5_OPENGL_PREFIX="$HOME/dev/ps5-opengl/build/sdk/ps5-opengl-gl46" \
  bash ps5/tools/build-vr.sh
```

Before public packaging, inspect the build output's `sce_sys/param.json` to ensure that the new application identity is actually applied. The release candidate proposes:

- Homebrew title ID: **`PPSA99431`** (candidate, not reserved or guaranteed globally unique)
- Display name: **Quake III PSVR2**
- Example content suffix for newly created metadata: `Q3PSVR2HOMEBREW1` (16 characters)

**Changing a folder name alone does not rename the installed PS5 app.** Update coordinated `titleId`, `contentId`, localized `titleName`, build paths and any generated app directory, then rebuild. For an audit before editing:

```sh
python3 tools/prepare_release_identity.py --root "$PWD"
python3 tools/prepare_release_identity.py --root "$PWD" --apply
```

The script backs up metadata and known PS5 build files before touching them, and intentionally does **not** alter external `PS5_HUI_DIR` metadata or proprietary binaries. Inspect its output; ensure no stale `PPSA99303` references remain in the actual generated app. **Do not overwrite the old working installation.**

## Build and install the cgame QVM

The working gameplay reticle is an overlay with `vm/cgame.qvm`; it runs independently of `eboot.bin`. To preserve the last verified game, keep its PK3 and an `autoexec.cfg` backup. When rebuilding from the existing isolated QVM workspace:

```sh
WORK="$HOME/Downloads/PS5_Quake3e/ps5vr_vm_aim_test"
python3 "$HOME/Downloads/ps5vr_qvm_resume_compat.py" --work "$WORK"
unzip -l "$WORK/zzzzz-ps5vr-cgame-aim-test.pk3"
```

The optional laser sight was successfully headset-tested after the **laser v2** shader/beam fix. It draws against the existing gameplay-aim trace and does not require Sense poses. **Release default is the visible gun with the laser off.** If you prefer the better-performing gun-hidden option, you can optionally enable the laser: `cg_drawGun "0"; ps5vr_laserSight "1"`. Do not enable the laser with the visible gun by default: the beam and synthetic weapon model do not align. Do not advertise a physically weapon-mounted laser. Retain the laser v2 QVM patch and source; do not reapply the obsolete v1 patch.

## Configurations

**Release default: visible gun, laser off.** This prioritizes the full VR presentation; the user has confirmed the gun works in stereo, though it costs FPS in heavy combat. Keep the working compact HUD and stereo settings unchanged. Put these settings in the `autoexec.cfg` actually staged into `PPSA99431/baseq3/`:

```cfg
seta cg_drawGun "1"
seta ps5vr_vmGunMode "1"
seta ps5vr_laserSight "0"
seta ps5vr_vmReticle "1"
seta ps5vr_crosshair "0"
seta cg_drawCrosshair "0"
```

See [`config/release_default.cfg`](config/release_default.cfg). The PCVR per-weapon scale/position preferences remain configurable as before.

**Performance alternative: hide the weapon.** This produced a substantial FPS improvement in developer testing. The laser works with the gun hidden, so players can optionally enable it:

```cfg
seta cg_drawGun "0"
seta ps5vr_laserSight "1"
```

See [`config/performance_laser.cfg`](config/performance_laser.cfg). The laser is **optional**, not forced on when the gun is hidden. Disable it with `seta ps5vr_laserSight "0"` if preferred.

**Important:** Gun and laser share the same gameplay-target direction only in gun-hidden mode. With the gun visible, the current laser beam does **not** align with the synthetic weapon model. Therefore the **visible-gun release default has the laser disabled**. The FPS regression's root cause is still unproven; keep `cg_drawGun "0"` documented as the first optimization to try.

## Packaging and installation

A verified native build must produce a folder named after its title ID containing `eboot.bin` and `sce_sys/param.json`. It should also contain whatever PS5 runtime libraries your native builder requires.

```sh
python3 tools/package_release.py \
  --app /path/to/build/PPSA99431 \
  --out /path/to/dist/q3psvr2-PPSA99431.zip \
  --reticle /path/to/zzzz-ps5vr-reticle.pk3 \
  --qvm /path/to/zzzzz-ps5vr-cgame-aim-test.pk3
```

Extract the ZIP to `/data/homebrew/` so the console sees `/data/homebrew/PPSA99431/`. Place your own legally obtained Quake data in the app's `baseq3` directory. Do not copy private firmware libraries into the Git repo or public ZIP. Keep the old `PPSA99303` app and config intact until the new title launches and the headset test passes.

**Never upload the archive until:** the title metadata, executable, dependencies, QVM load precedence and headset restart have been tested on the actual PS5. Packaging ZIP success is not proof of a runnable application. **Reject any custom `baseq3/*.pk3` containing `vm/ui.qvm`**, particularly `zzzzzz-q3psvr2-ui-menu-TEST.pk3`, until built from a validated, feature-compatible UI source. Run `python3 tools/release_guard.py --app /path/to/PPSA99431` before packaging. This is read-only unless `--apply` is supplied, in which case it quarantines only the known rejected menu test archive outside the app tree.

## Reproducible QVM source for Git

Before committing, run `tools/export_qvm_patches.py` against your original Quake III GPL checkout and current patched sandbox. Commit the resulting `patches/qvm-current/` files. See [`docs/QVM_SOURCE.md`](docs/QVM_SOURCE.md). This preserves the actual modified `cg_draw.c` and `cg_weapons.c` in source-patch form rather than relying on an untracked PK3.

## Sense controller integration

The new `native/q3psvr2_sense_bridge.[ch]` files define a tested, fail-closed contract for a **future** FW 7.51 platform adapter. They are *not* wired into the engine and do not call `sceVrTracker2GetResult` yet. Read [`docs/SENSE_PORTING.md`](docs/SENSE_PORTING.md) for the ABI/firmware and controller mapping work needed. Preserve DualSense fallback whenever the Sense controller is disconnected or pose data becomes stale.

## Development and contribution

The relevant development roadmap, confirmed regressions and QA gates are in [`docs/ROADMAP.md`](docs/ROADMAP.md) and [`docs/RELEASE_CHECKLIST.md`](docs/RELEASE_CHECKLIST.md). Open a PR against `q3psvr2` with a reproducible build, firmware, console logs and headset observations. Do not commit game data, decrypted `.sprx` system libraries, personal configs or console secrets.

## Credits and third-party code

Quake III PSVR2 exists thanks to other projects, not as a standalone engine written from scratch. The contributors and codebases we must recognize are:

- **id Software**: Quake III Arena engine and GPL gameplay/UI source.
- **Quake3e** (`ec-`): engine/renderer foundation used by the PS5 native build.
- **ioquake3**: community engine work in the project's broader ancestry, credit where included.
- **RippeR37/q3vr**: PCVR gun, crosshair, laser, UI and comfort implementation references, including per-weapon visual configuration.
- **Team Beef / Quake3Quest**: Quest VR implementation research and implementation reference.
- **theghostonline/PS5VR**: PS5 native Hmd2/Tracker2 API research and inspiration.
- **PS5 homebrew UI, PS5 OpenGL and homebrew SDK contributors**: graphics/packaging/toolchain support; exact repo URLs and licenses must be added from the final compiled dependencies.

**See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)** for source links and a distinction between copied code versus reference/inspiration. Include the complete existing license files and individual copyright notices in the published Git tree. Do not distribute commercial Quake asset PAKs or Sony firmware components.

## Known issues

The confirmed remaining problems are recorded in **[docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md)**:

- **Release blocker:** intermittent black/no-TV display after quitting VR, despite some normal HDMI shutdown logs.
- **Release cleanup:** automatic debug `q3vr-fbo*.ppm` / `q3vr-tiled*.ppm` captures are **not yet verified disabled** in the native code.
- **Menu:** Quake III's separately rendered 3D banner is clipped; decreasing `ps5vr_uiScale` does not necessarily help.
- **Text:** bot/console notifications and team chat require separate VR-placement verification.
- **Performance:** the visible gun is the chosen release default, but it can substantially reduce FPS; players can use `cg_drawGun 0` for better performance.
- **Laser:** v2 is verified working with the gun hidden; it does not visually align with the visible weapon, so it defaults off.
- **Feature gaps:** no validated native Sense tracking, 6DoF physical-room movement, or advanced haptics on firmware 7.51.

## Roadmap and release gates

Read **[docs/ROADMAP.md](docs/ROADMAP.md)** for the staged R0 (stable DualSense beta), R1 (laser/UI/haptics), R2 (Sense 6DoF weapons) and R3 (optimization/polish) plan. Do not advertise features beyond those actually tested on a 7.51 console.

## Debug screenshots and release hygiene

The PPM capture call sites were identified in `ps5/q3vr/ps5_q3vr_gl.c`. A separate release patch has been prepared to disable the automatic PPM path at build time, but the newest rebuilt EBOOT still needs PS5 validation. The handoff also includes a read-only scanner:

```sh
python3 tools/audit_ppm_and_chat.py --root . --only 'PPM capture'
```

Use its matches to disable **only automatic FBO/tiled debug dumps**, preferably behind an explicit debug cvar defaulted to `0` or a `#ifdef Q3PSVR2_DEBUG_CAPTURE` build flag, while leaving regular user-triggered screenshot commands intact. Do not call a build cleaned until you've inspected its call sites and verified no automatic files are generated during a 10-minute play session.

## Menu-logo and bot-chat status: rejected UI-QVM experiment

The current native PS5 UI path already displays a usable binocular menu, but the separate top Quake III 3D banner is outside/clipped in the HMD. A v3 UI-QVM experiment built from upstream `code/q3_ui/ui_menu.c` was **headset-tested and rejected**: it caused a CD-key prompt, oversized menu options, and did not reveal the logo. The entire experimental `zzzzzz-q3psvr2-ui-menu-TEST.pk3` must be removed from the installed `PPSA99431/baseq3` and excluded from release output. Do **not** attempt to repair this by merely setting `ps5vr_menuBannerViewportY` or `ps5vr_menuBannerModelDrop`. Those cvars belong only to the rejected test UI QVM.

To restore the *sandbox source* (not needed just to remove the deployed PK3):

```sh
WORK="$PWD/ps5vr_vm_aim_test"
python3 tools/patch_menu_banner.py --work "$WORK" --restore
```

This source undo requires its original `.before-q3psvr2-menu-v3` backup and will refuse unknown modifications. **Do not rebuild/install this experimental UI QVM again.** Future investigation belongs in the working native UI projection/3D banner viewport path, not a vanilla Q3 replacement UI.

**Should we make the menu smaller to restore the logo?** Only as a brief diagnostic. We already tested `ps5vr_uiScale "0.35"` without revealing the logo; the available `vendor/Quake3e/code/renderer/tr_backend.c` safe-area implementation clamps that value to **at least 0.35**, so `0.25` cannot help without code changes. The menu banner is separately rendered as a **3D scene in a 640×120 viewport** (`code/q3_ui/ui_menu.c`), whereas `ps5vr_uiScale` shrinks the 2D safe-area viewport. This points to a 3D viewport/scissor/projection mismatch, not simply oversized 2D menu text. Do not reinstall the rejected `ui.qvm` overlay. A future fix should instrument and adjust the existing native VR 3D UI scene and its per-eye scissor bounds, while preserving current navigation and working HUD.

Bot/CPU chat text has **not been validated by this UI-QVM experiment**. Native console notify (`Con_DrawNotify`) and QVM team chat (`CG_DrawTeamInfo`) are different paths. Test and document actual affected messages before changing either.

## Release sanitization

```sh
python3 tools/release_guard.py --app /path/to/build/PPSA99431
python3 tools/release_guard.py --app /path/to/build/PPSA99431 --apply
```

The default is a read-only audit. `--apply` moves **only the known rejected UI test PK3** into a timestamped quarantine folder outside the application tree. It will not delete other custom UI overlays or modify a working gameplay QVM. The guard fails if any other non-stock UI QVM overlay remains, requiring explicit review.
