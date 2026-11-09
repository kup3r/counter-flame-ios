# Flame ESP v5

Open menu: double tap with three fingers. ESP starts OFF. Enable after entering the match; disable before another loading screen. Active-scene changes and managed exceptions pause the module. Not every loading phase changes active scenes, so manual disabling remains required.

## Features

- Tight 2D joint-envelope boxes replace broad world-space renderer animation bounds. Padding covers skin around joints. These are not exact visible-mesh silhouettes.
- Skeleton lines follow actual Transform parent relationships in SkinnedMeshRenderer.bones, bridging excluded intermediate parents. No fixed bone indices are used. A name-based heuristic omits finger/toe/twist/helper/weapon chains for clarity; unfamiliar bone names may remain.
- Enemy-only switch defaults ON. Photon.Realtime.Player.get_Team is compared with the local player's team. During active spectating, SpectatorManager.CurrentPlayer provides the reference team instead. Unknown teams are excluded in enemy-only mode and counted in diagnostics. Local and watched actors are excluded.
- Dead PlayerController objects are excluded using get_IsDeath.
- Separate switches for boxes, skeleton, raw HP text, names, distance, tracers, and diagnostics.
- Distances use Unity world units (u); the meter scale has not been verified.
- 30/60 display-link target frequency; actual device rate/performance require testing. Object and rig discovery run at 2 Hz; cached joints are projected every display callback.

## Confirmed inputs

The supplied metadata v31 contains zero-argument getters for PlayerController.get_Player/get_Health/get_IsDeath, PhotonNetwork.get_LocalPlayer, Photon.Realtime.Player.get_Team/get_ActorNumber/get_NickName, SpectatorManager.get_IsSpectating/get_CurrentPlayer, SkinnedMeshRenderer.get_bones and Transform.get_parent. The established animator/current skeleton/body getter path remains in use. All 22 IL2CPP API symbols used by this source exist in the supplied UnityFramework symbol table. This verifies availability, not runtime semantics or safety.

## Limits

Bone discovery, enemy filtering, spectator behavior and on-device load still need runtime validation. Culling can leave animation bones stale; this module does not modify renderer or animator state. Missing usable rigs fall back to small dots. No visibility test, aim modifications or recoil modifications. Camera.main remains an assumption. Native crashes are not caught.

Builds do not consume Apple App IDs. Do not commit game IPA archives or signing credentials to this public repository.
