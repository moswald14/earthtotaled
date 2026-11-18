# Windows host

Program.cs hosts the offline game in WebView2. DesktopMouse.cs registers a
foreground-only raw mouse device and confines input to the active game view.
It leaves legacy clicks enabled, batches movement every 8ms, supports relative
and absolute packets, and releases confinement on deactivation, menus or exit.
The host acknowledges capture through desktop-bridge.js; game.js uses that
bridge only inside this desktop host. Browser pointer lock remains available
for the normal HTML build. Desktop saves keep the existing virtual host/profile.

Build from the workspace root with desktop/build-desktop.ps1. The script embeds
the current outputs/EARTHtotaled.html and desktop-bridge.js, plus SDK DLLs.
The single executable can be copied without external companion DLLs. Keep the
Microsoft license when redistributing it. WebView2 Runtime must be installed.

Restore the SDK to desktop/vendor/webview2 from the official package:
https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.4314.35/microsoft.web.webview2.1.0.4314.35.nupkg

Run node desktop/test-bridge.mjs to check the bridge protocol. The main test.mjs
suite checks desktop mouse input integration and its pause/workbench lifecycle.
