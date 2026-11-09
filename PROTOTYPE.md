# iOS marker prototype

FlameMarkers.mm builds an arm64 dynamic library. Open the menu with two taps using three fingers. Player markers can be toggled in the menu.

This is an experimental in-process Unity/IL2CPP prototype. It has no fixed game offsets. It resolves classes and methods at runtime. No Silent Aim or No Recoil implementation is included. Player root positions are shown; team, health and bounds are not resolved.

Build: Actions → Build unsigned iOS marker prototype → Run workflow. Download FlameMarkers-unsigned after a successful run. The workflow does not consume Apple App IDs, sign or install an app. The dylib still requires integration into an IPA and final signing. Successful compilation does not verify behavior on an iPhone.

Do not upload game IPA archives or signing credentials to this repository.
