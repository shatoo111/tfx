# Automatic effect startup and recovery — 0.3.3

The startup regression was reproduced on macOS: configure a saved Sakura theme before the first Telegram window sample, then start the tracker. Previously the overlay never became visible. `SakuraEffect::needsAnimation()` correctly returns false at zero dimensions, but OverlayController queried it before placing/resizing the window, so it never initialized those dimensions. Toggling settings after another effect/window had supplied dimensions could hide the bug.

The controller discovers an enabled effect before the simulation has a renderable size. It applies Telegram's geometry and uses the overlay window's logical size before starting animation. WindowTracker emits a sample-completed signal even if the target snapshot did not change. This lets the controller recover an independently hidden overlay or stopped animation timer without a settings toggle. While enabled, the tracker polls every 100 ms when it has no usable target. The effect stays visible over Telegram in the background and pauses when Telegram is minimized or unavailable. A one-second missing-frame watchdog re-shows the layer and requests rendering while preserving the simulation.

Sakura seeds a few petals along the top edge on its first simulation frame, including when branches are disabled. Later petals continue to enter from above the top edge.

## Verification

Before the fix, the native Metal regression test failed to show Sakura on cold start. The tests cover all three saved themes with an inactive Telegram window already open or appearing later without native notifications, visibility changes, and recovery from independent overlay hiding while the Telegram snapshot stays unchanged. The core Sakura test also checks first-frame petals with branches disabled.

The native integration tests use a target-window adapter; they do not read Telegram chats or capture its interface. Windows builds and headless tests run in GitHub Actions. Manual integration should check startup in either order, hiding settings, minimize/restore, virtual desktops and sleep/wake on both systems.

Qt documents that hiding a QQuickWindow can release graphics/scene graph resources: https://doc.qt.io/qt-6.8/qquickwindow.html
