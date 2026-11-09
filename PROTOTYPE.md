# Flame ESP v4

Menu: double tap with three fingers. Enable ESP only after match loading; disable before the next loading screen. Target update rate: selectable 30/60 Hz. Player discovery: every 0.5 seconds; positions and bounds: each display callback.

V3's body discovery was broken for this binary. Parsing the supplied metadata v31 shows that UnityEngine.Component has only generic GetComponentsInChildren overloads; the non-generic (System.Type) overload used by v3 is absent. V3 treated this as an optional feature and silently fell back to dots.

V4 uses the exact getters present in the supplied metadata: PlayerController.get_PlayerAnimatorController → ScarletWork.Photon.TPS.PlayerAnimatorController.get_CurrentSkeleton → ScarletWork.Game.PlayerSkeleton.get_Body. get_BodyLod is used only if Body is unavailable. Renderer.get_bounds and Bounds.get_min/get_max provide the box, without fixed offsets or invented character dimensions.

Disabled renderers are counted but no longer excluded from bounds reads. No renderer state is changed. This matters because the game includes model visibility/occlusion logic. Whether disabled renderer bounds stay current on the phone still needs verification.

On-screen diagnostics show body count, missing animator/skeleton/body counts, disabled renderers, empty bounds and boxes rejected near the camera. Dots remain an explicit fallback for unavailable bounds. A successful compilation does not prove the new getters work on the device. Native crashes are not caught.

No team, health, bone, visibility or dead-player filtering; no Silent Aim or No Recoil. Camera.main is still assumed to be the gameplay camera. Game archives and signing credentials must not be committed to this public repository. Builds do not consume Apple App IDs.
