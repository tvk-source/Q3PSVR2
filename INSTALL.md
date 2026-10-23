# Q3PSVR2 release defaults v6, 2026-10-23

## What users actually tested

- `PPSA99431` launches and runs on firmware 7.51.
- The experimental `zzzzzz-q3psvr2-ui-menu-TEST.pk3` **fails**: CD-key prompt, oversized menu, no Quake logo. **Remove it.**
- Revised gameplay-ray laser sight works when the gun is hidden. When gun is shown, the laser does not point where the model points. Keep laser optional; the release defaults to gun visible with laser off. Gun-hidden/laser-on is the documented FPS alternative.
- Display/TV restoration after quit, bot chat issues, gun-on FPS degradation, HMD 6DoF and Sense controllers remain unresolved unless separately verified.

## Immediate fix on the PS5

Remove (or rename to `.pk3.disabled` so Quake cannot mount it) the following from `/data/homebrew/PPSA99431/baseq3/`:

`zzzzzz-q3psvr2-ui-menu-TEST.pk3`

Do not delete the gameplay QVM, reticle PK3 or retail game data. Restart the app. The experimental banner configuration cvars may be removed from `autoexec.cfg`; they do nothing without the experimental UI.

## Restore the patched test source on Ubuntu (optional, recommended before release)

```sh
cd ~/Downloads/PS5_Quake3e
WORK="$PWD/ps5vr_vm_aim_test"
python3 tools/patch_menu_banner.py --work "$WORK" --restore
```

This command expects the script from the v3 docs bundle and the `ui_menu.c.before-q3psvr2-menu-v3` backup. It refuses to overwrite unrecognized modifications. Do **not** rebuild or deploy the original-source UI QVM again.

## Install corrected Git documentation and release guard

Extract `q3psvr2_release_defaults_v6.zip` to `~/Downloads`. Then:

```sh
cd ~/Downloads/PS5_Quake3e
python3 ~/Downloads/q3psvr2_release_defaults_v6/tools/install_correction.py --root "$PWD"
python3 ~/Downloads/q3psvr2_release_defaults_v6/tools/install_correction.py --root "$PWD" --apply
```

This operation backs up replaced docs and only adds an audit tool; it does not modify native game code, cgame QVM, or your autoexec.

Check your **local built app** (not the PS5 FTP path):

```sh
python3 tools/release_guard.py --app /path/to/PPSA99431
python3 tools/release_guard.py --app /path/to/PPSA99431 --apply
```

The guard defaults to read-only; `--apply` quarantines only the known rejected test UI PK3 in a timestamped folder **outside** the app tree. Other custom UI overlays are reported as blockers, never silently moved. Run it again after every build/stage and before packaging. Do not run the `.zip` or other resources automatically in the PS5.

## Release gameplay settings

**Default visible gun** (`config/release_default.cfg`):

```cfg
seta cg_drawGun "1"
seta ps5vr_vmGunMode "1"
seta ps5vr_laserSight "0"
seta ps5vr_vmReticle "1"
```

**Optional higher-FPS, gun-hidden mode** (`config/performance_laser.cfg`):

```cfg
seta cg_drawGun "0"
seta ps5vr_laserSight "1"
seta ps5vr_vmReticle "1"
```

Gun-hidden mode improved FPS in headset testing. The laser remains an independent option: set it to `0` if not wanted. The laser beam and visible model do not yet align, so the visible-gun default leaves the laser **off**.

The documentation installer does **not** edit `autoexec.cfg`. Manually copy just one profile into the source config staged by your release tooling, then verify the installed `PPSA99431/baseq3/autoexec.cfg`. No EBOOT/QVM rebuild is needed just for these cvar changes.

## What remains for the top logo

The menu logo is a separately rendered 3D banner in a 640×120 UI viewport, and the existing native VR menu projection seems to clip it. Previous testing at `ps5vr_uiScale 0.35` did not expose it. The native 2D safe-area code clamps the cvar to a minimum 0.35; reducing it further is not a useful test. Fix the existing native VR 3D banner viewport/scissor/projection rather than replacing the whole UI QVM. The v3/v4 standalone UI QVM experiment is invalid as a fix because it replaces the working menu code. A proper change requires inspecting the working native UI draw path and its viewport/FOV/panel transforms, then testing both eyes. Do not introduce another stock UI QVM from upstream.

## Files

- `README.md`: updated project description and credits
- `THIRD_PARTY_NOTICES.md`: upstream projects and license caveats
- `docs/KNOWN_ISSUES.md`: confirmed regressions and workarounds
- `docs/ROADMAP.md`: current prioritized roadmap
- `tools/release_guard.py`: safe build-output check/quarantine
- `tools/install_correction.py`: dry-run first, documented Git checkout merge
- `config/release_default.cfg`, `config/performance_laser.cfg`: updated visible-gun default and high-FPS alternative (legacy v5 profile filenames retained as aliases)
