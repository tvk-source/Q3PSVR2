# Quake III PSVR2 release gates

## Identity and licensing
- [ ] Unique, available title ID chosen; candidate `PPSA99431` checked against installed apps.
- [ ] `sce_sys/param.json` titleId/contentId/titleName consistent with the built app directory.
- [ ] Launcher icon and background identify Quake III PSVR2 (only distribute assets with permission).
- [ ] Copyright, GPL, renderer licenses and source offer included; no retail Quake assets or decrypted Sony firmware in the repository/archive.
- [ ] Clean checkout builds on documented Ubuntu SDK/toolchain environment.

## Native runtime and game
- [ ] Both eyes show gameplay, 3D convergence correct and no screenshot-dependent issues.
- [ ] DualSense navigation, movement, firing, pause and weapon switching still work.
- [ ] Reticle QVM loads from custom PK3 and points toward actual hits.
- [ ] Compact HUD readable in both eyes, including bot chats and scoreboard.
- [ ] Menus, logo, cinematics, pause menu and console fit in the virtual screen.
- [ ] Try combat scene with weapon ON/OFF and quantify FPS, stutter and comfort.
- [ ] If the gun remains expensive, ship the safer `cg_drawGun 0` default.
- [ ] Laser sight validated stereo and on actual bullet impacts, if included/enabled.
- [ ] Sense support absent from release claim unless the 7.51 validation matrix passes.

## Package and rollback
- [ ] Preserve prior `/data/homebrew/PPSA99303` installation and its `autoexec.cfg`.
- [ ] New title path `/data/homebrew/PPSA99431` launches without conflict.
- [ ] Runtime dependencies and custom QVM/reticle are included in the built package.
- [ ] Original Quake data supplied separately by each user; nothing proprietary bundled.
- [ ] Readable install, known-bugs and recovery steps in README.
- [ ] ZIP hash verified; install/exit/restart/headset reconnect tested end-to-end.
