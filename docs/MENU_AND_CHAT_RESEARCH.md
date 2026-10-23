# Menu logo and bot-chat diagnosis from other Quake VR ports

## Main-menu logo

Quake III base UI: `code/q3_ui/ui_menu.c`, `Main_MenuDraw`:

- A **separate 3D model** `models/mapobjects/banner/banner5.md3`, *not* a normal `UI_DrawPic` overlay.
- A `refdef` viewport at `x=0,y=0,w=640,h=120` transformed with `UI_AdjustFrom640`.
- Stock id Software model origin has `origin[2] = -32`; the current PCVR reference (RippeR37/q3vr `code/q3_ui/ui_menu.c`) uses `origin[2] = -128`, models axes 1.6× and 6×, and keeps its own viewport. This is a custom PCVR menu appearance, **not proof of the correct values** for the PS5 asymmetric eye/panel projection.
- `ps5vr_uiScale` changes the 2D viewport, not necessarily the position/cropping of this separate 3D banner. Therefore a cgame-only or generic HUD scale patch cannot fix it reliably.

The optional `patch_menu_banner.py` adds two independent diagnostics to **UI QVM** source:

```cfg
seta ps5vr_menuBannerViewportY "0" // Original viewport location
seta ps5vr_menuBannerModelDrop "0" // Original model Z
```

For testing (only after building and explicitly installing experimental `ui.qvm`):

```cfg
seta ps5vr_menuBannerViewportY "12"
seta ps5vr_menuBannerModelDrop "24"
```

Compare one setting at a time with the source patch default 0, inspect both eye views. Failure to render or UI incompatibility requires immediate rollback by removing test UI PK3. Preserve the currently working original UI QVM.

## Bot chat versus native notify text

There are **at least two independent drawing paths**, and the symptoms need distinguishing:

1. `code/cgame/cg_draw.c` `CG_DrawTeamInfo` renders team-chat rows near `CHATLOC_X=0, CHATLOC_Y=420` in 640×480 logical coordinates; those come from `CG_AddToTeamChat` in `cg_servercmds.c`. The cgame QVM can change this path.
2. The console notification overlay (bot/client `print`/chat lines at top left) is rendered from native client console code, usually `Con_DrawNotify` and projection `RB_SetGL2D`. Patching a cgame QVM **does not fix** this path.

Current PS5 headset reports should be tested separately: trigger a bot `say` message versus team chat; check console notify, scoreboard and compact HUD when the VR panel is active. Run `tools/audit_ppm_and_chat.py` read-only to find the native paths in your checkout. Aim for shared head-locked/world-panel clipping rules with independent anchors; don't just offset the whole HUD, which moves gameplay health/ammo as well.

Reference upstream:
- https://github.com/id-Software/Quake-III-Arena/blob/master/code/q3_ui/ui_menu.c
- https://github.com/RippeR37/q3vr/blob/master/code/q3_ui/ui_menu.c
- https://github.com/RippeR37/q3vr/blob/master/code/vr/vr_virtual_screen.c
- https://github.com/RippeR37/q3vr/blob/master/code/cgame/cg_draw.c
- https://github.com/id-Software/Quake-III-Arena/blob/master/code/cgame/cg_servercmds.c
