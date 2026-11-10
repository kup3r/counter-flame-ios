# Flame v12

A native arm64 in-process module for the supplied CounterFlame 1.4.1 build 7 IPA. No fixed game addresses or hard-coded field offsets. Menu and additional gameplay controls need on-device testing.

## Menu

Sand/cream panel with a gray sidebar, Visual / Aim / Misc / Skins / Config tabs, draggable title bar, close button and miniature ESP preview. Open using two taps with three fingers. Configuration saves locally under a namespaced UserDefaults key; loading never enables the module automatically.

Visual controls: enemy-only filter, corner or normal boxes, fill, skeleton, head markers, HP text, names, world-unit distance, tracers, line thickness, color, distance limit, crosshair and actual ESP callback FPS. The ESP preview is illustrative. The visual tab also offers a box glow, edge vignette and center dot.

## Gameplay controls: experimental and initially off

- AIM: previous local camera steering while holding fire; radius and smoothing controls. The Aim tab adds a fire delay, turn-rate limit, center dead zone, distance limit and optional radius display. No visibility check. Silent Aim remains unimplemented.
- No Recoil: previous named local recoil/camera parameters.
- No Spread: named local accuracy/additive/air penalty, live RecoilData dispersion, pellet spread and AirInaccuracy vectors. Reads a type-checked scalar or Vector2 magnitude from NextShotSpread before and after local changes when available. Missing diagnostics no longer revert the field changes. Client trajectory and server spread still require match testing.
- SpeedHack: scales the captured PlayerMovement._speed and the actual SourceMovementTuning.MaxSpeed cap without compounding. The menu reports horizontal speed and the active player cap. Range 1–3x. The server may clamp or correct travel.
- Unlimited Ammo: replenishes the counters in ArmsController and updates the held controller’s separate Ammunition instance from WeaponParameters capacity. The old getter produced a detached copy. Refuses to write if the live/config objects are the same or capacities are invalid. Does not grant account inventory or guarantee server-side unlimited shots.
- Fixed camera FOV: 60–120 degrees; restores the last game-provided FOV when disabled.

Saved float/vector modifications have separate recoil/spread/movement groups. Disabling one does not undo another. Restoration only writes when the current value still matches this module's last applied value. Native faults remain outside managed exception handling.

## SkinChanger

SkinCatalog.json is generated from the supplied WeaponId metadata constants and bundled Addressables paths: 52 weapon/item entries, 330 unique material keys. Some entries, such as grenades, have no skin materials. Legacy naming aliases map catalog prefixes (for example DesertEagle → deagle, M4A1 → m4a4); live weapon IDs are checked before material calls.

Search supports weapon and finish names; category tabs narrow the list. Applying a material requires that exact selected weapon ID in hand. It does not replace AK gameplay with AWP parameters. Knife-model replacement is separate and works from any knife held by the living local player, without a CT-only restriction. Requested model configurations must report the selected ID before the normal game CreateWeapon routine is invoked. The seven-argument signature is checked at runtime. A selected knife material goes to CreateWeapon with the model. Both weapon paths now construct a disabled StatTrak instance because the iOS build dereferences it. The menu shows the failed Unity method and exception for a rejected request. If the equipped-key readback differs, the module can try SetSkin on the equipped knife. Normal game routines may synchronize appearance; the module does not grant persistent ownership.

The requested knife and skin keys are kept in memory for the game session and reapplied when a new local character equips the appropriate weapon. Death and spectating release character-specific state while leaving the master switch enabled within the same scene. The Skins tab shows 330 browsable, searchable inventory cards with the finish name below its image. 329 thumbnails are extracted from the original 1.4.1 build 7 sprites. MAC-10 Neon Rider uses the matching Steam inventory image because the shipped resources have no matching sprite. Browsing the cards works with the module switched off. Material requests and delayed equipped-key readback distinguish a request from confirmation. Original nonempty material keys can be restored. For an original standard material, the module requests the original weapon configuration and preserves separately detected live ammunition counters. Restore/material results still require device confirmation. Model/animation/material correctness is not proven by compilation.

## v9 scene and input options

- AIM selects the head, chest, pelvis, or the available joint closest to the crosshair. Body joints are matched by names in the live skeleton.
- Visual offers a fading local trail, skybox tint on supported camera skybox materials with a solid camera background fallback, a time-of-day lighting slider using MapColorGrading, fixed/RGB local player tint, and a button to cycle to another loaded agent from the same team.
- Misc adds a local third-person display camera and a continuous yaw control. The display camera leaves the game's first-person camera transform in place. Its rear offset is limited by a linecast. Third-person display suspends automatic targeting while active. The spin changes the local visual skeleton yaw after checking that the camera is outside its transform hierarchy.
- TriggerBot traces with the weapon's game layer mask and requires the first hit to resolve to a living opponent. It uses ArmsController.Shoot after a configurable dwell and checks the game's own shooting predicates. Bunnyhop uses the game's held-jump behavior and restores the two local settings when disabled.
- The sticker catalog lists 116 kits read from the game bundle. A selected slot is sent to WeaponController.ShowStickers for the held weapon, using the game’s StickerCodec to construct the managed StickerData list while preserving entries in other slots. This is a local appearance request.

Native gameplay paths remain subject to iPhone testing. Opponent-facing Anti Aim is absent. No game binary or metadata dump is committed.

## v11 gloves and solo practice

- Skins → Перчатки lists 33 gloves from the shipped iOS asset definitions and sprites. The local held weapon's GlovesController resolves the selected mesh and loads the existing material. The session selection follows weapon swaps and respawns. Reset requests the original inventory appearance.
- Misc → Раунд includes AutoWin, Plant Anywhere and Defuse Anywhere. They require a one-participant room with local host authority. The room must be offline, or owned by the local player with the game's private-room cheats enabled. Plant Anywhere requires a living TR with C4 and a floor hit. Defuse Anywhere requires a living CT and an active bomb. AutoWin requests a stock round-end phase once per active round for the local side.
- All game actions still require an iPhone match check. The archive remains a single dylib.

## v12 grenade effects, movement and lineups

- Anti Flash disables the local flash Canvas. Anti Smoke changes supported opacity/color/depth properties of the smoke materials. Both restore captured render values when disabled.
- Auto-Strafe uses the shipped SourceMove.AirAccelerate routine while airborne. Edge Jump checks ground ahead and requests the normal jump event. Unlimited Duck adjusts the local duck speed multiplier and timing values. These experimental client changes require device tests and may be corrected by the server.
- Grenade Helper stores up to 24 player-made lineups per scene, with position and aim markers. The optional automatic positioning and alignment use the solo practice gate.
- Pitch and yaw controls apply to the local visual skeleton with Spin, Backward and Jitter modes. Fake Lag uses Photon outgoing delay (30–250 ms) in a locally owned, private online room with game cheats enabled.
- WallShot is a conservative trigger mode for a single thin penetrable surface. It checks the game’s material rules, exit trace, and estimated penetration loss, then requests an ordinary shot only if the next hit is a living enemy.
- Blockbot, network Desync/Fake Duck and Air Stuck/Edge Bug are not present.

## Validation

CI runs portable grouped-restoration/speed-scaling tests with address/undefined-behavior sanitizers, builds the arm64 dylib and checks its architecture/dependencies. A separate optional job renders the actual UIKit menu in an iOS simulator. Simulator rendering exercises UI/catalog/property-list structure, not real game functions. Device performance and server acceptance remain unverified.

Enable the module after match loading and disable before the next loading screen. Active-scene changes pause it, but not every loading phase changes scenes. No game IPA or signing secrets are committed; builds consume no Apple App IDs.
