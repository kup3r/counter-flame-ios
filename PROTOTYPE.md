# iOS marker prototype — diagnostic v2

Open the menu with two taps using three fingers. Player markers start OFF. Enter a match with them off, then turn them on after loading. Turn them off again before leaving or entering another match.

The user confirmed that v1 crashes during match loading with markers enabled, but works when enabled only after entering the match. A crash report has not been inspected, so the exact failing call is unknown.

V2 retains managed return values with GC handles during each sampling pass, stops calls after a managed exception, verifies thread attachment, checks the active scene's loaded state and handle, waits five seconds after enabling or a detected transition, rejects destroyed player objects and non-finite projected coordinates, and reduces sampling to twice a second. Scene checks are an extra precaution: they cannot detect every loading phase, especially when the active scene is unchanged. The explicit off-during-loading workaround remains required until tested further.

This is an experimental Unity/IL2CPP library for execution inside the game process. No fixed game offsets, Silent Aim, No Recoil, team filtering, health or bounds are implemented.

Build: Actions → Build unsigned iOS marker prototype. A successful workflow produces an arm64 dylib, not an installed or signed app. Compilation does not verify runtime safety on an iPhone. Building does not consume Apple App IDs. Do not upload game IPA archives or signing credentials to this public repository.
