# PSVR2 Sense integration on FW 7.51: engineering acceptance criteria

**NOT A WORKING DRIVER.** The bridge in `native/` is a thread-constrained interface that can accept valid controller samples from a future platform implementation. It has no native PS5 API calls and must **not** be advertised as controller support in a release.

## Known facts and missing evidence

The firmware 7.51 research previously found `sceVrTracker2Initialize`, `Finalize`, `GetResult`, `RegisterDevice`, `UnregisterDevice` and other exports from `libSceVrTracker2.sprx`. The firmware's `libSceVrHand.sprx` was not found. A different open-source native PS5VR project documents Sense pointers on **13.60**, but its offsets and structs cannot safely be transplanted to 7.51. Sony's hardware supports the controllers; this does not prove access from our homebrew executable.

Before writing native tracking calls:

1. Verify which 7.51 module exports return **controller pose**, button state and device registration. Reuse 7.51 firmware-specific exports and check anchor/version before invoking any guessed offset. Do not dereference an arbitrary pointer or call a function using a 13.60 signature.
2. Establish **typed ABI** by inspecting 7.51 `GetResult` call sites and observing harmless result metadata: structure sizes, return codes, device indexes, timestamp units, validity bits and coordinate orientation. Use bounded probes with canary buffers; do not assume a controller record fits the HMD record.
3. Log each hand's **connected / pose-valid / position XYZ / quaternion / timestamp / trigger / grip / thumbstick**, at a low rate. Verify left/right identification, HMD-relative tracking, recenter behavior and disconnect handling.
4. Implement a **platform adapter** that transforms tracker-space coordinates to a documented common reference. Call `q3psvr2_sense_publish` on the engine's game/input thread (the provided bridge is not thread-safe). Reject invalid or stale samples rather than keeping a frozen gun in space.
5. Convert controller pose to the **same world space** used by cgame's trace and the stereo world renderer. Test roll/pitch/yaw signs and units (Quake units vs meters) and muzzle offsets for each gun.
6. Change the **actual game/server firing direction**, not just the rendered weapon. In Quake III, server-side hit registration normally derives from predicted/networked player view angles. A controller-independent aim direction needs an authoritative network/gameplay contract; keep standard servers and DualSense compatibility.
7. Use the same ray for controller-positioned weapon, hit reticle and optional laser. Avoid a second QVM trace each eye, and measure FPS during heavy combat.
8. Only after input poses work, add per-hand rumble/haptic calls and grip/trigger mappings. Differentiate basic pad rumble from Sense-specific haptics/trigger resistance.

## Current bridge contract

`q3psvr2_sense_publish(hand, sample)` stores one sample; `q3psvr2_sense_get_pose(hand,now,max_age,out)` yields a fresh pose only and otherwise returns false with `out` zeroed, allowing the existing DualSense/gameplay-aim fallback. Coordinates are not converted inside the bridge. **A fake or stale controller pose must never silently override gameplay aim.**

## Milestone test matrix

- [ ] Firmware export/ABI validated on an actual PS5 FW 7.51.
- [ ] Both hands produce valid, independent 6DoF samples and buttons.
- [ ] Room-scale HMD motion and weapon motion remain in the same reference frame.
- [ ] Controller model/weapon stereo-fuses, doesn't clip, and tracks hand in real time.
- [ ] Shots and `CG_Trace` endpoint agree at near/far targets and against bots.
- [ ] Optional laser and 3D reticle share the same authoritative ray.
- [ ] Disconnect/reconnect, low battery, recenter and paused menus fall back safely.
- [ ] Combat FPS acceptable and haptics independently switchable.

Only mark **Sense tracking supported** once these pass on hardware.
