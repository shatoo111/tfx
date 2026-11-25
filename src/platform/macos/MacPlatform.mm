#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#include "platform/NativePlatform.h"
#include "core/TelegramClients.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QHash>
#include <QSet>
#include <QWindow>
#include <QXmlStreamWriter>

namespace te {
namespace {
NSString* const nativeClient = @"ru.keepcoder.Telegram"; // The App Store calls it simply Telegram.
bool opensTelegramLinks(NSURL* bundle) {
    if (![bundle.pathExtension isEqualToString:@"app"]) return false;
    NSDictionary* info = [NSBundle bundleWithURL:bundle].infoDictionary;
    NSArray* types = info[@"CFBundleURLTypes"];
    if (![types isKindOfClass:NSArray.class]) return false;
    for (NSDictionary* type in types) {
        NSArray* schemes = [type isKindOfClass:NSDictionary.class] ? type[@"CFBundleURLSchemes"] : nil;
        if ([schemes isKindOfClass:NSArray.class] && [schemes containsObject:@"tg"]) return true;
    }
    return false;
}
// A process is looked at once: reading its bundle is slow. Forgotten when programs start or quit.
QHash<pid_t, bool>& verdicts() { static QHash<pid_t, bool> map; return map; }
bool isTelegram(NSRunningApplication* app) {
    if (!app || app.processIdentifier <= 0) return false;
    auto& known = verdicts();
    if (const auto found = known.constFind(app.processIdentifier); found != known.constEnd()) return *found;
    NSString* bundle = app.bundleIdentifier;
    const bool telegram = bundle && ![bundle hasPrefix:@"com.apple."]
        && (isTelegramBundle(QString::fromNSString(bundle)) || opensTelegramLinks(app.bundleURL));
    known.insert(app.processIdentifier, telegram);
    return telegram;
}
NSWindow* nativeWindow(QWindow& window) {
    auto* view = (__bridge NSView*)reinterpret_cast<void*>(window.winId());
    return view.window;
}
QString launchAgentPath() {
    return QDir::homePath() + QStringLiteral("/Library/LaunchAgents/org.telegrameffects.desktop.plist");
}
class MacPlatform final : public NativePlatform {
public:
    ~MacPlatform() override {
        setRenderingActive(false);
        watchPrimaryPress(0, {});
        for (id observer in observers_) [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:observer];
    }
    WindowSnapshot sample() override {
        @autoreleasepool {
            WindowSnapshot result;
            NSRunningApplication* front = NSWorkspace.sharedWorkspace.frontmostApplication;
            result.active = sessionActive_ && isTelegram(front);
            QSet<pid_t> visibleProcesses, nativeProcesses;
            for (NSRunningApplication* app in telegramApps()) {
                if (app.terminated) { telegram_ = nil; continue; }
                result.running = true;
                if (!app.hidden && (!result.active || app.processIdentifier == front.processIdentifier))
                    visibleProcesses.insert(app.processIdentifier);
                if ([app.bundleIdentifier isEqualToString:nativeClient]) nativeProcesses.insert(app.processIdentifier);
            }
            if (!result.running) { result.detail = QStringLiteral("Ожидание Telegram"); return result; }
            if (!sessionActive_ || visibleProcesses.isEmpty()) {
                result.detail = QStringLiteral("Пауза — окно Telegram недоступно"); return result;
            }
            // Only PID, layer, number, alpha and bounds. No images, window titles or AX tree are read.
            CFArrayRef windows = CGWindowListCopyWindowInfo(
                kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
            if (!windows) { result.detail = QStringLiteral("macOS не вернула геометрию окон"); return result; }
            NSArray* list = CFBridgingRelease(windows);
            NSInteger previousNormal = 0;
            for (NSDictionary* entry in list) {
                const auto pid = [entry[(__bridge NSString*)kCGWindowOwnerPID] intValue];
                const auto layer = [entry[(__bridge NSString*)kCGWindowLayer] intValue];
                if (layer != 0 || [entry[(__bridge NSString*)kCGWindowAlpha] doubleValue] <= 0) continue;
                const NSInteger number = [entry[(__bridge NSString*)kCGWindowNumber] integerValue];
                if (!visibleProcesses.contains(pid)) {
                    previousNormal = number;
                    continue;
                }
                CGRect bounds{};
                if (!CGRectMakeWithDictionaryRepresentation(
                    (__bridge CFDictionaryRef)entry[(__bridge NSString*)kCGWindowBounds], &bounds)) continue;
                // Pick the frontmost normal window; do not fall through to a chat underneath a small dialog.
                // The small windows of the native client are passing hints above the same chat.
                if (bounds.size.width < 180 || bounds.size.height < 120) {
                    if (nativeProcesses.contains(pid)) continue;
                    break;
                }
                result.id = [entry[(__bridge NSString*)kCGWindowNumber] unsignedIntValue];
                result.bounds = QRectF(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
                result.visible = true;
                if (overlay_.visible) {
                    const CGFloat top = NSMaxY(NSScreen.screens.firstObject.frame);
                    const NSRect desired = NSMakeRect(bounds.origin.x, top - CGRectGetMaxY(bounds),
                                                      bounds.size.width, bounds.size.height);
                    result.needsPlacement = previousNormal != overlay_.windowNumber || !NSEqualRects(overlay_.frame, desired);
                }
                break;
            }
            result.detail = result.canOverlay() ? QStringLiteral("Telegram подключён")
                : QStringLiteral("Пауза — окно свёрнуто или находится на другом рабочем столе");
            return result;
        }
    }
    void configureOverlay(QWindow& window) override {
        NSWindow* native = nativeWindow(window);
        overlay_ = native;
        native.opaque = NO;
        native.backgroundColor = NSColor.clearColor;
        native.hasShadow = NO;
        native.ignoresMouseEvents = YES;
        native.hidesOnDeactivate = NO;
        native.level = NSNormalWindowLevel;
        // FullScreenAuxiliary permits Telegram's full-screen Space; never activate or move its Space.
        native.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces
            | NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorIgnoresCycle;
        native.animationBehavior = NSWindowAnimationBehaviorNone;
    }
    void placeOverlay(QWindow& window, const WindowSnapshot& snapshot) override {
        const auto& r = snapshot.bounds;
        // Qt and Quartz both use top-left global points. Cocoa uses bottom-left global points.
        // A single Cocoa frame change lets Qt follow the native screen and backing scale.
        NSWindow* native = nativeWindow(window);
        const CGFloat primaryHeight = NSScreen.screens.firstObject.frame.size.height;
        const NSRect frame = NSMakeRect(r.x(), primaryHeight - r.y() - r.height(), r.width(), r.height());
        if (!NSEqualRects(native.frame, frame)) [native setFrame:frame display:NO];
        if (window.isVisible())
            [native orderWindow:NSWindowAbove relativeTo:static_cast<NSInteger>(snapshot.id)];
    }
    QPointF cursorInOverlay(const WindowSnapshot& snapshot, const QWindow&) const override {
        const NSPoint mouse = NSEvent.mouseLocation;
        const CGFloat height = NSScreen.screens.firstObject.frame.size.height;
        return {mouse.x - snapshot.bounds.x(), height - mouse.y - snapshot.bounds.y()};
    }
    void watchPrimaryPress(quintptr target, std::function<void(QPointF)> pressed) override {
        if (pressMonitor_) {
            [NSEvent removeMonitor:pressMonitor_];
            pressMonitor_ = nil;
        }
        if (!target) return;
        pressMonitor_ = [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
            handler:^(NSEvent* event) {
                if (!sessionActive_ || !overlay_.visible || !event.CGEvent ||
                    !isTelegram(NSWorkspace.sharedWorkspace.frontmostApplication)) return;
                const CGPoint point = CGEventGetLocation(event.CGEvent);
                const NSPoint screen = NSMakePoint(point.x, NSMaxY(NSScreen.screens.firstObject.frame) - point.y);
                if ([NSWindow windowNumberAtPoint:screen belowWindowWithWindowNumber:overlay_.windowNumber]
                    == static_cast<NSInteger>(target)) pressed(QPointF(point.x, point.y));
            }];
    }
    void watch(std::function<void()> changed) override {
        changed_ = std::move(changed);
        observers_ = [NSMutableArray array];
        auto* center = NSWorkspace.sharedWorkspace.notificationCenter;
        NSArray<NSNotificationName>* names = @[
            NSWorkspaceDidActivateApplicationNotification, NSWorkspaceDidDeactivateApplicationNotification,
            NSWorkspaceDidHideApplicationNotification, NSWorkspaceDidUnhideApplicationNotification,
            NSWorkspaceDidTerminateApplicationNotification, NSWorkspaceDidLaunchApplicationNotification,
            NSWorkspaceActiveSpaceDidChangeNotification, NSWorkspaceSessionDidResignActiveNotification,
            NSWorkspaceSessionDidBecomeActiveNotification, NSWorkspaceWillSleepNotification, NSWorkspaceDidWakeNotification];
        for (NSNotificationName name in names) {
            id observer = [center addObserverForName:name object:nil queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification* note) {
                    if ([note.name isEqualToString:NSWorkspaceSessionDidResignActiveNotification]
                        || [note.name isEqualToString:NSWorkspaceWillSleepNotification]) sessionActive_ = false;
                    if ([note.name isEqualToString:NSWorkspaceSessionDidBecomeActiveNotification]
                        || [note.name isEqualToString:NSWorkspaceDidWakeNotification]) sessionActive_ = true;
                    if ([note.name isEqualToString:NSWorkspaceDidLaunchApplicationNotification]
                        || [note.name isEqualToString:NSWorkspaceDidTerminateApplicationNotification]) {
                        telegram_ = nil;
                        if (NSRunningApplication* app = note.userInfo[NSWorkspaceApplicationKey]) verdicts().remove(app.processIdentifier);
                    }
                    if (changed_) changed_();
                }];
            [observers_ addObject:observer];
        }
    }
    bool pointerPressed() const override { return NSEvent.pressedMouseButtons != 0; }
    bool layerOccluded() const override {
        return overlay_.visible && !(overlay_.occlusionState & NSWindowOcclusionStateVisible);
    }
    void setRenderingActive(bool active) override {
        if (active == (renderingActivity_ != nil)) return;
        if (active) {
            // Without this App Nap may throttle timers once TFX has no visible normal window.
            renderingActivity_ = [NSProcessInfo.processInfo
                beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
                reason:@"Animating the Telegram overlay"];
        } else {
            [NSProcessInfo.processInfo endActivity:renderingActivity_];
            renderingActivity_ = nil;
        }
    }
    bool reduceMotion() const override { return NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceMotion; }
    bool autoStartEnabled() const override { return QFile::exists(launchAgentPath()); }
    bool setAutoStart(bool enabled, QString& error) override {
        const QString path = launchAgentPath();
        if (!enabled) {
            if (!QFile::exists(path) || QFile::remove(path)) return true;
            error = QStringLiteral("Не удалось удалить пользовательский LaunchAgent."); return false;
        }
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
            error = QStringLiteral("Не удалось создать каталог LaunchAgents."); return false;
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
        QXmlStreamWriter xml(&file);
        xml.setAutoFormatting(true); xml.writeStartDocument();
        xml.writeDTD(QStringLiteral("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"));
        xml.writeStartElement(QStringLiteral("plist")); xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
        xml.writeStartElement(QStringLiteral("dict"));
        xml.writeTextElement(QStringLiteral("key"), QStringLiteral("Label"));
        xml.writeTextElement(QStringLiteral("string"), QStringLiteral("org.telegrameffects.desktop"));
        xml.writeTextElement(QStringLiteral("key"), QStringLiteral("ProgramArguments"));
        xml.writeStartElement(QStringLiteral("array"));
        xml.writeTextElement(QStringLiteral("string"), QCoreApplication::applicationFilePath());
        xml.writeTextElement(QStringLiteral("string"), QStringLiteral("--background")); xml.writeEndElement();
        xml.writeTextElement(QStringLiteral("key"), QStringLiteral("RunAtLoad")); xml.writeEmptyElement(QStringLiteral("true"));
        xml.writeTextElement(QStringLiteral("key"), QStringLiteral("ProcessType"));
        xml.writeTextElement(QStringLiteral("string"), QStringLiteral("Interactive"));
        xml.writeEndElement(); xml.writeEndElement(); xml.writeEndDocument();
        if (!file.commit()) { error = file.errorString(); return false; }
        return true; // launchd reads this per-user agent on next login; no elevated helper needed.
    }
private:
    // Asking every process for its bundle is slow. The list changes only on launch and exit,
    // both announced by notifications; the age limit covers a missed one.
    NSArray<NSRunningApplication*>* telegramApps() {
        const CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        if (telegram_ && now - listed_ < 2) return telegram_;
        listed_ = now;
        NSMutableArray* found = [NSMutableArray array];
        QSet<pid_t> alive;
        for (NSRunningApplication* app in NSWorkspace.sharedWorkspace.runningApplications) {
            alive.insert(app.processIdentifier);
            if (isTelegram(app)) [found addObject:app];
        }
        verdicts().removeIf([&alive](const auto& entry) { return !alive.contains(entry.key()); });
        return telegram_ = found;
    }
    NSArray<NSRunningApplication*>* telegram_ = nil;
    CFAbsoluteTime listed_ = 0;
    std::function<void()> changed_;
    id pressMonitor_ = nil;
    __weak NSWindow* overlay_ = nil;
    NSMutableArray* observers_ = nil;
    id<NSObject> renderingActivity_ = nil;
    bool sessionActive_ = true;
};
}
std::unique_ptr<NativePlatform> createNativePlatform() { return std::make_unique<MacPlatform>(); }
}
