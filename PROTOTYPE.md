# Flame v8

A native arm64 in-process module for the supplied CounterFlame 1.4.1 build 7 IPA. No fixed game addresses or hard-coded field offsets. Menu and additional gameplay controls need on-device testing.

## Menu

Sand/cream panel with a gray sidebar, Visual / Aim / Misc / Skins / Config tabs, draggable title bar, close button and miniature ESP preview. Open using two taps with three fingers. Configuration saves locally under a namespaced UserDefaults key; loading never enables the module automatically.

Visual controls: enemy-only filter, corner or normal boxes, fill, skeleton, head markers, HP text, names, world-unit distance, tracers, line thickness, color, distance limit, crosshair and actual ESP callback FPS. The ESP preview is illustrative. The visual tab also offers a box glow, edge vignette and center dot.

## Gameplay controls: experimental and initially off

- AIM: previous local camera steering while holding fire; radius and smoothing controls. The Aim tab adds a fire delay, turn-rate limit, center dead zone, distance limit and optional radius display. No visibility check. Silent Aim remains unimplemented.
- No Recoil: previous named local recoil/camera parameters.
- No Spread: named local accuracy/additive/air penalty, live RecoilData dispersion, pellet spread and AirInaccuracy vectors. Reads a type-checked scalar or Vector2 magnitude from NextShotSpread before and after local changes when available. Missing diagnostics no longer revert the field changes. Client trajectory and server spread still require match testing.
- SpeedHack: scales PlayerMovement._speed from the captured game base, not from the already multiplied value. Range 1–3x. The server may clamp or correct travel.
- Unlimited Ammo: replenishes a separate live Ammunition instance from actual WeaponParameters capacity through Ammunition.Set(Int32, Int32). Refuses to write if the live/config objects are the same or capacities are invalid. Does not grant account inventory or guarantee server-side unlimited shots.
- Fixed camera FOV: 60–120 degrees; restores the last game-provided FOV when disabled.

Saved float/vector modifications have separate recoil/spread/movement groups. Disabling one does not undo another. Restoration only writes when the current value still matches this module's last applied value. Native faults remain outside managed exception handling.

## SkinChanger

SkinCatalog.json is generated from the supplied WeaponId metadata constants and bundled Addressables paths: 52 weapon/item entries, 330 unique material keys. Some entries, such as grenades, have no skin materials. Legacy naming aliases map catalog prefixes (for example DesertEagle → deagle, M4A1 → m4a4); live weapon IDs are checked before material calls.

Search supports weapon and finish names; category tabs narrow the list. Applying a material requires that exact selected weapon ID in hand. It does not replace AK gameplay with AWP parameters. Knife-model replacement is separate and works from any knife held by the living local player, without a CT-only restriction. Requested model configurations must report the selected ID before the normal game CreateWeapon routine is invoked. The seven-argument signature is checked at runtime. A selected knife material goes to CreateWeapon with the model. If the equipped-key readback differs, the module can try SetSkin on the equipped knife. Normal game routines may synchronize appearance; the module does not grant persistent ownership.

The requested knife and skin keys are kept in memory for the game session and reapplied when a new local character equips the appropriate weapon. Death and spectating release character-specific state while leaving the master switch enabled within the same scene. Preview temporarily requests a material on the matching held weapon for six seconds, with restoration attempted on the original weapon instance. Material requests and delayed equipped-key readback distinguish a request from confirmation. Original nonempty material keys can be restored. For an original standard material, the module requests the original weapon configuration and preserves separately detected live ammunition counters. Restore/material results still require device confirmation. Model/animation/material correctness is not proven by compilation.

## Validation

CI runs portable grouped-restoration/speed-scaling tests with address/undefined-behavior sanitizers, builds the arm64 dylib and checks its architecture/dependencies. A separate optional job renders the actual UIKit menu in an iOS simulator. Simulator rendering exercises UI/catalog/property-list structure, not real game functions. Device performance and server acceptance remain unverified.

Enable the module after match loading and disable before the next loading screen. Active-scene changes pause it, but not every loading phase changes scenes. No game IPA or signing secrets are committed; builds consume no Apple App IDs.
