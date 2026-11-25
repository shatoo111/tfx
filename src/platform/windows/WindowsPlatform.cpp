#include "platform/NativePlatform.h"
#include "core/TelegramClients.h"
#include <windows.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <shobjidl.h>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSettings>
#include <QWindow>
#include <vector>

namespace te {
namespace {
QString processImage(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return {};
    wchar_t path[32768]; DWORD length = 32768;
    const bool ok = QueryFullProcessImageNameW(process, 0, path, &length);
    CloseHandle(process);
    return ok ? QString::fromWCharArray(path, int(length)) : QString();
}
DWORD windowProcess(HWND window) {
    DWORD pid = 0;
    if (!window) return pid;
    // Unigram is a store app: its frame belongs to the system and the content window inside to the app.
    wchar_t name[64];
    if (GetClassNameW(window, name, 64) && wcscmp(name, L"ApplicationFrameWindow") == 0)
        if (const HWND content = FindWindowExW(window, nullptr, L"Windows.UI.Core.CoreWindow", nullptr)) window = content;
    GetWindowThreadProcessId(window, &pid);
    return pid;
}
// A remake that is not in the list is still known as the program opening tg:// links.
QString linkHandler() {
    static QString name;
    static QElapsedTimer age;
    if (!age.isValid() || age.elapsed() > 60000) {
        age.start();
        name = telegramLinkHandler(QSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\tg\\shell\\open\\command"),
            QSettings::NativeFormat).value(QStringLiteral(".")).toString());
    }
    return name;
}
bool telegramName(const QString& name) {
    return isTelegramExecutable(name) || (!name.isEmpty() && name.compare(linkHandler(), Qt::CaseInsensitive) == 0);
}
bool telegram(DWORD pid) { return telegramName(QFileInfo(processImage(pid)).fileName()); }
bool telegramRunning() {
    HANDLE processes = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processes == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(processes, &entry)) {
        do {
            found = telegramName(QString::fromWCharArray(entry.szExeFile));
        } while (!found && Process32NextW(processes, &entry));
    }
    CloseHandle(processes);
    return found;
}
bool available(HWND window) {
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    return !IsIconic(window) && !cloaked;
}
bool isOverlayTarget(HWND window) {
    if (!window || !IsWindowVisible(window) || GetWindow(window, GW_OWNER)) return false;
    const auto style = GetWindowLongPtrW(window, GWL_STYLE);
    const auto extendedStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if ((style & WS_CHILD) || (extendedStyle & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE))) return false;
    // Telegram's frameless main window can have WS_POPUP with normal window controls.
    return !(style & WS_POPUP) || (style & WS_SYSMENU) || (extendedStyle & WS_EX_APPWINDOW);
}
HWND findTelegramWindow(DWORD requiredProcess) {
    struct Search {
        DWORD process;
        HWND target = nullptr;
    } search{requiredProcess};
    EnumWindows([](HWND window, LPARAM value) -> BOOL {
        auto& search = *reinterpret_cast<Search*>(value);
        if (!isOverlayTarget(window)) return TRUE;
        const DWORD pid = windowProcess(window);
        if (search.process ? pid != search.process : !telegram(pid)) return TRUE;
        if (!available(window)) { if (!search.target) search.target = window; return TRUE; }
        search.target = window;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&search));
    return search.target;
}
constexpr auto runKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
class WindowsPlatform final : public NativePlatform {
public:
    WindowsPlatform() {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comInitialized_ = SUCCEEDED(result);
        CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&desktops_));
    }
    ~WindowsPlatform() override {
        setRenderingActive(false);
        watchPrimaryPress(0, {});
        for (auto hook : hooks_) UnhookWinEvent(hook);
        instance_ = nullptr;
        if (desktops_) desktops_->Release();
        if (comInitialized_) CoUninitialize();
    }
    WindowSnapshot sample() override {
        WindowSnapshot result;
        const HWND foreground = GetForegroundWindow();
        const DWORD foregroundProcess = windowProcess(foreground);
        const bool telegramForeground = foregroundProcess && telegram(foregroundProcess);
        HWND target = nullptr;
        // Re-check identity each sample. Never trust a recycled HWND or PID cache.
        if (telegramForeground) {
            const HWND owner = GetAncestor(foreground, GA_ROOTOWNER);
            if (isOverlayTarget(owner) && windowProcess(owner) == foregroundProcess)
                target = owner;
        }
        // Re-evaluate native z-order when focus changes and periodically while
        // Telegram is in the background. A newly raised second window can
        // cover the cached one without changing the foreground process.
        if (!target && foreground == lastForeground_ && selectionAge_.isValid()
            && selectionAge_.elapsed() < 400 && isOverlayTarget(lastTarget_) && available(lastTarget_)) {
            const DWORD pid = windowProcess(lastTarget_);
            if (telegramForeground ? pid == foregroundProcess : telegram(pid))
                target = lastTarget_;
        }
        if (!target) {
            target = findTelegramWindow(telegramForeground ? foregroundProcess : 0);
            selectionAge_.start();
        }
        lastForeground_ = foreground;
        result.active = telegramForeground && target != nullptr;
        lastTarget_ = target;
        result.running = target != nullptr || telegramRunning();
        if (!target) { result.detail = result.running ? QStringLiteral("Окно Telegram скрыто") : QStringLiteral("Ожидание Telegram"); return result; }
        result.id = reinterpret_cast<quintptr>(target);
        BOOL cloaked = FALSE;
        DwmGetWindowAttribute(target, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        BOOL currentDesktop = TRUE;
        if (desktops_ && FAILED(desktops_->IsWindowOnCurrentVirtualDesktop(target, &currentDesktop)))
            currentDesktop = TRUE; // DWM cloaking still filters windows when the shell is unavailable.
        result.visible = IsWindowVisible(target) && !IsIconic(target) && !cloaked && currentDesktop;
        RECT bounds{};
        if (FAILED(DwmGetWindowAttribute(target, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds))))
            GetWindowRect(target, &bounds);
        if (overlay_ && IsWindowVisible(overlay_)) {
            RECT rect{};
            result.needsPlacement = GetWindow(target, GW_HWNDPREV) != overlay_
                || !GetWindowRect(overlay_, &rect) || !EqualRect(&bounds, &rect);
        }
        result.bounds = QRectF(bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
        result.detail = result.canOverlay() ? QStringLiteral("Telegram подключён")
            : QStringLiteral("Пауза — окно свёрнуто или недоступно");
        return result;
    }
    void configureOverlay(QWindow& window) override {
        const HWND handle = reinterpret_cast<HWND>(window.winId());
        overlay_ = handle;
        auto style = GetWindowLongPtrW(handle, GWL_EXSTYLE);
        style |= WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
        style &= ~WS_EX_APPWINDOW;
        SetWindowLongPtrW(handle, GWL_EXSTYLE, style);
        SetWindowPos(handle, nullptr, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        // Qt's alpha swapchain owns transparency. Do not call SetLayeredWindowAttributes:
        // doing so can interfere with Qt's per-pixel alpha path.
    }
    void placeOverlay(QWindow& window, const WindowSnapshot& snapshot) override {
        const auto& r = snapshot.bounds;
        // Use physical coordinates directly. Qt handles WM_DPICHANGED and backing-store DPR.
        // Dividing global coordinates by DPR is incorrect on mixed-scale monitor layouts.
        const HWND handle = reinterpret_cast<HWND>(window.winId());
        const HWND target = reinterpret_cast<HWND>(snapshot.id);
        HWND above = GetWindow(target, GW_HWNDPREV);
        if (above == handle) above = GetWindow(handle, GW_HWNDPREV);
        const bool topmost = GetWindowLongPtrW(target, GWL_EXSTYLE) & WS_EX_TOPMOST;
        if (!topmost) {
            if (above && (GetWindowLongPtrW(above, GWL_EXSTYLE) & WS_EX_TOPMOST)) above = nullptr;
            if (GetWindowLongPtrW(handle, GWL_EXSTYLE) & WS_EX_TOPMOST)
                SetWindowPos(handle, HWND_NOTOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        SetWindowPos(handle, above ? above : (topmost ? HWND_TOPMOST : HWND_TOP),
            qRound(r.x()), qRound(r.y()), qRound(r.width()), qRound(r.height()),
            SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    QPointF cursorInOverlay(const WindowSnapshot&, const QWindow& overlay) const override {
        POINT cursor{}; GetCursorPos(&cursor);
        ScreenToClient(reinterpret_cast<HWND>(overlay.winId()), &cursor);
        return QPointF(cursor.x / overlay.devicePixelRatio(), cursor.y / overlay.devicePixelRatio());
    }
    void watchPrimaryPress(quintptr target, std::function<void(QPointF)> pressed) override {
        if (pressHook_) {
            UnhookWindowsHookEx(pressHook_);
            pressHook_ = nullptr;
        }
        pressTarget_ = reinterpret_cast<HWND>(target);
        pressed_ = std::move(pressed);
        if (target) pressHook_ = SetWindowsHookExW(WH_MOUSE_LL, &WindowsPlatform::onMouse, GetModuleHandleW(nullptr), 0);
    }
    void watch(std::function<void()> changed) override {
        changed_ = std::move(changed); instance_ = this;
        const auto add = [this](DWORD first, DWORD last) {
            if (auto hook = SetWinEventHook(first, last, nullptr, &WindowsPlatform::event, 0, 0,
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS)) hooks_.push_back(hook);
        };
        add(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND);
        add(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND);
        add(EVENT_SYSTEM_DESKTOPSWITCH, EVENT_SYSTEM_DESKTOPSWITCH);
        add(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE);
        add(EVENT_OBJECT_DESTROY, EVENT_OBJECT_HIDE);
    }
    bool reduceMotion() const override {
        BOOL enabled = TRUE;
        return SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0) && !enabled;
    }
    bool autoStartEnabled() const override {
        QSettings settings(QString::fromLatin1(runKey), QSettings::NativeFormat);
        return settings.contains(QStringLiteral("TelegramEffects"));
    }
    bool setAutoStart(bool enabled, QString& error) override {
        QSettings settings(QString::fromLatin1(runKey), QSettings::NativeFormat);
        if (enabled) settings.setValue(QStringLiteral("TelegramEffects"),
            QStringLiteral("\"%1\" --background").arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())));
        else settings.remove(QStringLiteral("TelegramEffects"));
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            error = QStringLiteral("Не удалось изменить автозапуск для текущего пользователя."); return false;
        }
        return true;
    }
    void setRenderingActive(bool active) override {
        if (active == renderingActive_) return;
        renderingActive_ = active;
        // Opt out of EcoQoS while animating: once the settings window is minimized or
        // closed to the tray Windows 11 may otherwise slow the process and its timers.
        // Returning the bits to the system restores default power management.
#ifndef PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
#define PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION 0x4
#endif
        PROCESS_POWER_THROTTLING_STATE state{};
        state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        state.ControlMask = active ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED
            | PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION : 0;
        state.StateMask = 0;
        if (!SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &state, sizeof(state)) && active) {
            state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED; // Windows 10 lacks the timer flag.
            SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &state, sizeof(state));
        }
    }
private:
    static LRESULT CALLBACK onMouse(int code, WPARAM message, LPARAM data) {
        auto* self = instance_;
        if (code == HC_ACTION && message == WM_LBUTTONDOWN && self && self->pressed_
            && GetForegroundWindow() == self->pressTarget_) {
            const POINT point = reinterpret_cast<const MSLLHOOKSTRUCT*>(data)->pt;
            const HWND under = GetAncestor(WindowFromPoint(point), GA_ROOT);
            if (under == self->pressTarget_ || under == self->overlay_)
                self->pressed_(QPointF(point.x, point.y));
        }
        return CallNextHookEx(nullptr,code,message,data);
    }
    HHOOK pressHook_ = nullptr;
    HWND pressTarget_ = nullptr;
    std::function<void(QPointF)> pressed_;
    static void CALLBACK event(HWINEVENTHOOK, DWORD type, HWND window, LONG object, LONG, DWORD, DWORD) {
        if (!instance_ || !instance_->changed_) return;
        if (type >= EVENT_OBJECT_DESTROY && (object != OBJID_WINDOW || window != instance_->lastTarget_)) return;
        instance_->changed_();
    }
    inline static WindowsPlatform* instance_ = nullptr;
    std::function<void()> changed_;
    std::vector<HWINEVENTHOOK> hooks_;
    HWND overlay_ = nullptr;
    HWND lastForeground_ = nullptr;
    QElapsedTimer selectionAge_;
    HWND lastTarget_ = nullptr;
    IVirtualDesktopManager* desktops_ = nullptr;
    bool comInitialized_ = false;
    bool renderingActive_ = false;
};
}
std::unique_ptr<NativePlatform> createNativePlatform() { return std::make_unique<WindowsPlatform>(); }
}
