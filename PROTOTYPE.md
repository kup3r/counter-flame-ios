# Flame ESP v3

Menu: double tap with three fingers. ESP starts OFF: enable only after entering a match and disable before the next loading screen. After enabling, sampling waits five seconds. An observed active scene change pauses ESP until manually re-enabled; unchanged-scene loading phases cannot be reliably detected.

V3 requests 60 display-link callbacks per second (30/60 selector). Discovery of PlayerController objects and child SkinnedMeshRenderer objects runs every 0.5 seconds; cached objects are held by GC handles, and their model bounds and projections are refreshed on each display callback. Positions are not interpolated from old snapshots. Actual callback rate and CPU cost require on-device measurement.

Boxes enclose the projected world-space bounds of enabled child skinned meshes. Renderer bounds can be conservative, include equipment, or not follow every animation precisely. An object without available bounds falls back to a moving dot. Boxes crossing the camera near plane are omitted. No team, health, bone, occlusion or dead-player filtering is implemented. Camera.main is still assumed to be the gameplay camera.

IL2CPP API methods are resolved at runtime and cached. Missing required methods pause ESP; managed exceptions pause sampling. This does not catch native crashes. All methods and bounds must still be verified on the phone. No game addresses or field offsets are hard-coded. No Silent Aim or No Recoil.

GitHub Actions compiles an arm64 dylib. Integrating into an IPA and signing are separate; builds consume no Apple App IDs. Never commit the game IPA or signing credentials to this public repository.
