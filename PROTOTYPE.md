# Flame v6 — experimental controls

ESP v5 is retained. Menu: two taps with three fingers. Enable the module after match loading; disable before the next loading screen. New AIM and No Recoil switches default OFF. These additions have not been tested on the phone.

## Knife and skins

Take a knife in hand as a living local player, then use Knife → Butterfly (ID 8). The supplied metadata's byte enum constants are KnifeCT=7 and Butterfly=8. StatTrak is a reference class, not a value type. Butterfly material assets are present in the supplied catalog.

The module searches loaded WeaponParameters for an actual enum ID 8, with a fallback to ResourceUtility.LoadWeaponParameters("Butterfly"). A result with any other ID is rejected. It invokes the game's complete CreateWeapon routine rather than overwriting the ID on a default knife. All seven parameter positions are checked at runtime before invocation: WeaponParameters, String, StatTrak, Boolean, a non-value/non-by-reference sticker collection reference, Int32, Int32. The optional sticker argument is null; its specific collection type is not assumed. The original parameters and arms instance are retained for the restore button. A delayed read displays the observed current ID. An accepted call alone is not confirmation that the model and animation changed correctly.

Vanilla/Fade buttons call the confirmed two-argument SetSkin overload only while current WeaponId is 8. Material keys are based on the actual catalog's butterfly_vanilla and butterfly_fade folders. Material application and animation correctness still require device testing. These methods may perform normal game appearance synchronization; their native implementations have not been fully traced. No persistent inventory grant or ownership change is implemented.

## Ordinary AIM (experimental)

While the real local shoot button is pressed, selects an identified enemy's confirmed head Transform within a configurable screen radius. Adjusts local PlayerLock pitch and body yaw with smoothing. Checks field types, float finiteness, pitch/camera convention and look limit before writing. Disabled while observing, dead, or with menu open. No line-of-sight test is implemented, so occluded targets can be selected. This is visible camera steering, not Silent Aim.

## No Recoil (experimental)

Zeroes the local weapon RecoilParameters' horizontal/vertical ranges, camera deviation and view kick through named IL2CPP fields whose types are checked as System.Single. Also clears selected transient local arms/view kick values. Configuration values are saved and restored when disabled, paused, backgrounded or observing, provided the value is still zero. It does not remove unrelated random spread or guarantee server acceptance.

## Silent Aim status

Not implemented. Confirmed method names such as ArmsController.FireRay are not proof of a safe executable interception point or of server-side acceptance. No placeholder toggle is exposed.

## Verification and limits

All additional API symbols and game member names used here were checked against the supplied binary/metadata. No fixed addresses or guessed field offsets. GitHub compilation verifies the arm64 build, not runtime correctness. Managed exceptions in experimental controls disable those controls while retaining ESP; native faults cannot be caught this way. Revert to v5 if testing regresses behavior.

No IPA or signing credentials are stored in this repository. Builds do not consume Apple App IDs.
