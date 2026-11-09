#!/bin/bash
set -euo pipefail
mkdir -p build/MenuPreview.app
xcrun --sdk iphonesimulator clang++ -target "$(uname -m)-apple-ios15.0-simulator" -isysroot "$(xcrun --sdk iphonesimulator --show-sdk-path)" -std=c++17 -fobjc-arc tests/MenuPreview.mm -framework UIKit -framework Foundation -framework CoreGraphics -o build/MenuPreview.app/MenuPreview
cat > build/MenuPreview.app/Info.plist <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleIdentifier</key><string>com.example.flame.preview</string>
<key>CFBundleExecutable</key><string>MenuPreview</string>
<key>CFBundleName</key><string>FlamePreview</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleVersion</key><string>1</string>
<key>CFBundleShortVersionString</key><string>1.0</string>
<key>MinimumOSVersion</key><string>15.0</string>
<key>UIDeviceFamily</key><array><integer>1</integer></array>
<key>UISupportedInterfaceOrientations</key><array><string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string></array>
<key>UILaunchScreen</key><dict/>
</dict></plist>
PLIST
device=$(xcrun simctl list devices available -j | python3 -c 'import sys,json; d=json.load(sys.stdin); a=[x["udid"] for v in d["devices"].values() for x in v if "iPhone" in x["name"]]; print(a[0] if a else "")')
test -n "$device"
xcrun simctl boot "$device" || true
xcrun simctl bootstatus "$device" -b
xcrun simctl install "$device" build/MenuPreview.app
for entry in '0:visual' '3:skins'; do
  tab=${entry%%:*}
  name=${entry##*:}
  xcrun simctl terminate "$device" com.example.flame.preview || true
  xcrun simctl launch "$device" com.example.flame.preview -FlamePreviewTab "$tab"
  container=$(xcrun simctl get_app_container "$device" com.example.flame.preview data)
  for attempt in $(seq 1 15); do
    if test -f "$container/tmp/Flame-menu.png"; then break; fi
    sleep 2
  done
  cp "$container/tmp/Flame-menu.png" "build/menu-$name.png"
  rm -f "$container/tmp/Flame-menu.png"
done
xcrun simctl shutdown "$device"
