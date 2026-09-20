#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <commctrl.h>
#include <dinput.h>
#include <shellapi.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <atomic>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "mapping.h"
#include "version.h"
#include "native_smoke.h"
using std::wstring;
static HWND win, tabs, deviceBox, statusText, liveText, rawText, feedbackText, startButton;
static HFONT font, titleFont, smallFont;
static HBRUSH background;
static std::vector<HWND> pages[3];
static int page = 0;
static double uiScale = 1;
static bool uiSmoke = false;
static int startupExit = 0;
static int S(int n) {
    return int(std::lround(n * uiScale));
}
static wstring root, configPath, logPath;
static IDirectInput8W *di = nullptr;
static IDirectInputDevice8W *wheel = nullptr;
static IDirectInputEffect *effect = nullptr;
static bool running = false, calibrating = false, hasState = false, feedbackReady = false, hotkey = false,
            notified = false;
static bool connected = false, added = false;
static UINT_PTR timerId = 0;
static ULONGLONG testUntil = 0, xboxTestUntil = 0, forceStarted = 0;
static DWORD oldAutocenter = 0, oldDeviceGain = 10000;
static bool restoreDeviceGain = false;
static thrusty::WheelMotion motion;
static bool restoreAutocenter = false;
struct Vibration {
    WORD left, right;
};
static HMODULE xinput = nullptr;
static DWORD(WINAPI *setVibration)(DWORD, Vibration *) = nullptr;
static DWORD xboxIndex = 0;
static DIJOYSTATE2 state{};
static int calMin[8], calMax[8], calRest[8];
static std::atomic<unsigned> motors{0}, packets{0};
static std::atomic<ULONGLONG> lastRumble{0};
static HWND axisBoxes[3], inverseBoxes[3], mapBoxes[12], deadEdit, curveEdit, gainEdit, springEdit, reachEdit,
    boostEdit, feedbackCheck, reverseCheck, assistCheck;
static HWND meters[5];
static const WORD masks[12] = {0x1000, 0x2000, 0x4000, 0x8000, 0x100, 0x200,
                               0x20,   0x10,   0x40,   0x80,   0x400, 0};
static const wchar_t *mapNames[12] = {L"A",
                                      L"B",
                                      L"X",
                                      L"Y",
                                      L"LB / shift down",
                                      L"RB / shift up",
                                      L"Back",
                                      L"Start",
                                      L"Left stick click",
                                      L"Right stick click",
                                      L"Guide",
                                      L"Handbrake (A)"};
static const wchar_t *axisNames[8] = {L"X", L"Y", L"Z", L"Rx", L"Ry", L"Rz", L"Slider 1", L"Slider 2"};
struct Config {
    int axes[3] = {0, 5, 1};
    int invert[3] = {0, 0, 0};
    int lo = 0, mid = 32767, hi = 65535;
    int rest[2] = {65535, 65535}, full[2] = {0, 0};
    int dead = 3, curve = 100, gain = 15, spring = 0, reach = 25, boost = 0, assist = 1, enable = 0,
        reverse = 0;
    int map[12] = {2, 3, 1, 4, 5, 6, 9, 10, 11, 12, 13, 0};
    wstring device;
} cfg;
struct Device {
    GUID guid;
    wstring name;
};
static std::vector<Device> devices;
#pragma pack(push, 1)
struct Report {
    USHORT buttons;
    BYTE lt, rt;
    SHORT lx, ly, rx, ry;
};
#pragma pack(pop)
static_assert(sizeof(Report) == 12, "XUSB_REPORT ABI");
using Notice = void(CALLBACK *)(void *, void *, UCHAR, UCHAR, UCHAR, void *);
struct Vigem {
    HMODULE dll = nullptr;
    void *client = nullptr;
    void *target = nullptr;
    void *(__cdecl *alloc)() = nullptr;
    void(__cdecl *freeClient)(void *) = nullptr;
    DWORD(__cdecl *connect)(void *) = nullptr;
    void(__cdecl *disconnect)(void *) = nullptr;
    void *(__cdecl *allocTarget)() = nullptr;
    void(__cdecl *freeTarget)(void *) = nullptr;
    DWORD(__cdecl *add)(void *, void *) = nullptr;
    DWORD(__cdecl *remove)(void *, void *) = nullptr;
    DWORD(__cdecl *update)(void *, void *, Report) = nullptr;
    DWORD(__cdecl *getIndex)(void *, void *, ULONG *) = nullptr;
    DWORD(__cdecl *reg)(void *, void *, Notice, void *) = nullptr;
    void(__cdecl *unreg)(void *) = nullptr;
} v;
static constexpr DWORD OK = 0x20000000;
static void log(const wstring &s) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::wofstream f(logPath.c_str(), std::ios::app);
    f << st.wYear << L"-" << st.wMonth << L"-" << st.wDay << L" " << st.wHour << L":" << st.wMinute << L":"
      << st.wSecond << L" " << s << L"\n";
}
static wstring hex(DWORD n) {
    std::wstringstream s;
    s << L"0x" << std::hex << std::uppercase << n;
    return s.str();
}
static void status(const wstring &s) {
    SetWindowTextW(statusText, s.c_str());
    log(s);
}
static void error(const wstring &s) {
    status(s);
    MessageBoxW(win, s.c_str(), L"Thrusty", MB_OK | MB_ICONWARNING);
}
static wstring guidText(const GUID &g) {
    wchar_t b[40];
    StringFromGUID2(g, b, 40);
    return b;
}
static int setting(const wchar_t *key, int def) {
    return GetPrivateProfileIntW(L"Thrusty", key, def, configPath.c_str());
}
static bool settingsWriteOk = true;
static void put(const wchar_t *key, int value) {
    settingsWriteOk &=
        !!WritePrivateProfileStringW(L"Thrusty", key, std::to_wstring(value).c_str(), configPath.c_str());
}
static bool save() {
    settingsWriteOk = true;
    for (int i = 0; i < 3; i++) {
        put((L"Axis" + std::to_wstring(i)).c_str(), cfg.axes[i]);
        put((L"Invert" + std::to_wstring(i)).c_str(), cfg.invert[i]);
    }
    for (int i = 0; i < 2; i++) {
        put((L"Rest" + std::to_wstring(i)).c_str(), cfg.rest[i]);
        put((L"Full" + std::to_wstring(i)).c_str(), cfg.full[i]);
    }
    for (int i = 0; i < 12; i++)
        put((L"Button" + std::to_wstring(i)).c_str(), cfg.map[i]);
    put(L"Low", cfg.lo);
    put(L"Center", cfg.mid);
    put(L"High", cfg.hi);
    put(L"Deadzone", cfg.dead);
    put(L"Curve", cfg.curve);
    put(L"Gain", cfg.gain);
    put(L"Spring", cfg.spring);
    put(L"SpringReach", cfg.reach);
    put(L"CenterBoost", cfg.boost);
    put(L"PowerSteering", cfg.assist);
    put(L"Feedback", cfg.enable);
    put(L"ReverseForce", cfg.reverse);
    settingsWriteOk &=
        !!WritePrivateProfileStringW(L"Thrusty", L"Device", cfg.device.c_str(), configPath.c_str());
    if (!settingsWriteOk)
        error(L"Windows could not save settings. Check access to your local app-data folder.");
    return settingsWriteOk;
}
static int bounded(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}
static void load() {
    for (int i = 0; i < 3; i++) {
        cfg.axes[i] = bounded(setting((L"Axis" + std::to_wstring(i)).c_str(), cfg.axes[i]), 0, 7);
        cfg.invert[i] = !!setting((L"Invert" + std::to_wstring(i)).c_str(), 0);
    }
    for (int i = 0; i < 2; i++) {
        cfg.rest[i] = bounded(setting((L"Rest" + std::to_wstring(i)).c_str(), 65535), 0, 65535);
        cfg.full[i] = bounded(setting((L"Full" + std::to_wstring(i)).c_str(), 0), 0, 65535);
    }
    for (int i = 0; i < 12; i++)
        cfg.map[i] = bounded(setting((L"Button" + std::to_wstring(i)).c_str(), cfg.map[i]), 0, 128);
    cfg.lo = bounded(setting(L"Low", 0), 0, 65535);
    cfg.mid = bounded(setting(L"Center", 32767), 0, 65535);
    cfg.hi = bounded(setting(L"High", 65535), 0, 65535);
    if (cfg.mid - cfg.lo < 100 || cfg.hi - cfg.mid < 100) {
        cfg.lo = 0;
        cfg.mid = 32767;
        cfg.hi = 65535;
    }
    cfg.dead = bounded(setting(L"Deadzone", 3), 0, 25);
    cfg.curve = bounded(setting(L"Curve", 100), 50, 250);
    cfg.gain = bounded(setting(L"Gain", 15), 0, 35);
    cfg.spring = bounded(setting(L"Spring", 0), 0, 100);
    cfg.assist = !!setting(L"PowerSteering", 1);
    cfg.boost = bounded(setting(L"CenterBoost", 0), 0, 100);
    cfg.reach = bounded(setting(L"SpringReach", 25), 5, 100);
    cfg.enable = !!setting(L"Feedback", 0);
    cfg.reverse = !!setting(L"ReverseForce", 0);
    wchar_t b[80];
    GetPrivateProfileStringW(L"Thrusty", L"Device", L"", b, 80, configPath.c_str());
    cfg.device = b;
}
static HWND control(const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id,
                    int p = -1) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), win,
                             (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    if (p >= 0)
        pages[p].push_back(c);
    return c;
}
static void label(const wchar_t *text, int x, int y, int w, int h, int p) {
    control(L"STATIC", text, 0, x, y, w, h, 0, p);
}
static HWND button(const wchar_t *text, int x, int y, int w, int h, int id, int p = -1) {
    return control(L"BUTTON", text, WS_TABSTOP | BS_PUSHBUTTON, x, y, w, h, id, p);
}
static HWND combo(int x, int y, int w, int id, int p) {
    return control(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, x, y, w, 280, id, p);
}
static void item(HWND c, const wstring &s) {
    SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)s.c_str());
}
static int selected(HWND c) {
    return (int)SendMessageW(c, CB_GETCURSEL, 0, 0);
}
static HWND edit(int x, int y, int w, int id, int p, int val) {
    HWND c = control(L"EDIT", std::to_wstring(val).c_str(),
                     WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL, x, y, w, 24, id, p);
    SendMessageW(c, EM_LIMITTEXT, 3, 0);
    return c;
}
static int number(HWND c, int lo, int hi) {
    wchar_t b[20];
    GetWindowTextW(c, b, 20);
    return bounded(_wtoi(b), lo, hi);
}
static HWND check(const wchar_t *text, int x, int y, int w, int id, int p, bool on) {
    HWND c = control(L"BUTTON", text, BS_AUTOCHECKBOX | WS_TABSTOP, x, y, w, 24, id, p);
    SendMessageW(c, BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
    return c;
}
static bool checked(HWND c) {
    return SendMessageW(c, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
static void stopForces();
static bool readControls() {
    stopForces();
    if (selected(axisBoxes[0]) == selected(axisBoxes[1]) ||
        selected(axisBoxes[0]) == selected(axisBoxes[2]) ||
        selected(axisBoxes[1]) == selected(axisBoxes[2])) {
        error(L"Choose three distinct axes for steering, accelerator and brake. Combined pedals are not "
              L"supported.");
        return false;
    }
    for (int i = 0; i < 3; i++) {
        int n = bounded(selected(axisBoxes[i]), 0, 7);
        if (n != cfg.axes[i]) {
            if (i == 0) {
                cfg.lo = 0;
                cfg.mid = 32767;
                cfg.hi = 65535;
            } else {
                cfg.rest[i - 1] = 65535;
                cfg.full[i - 1] = 0;
            }
        }
        cfg.axes[i] = n;
        cfg.invert[i] = checked(inverseBoxes[i]);
    }
    for (int i = 0; i < 12; i++)
        cfg.map[i] = bounded(selected(mapBoxes[i]), 0, 128);
    cfg.dead = number(deadEdit, 0, 25);
    cfg.curve = number(curveEdit, 50, 250);
    cfg.gain = number(gainEdit, 0, 35);
    cfg.spring = number(springEdit, 0, 100);
    cfg.assist = checked(assistCheck);
    cfg.boost = number(boostEdit, 0, 100);
    cfg.reach = number(reachEdit, 5, 100);
    cfg.enable = checked(feedbackCheck);
    cfg.reverse = checked(reverseCheck);
    SetWindowTextW(deadEdit, std::to_wstring(cfg.dead).c_str());
    SetWindowTextW(curveEdit, std::to_wstring(cfg.curve).c_str());
    SetWindowTextW(gainEdit, std::to_wstring(cfg.gain).c_str());
    SetWindowTextW(springEdit, std::to_wstring(cfg.spring).c_str());
    SetWindowTextW(reachEdit, std::to_wstring(cfg.reach).c_str());
    SetWindowTextW(boostEdit, std::to_wstring(cfg.boost).c_str());
    return save();
}
static void pageTo(int n) {
    page = n;
    for (int p = 0; p < 3; p++)
        for (HWND c : pages[p])
            ShowWindow(c, p == n ? SW_SHOW : SW_HIDE);
    InvalidateRect(win, nullptr, TRUE);
}
static void stopForces() {
    testUntil = 0;
    if (xboxTestUntil && setVibration) {
        Vibration zero{};
        setVibration(xboxIndex, &zero);
    }
    xboxTestUntil = 0;
    motors = 0;
    if (effect)
        effect->Stop();
    if (wheel)
        wheel->SendForceFeedbackCommand(DISFFC_STOPALL);
}
static void closeWheel() {
    stopForces();
    motion.reset();
    feedbackReady = false;
    hasState = false;
    if (effect) {
        effect->Release();
        effect = nullptr;
    }
    if (wheel) {
        if (restoreDeviceGain) {
            DIPROPDWORD g{};
            g.diph.dwSize = sizeof(g);
            g.diph.dwHeaderSize = sizeof(g.diph);
            g.diph.dwHow = DIPH_DEVICE;
            g.dwData = oldDeviceGain;
            wheel->SetProperty(DIPROP_FFGAIN, &g.diph);
        }
        restoreDeviceGain = false;
        wheel->Unacquire();
        if (restoreAutocenter) {
            DIPROPDWORD a{};
            a.diph.dwSize = sizeof(a);
            a.diph.dwHeaderSize = sizeof(a.diph);
            a.diph.dwHow = DIPH_DEVICE;
            a.dwData = oldAutocenter;
            wheel->SetProperty(DIPROP_AUTOCENTER, &a.diph);
        }
        restoreAutocenter = false;
        wheel->Release();
        wheel = nullptr;
    }
}
static void stopBridge() {
    running = false;
    stopForces();
    if (v.target) {
        if (added)
            v.update(v.client, v.target, Report{});
        if (notified) {
            v.unreg(v.target);
            notified = false;
        }
        if (added) {
            v.remove(v.client, v.target);
            added = false;
        }
        v.freeTarget(v.target);
        v.target = nullptr;
    }
    if (v.client) {
        if (connected) {
            v.disconnect(v.client);
            connected = false;
        }
        v.freeClient(v.client);
        v.client = nullptr;
    }
    motors = 0;
    lastRumble = 0;
    SetWindowTextW(startButton, L"Start bridge");
    EnableWindow(deviceBox, TRUE);
}
static BOOL CALLBACK enumDevices(const DIDEVICEINSTANCEW *d, void *) {
    // Never feed the emulated Xbox controller back into itself.
    if (LOWORD(d->guidProduct.Data1) == 0x044f || wcsstr(d->tszProductName, L"T300") ||
        wcsstr(d->tszProductName, L"Thrustmaster"))
        devices.push_back({d->guidInstance, d->tszProductName});
    return DIENUM_CONTINUE;
}
static BOOL CALLBACK setAxis(const DIDEVICEOBJECTINSTANCEW *obj, void *) {
    if (obj->dwType & DIDFT_AXIS) {
        DIPROPRANGE r{};
        r.diph.dwSize = sizeof(r);
        r.diph.dwHeaderSize = sizeof(r.diph);
        r.diph.dwHow = DIPH_BYID;
        r.diph.dwObj = obj->dwType;
        r.lMin = 0;
        r.lMax = 65535;
        wheel->SetProperty(DIPROP_RANGE, &r.diph);
    }
    return DIENUM_CONTINUE;
}
static bool acquireWheel(bool forces) {
    closeWheel();
    int n = selected(deviceBox);
    if (n < 0 || n >= (int)devices.size()) {
        error(L"No Thrustmaster wheel selected. Connect the T300, install its driver, then click Refresh.");
        return false;
    }
    HRESULT hr = di->CreateDevice(devices[n].guid, &wheel, nullptr);
    if (FAILED(hr)) {
        error(L"Cannot open wheel: " + hex(hr));
        return false;
    }
    hr = wheel->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        closeWheel();
        error(L"Cannot select DirectInput format: " + hex(hr));
        return false;
    }
    hr = wheel->SetCooperativeLevel(win, DISCL_BACKGROUND | (forces ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE));
    if (FAILED(hr)) {
        closeWheel();
        error(L"Cannot set wheel access: " + hex(hr));
        return false;
    }
    wheel->EnumObjects(setAxis, nullptr, DIDFT_AXIS);
    if (forces) {
        DIPROPDWORD ac{};
        ac.diph.dwSize = sizeof(ac);
        ac.diph.dwHeaderSize = sizeof(ac.diph);
        ac.diph.dwHow = DIPH_DEVICE;
        if (SUCCEEDED(wheel->GetProperty(DIPROP_AUTOCENTER, &ac.diph))) {
            oldAutocenter = ac.dwData;
            restoreAutocenter = true;
        }
        ac.dwData = DIPROPAUTOCENTER_OFF;
        wheel->SetProperty(DIPROP_AUTOCENTER, &ac.diph);
    }
    hr = wheel->Acquire();
    if (FAILED(hr)) {
        closeWheel();
        error(L"Wheel is busy or disconnected. Close its control panel and other wheel software, then try "
              L"again. " +
              hex(hr));
        return false;
    }
    cfg.device = guidText(devices[n].guid);
    if (forces) {
        DIPROPDWORD gain{};
        gain.diph.dwSize = sizeof(gain);
        gain.diph.dwHeaderSize = sizeof(gain.diph);
        gain.diph.dwHow = DIPH_DEVICE;
        if (SUCCEEDED(wheel->GetProperty(DIPROP_FFGAIN, &gain.diph))) {
            oldDeviceGain = gain.dwData;
            restoreDeviceGain = true;
        }
        wheel->SendForceFeedbackCommand(DISFFC_RESET);
        gain.dwData = DI_FFNOMINALMAX;
        HRESULT gainResult = wheel->SetProperty(DIPROP_FFGAIN, &gain.diph);
        if (FAILED(gainResult))
            log(L"Cannot set full DirectInput device gain; driver may scale forces: " + hex(gainResult));
        DWORD axis = DIJOFS_X;
        LONG direction = 10000;
        DICONSTANTFORCE cf{};
        DIEFFECT e{};
        e.dwSize = sizeof(e);
        e.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
        e.dwDuration = 100000;
        e.dwGain = DI_FFNOMINALMAX;
        e.dwTriggerButton = DIEB_NOTRIGGER;
        e.cAxes = 1;
        e.rgdwAxes = &axis;
        e.rglDirection = &direction;
        e.cbTypeSpecificParams = sizeof(cf);
        e.lpvTypeSpecificParams = &cf;
        hr = wheel->CreateEffect(GUID_ConstantForce, &e, &effect, nullptr);
        feedbackReady = SUCCEEDED(hr);
        if (!feedbackReady)
            log(L"Constant-force effect unavailable: " + hex(hr));
    }
    return true;
}
static void refresh() {
    stopBridge();
    closeWheel();
    devices.clear();
    SendMessageW(deviceBox, CB_RESETCONTENT, 0, 0);
    di->EnumDevices(DI8DEVCLASS_GAMECTRL, enumDevices, nullptr, DIEDFL_ATTACHEDONLY);
    int found = 0;
    for (size_t i = 0; i < devices.size(); ++i) {
        item(deviceBox, devices[i].name);
        if (guidText(devices[i].guid) == cfg.device)
            found = (int)i;
    }
    if (devices.empty()) {
        status(L"No Thrustmaster wheel found. Connect the wheel, then Refresh.");
        return;
    }
    SendMessageW(deviceBox, CB_SETCURSEL, found, 0);
    if (acquireWheel(false))
        status(L"Wheel connected. Set your axes, then calibrate before driving.");
}
static void CALLBACK rumble(void *, void *, UCHAR large, UCHAR small, UCHAR, void *) {
    motors.store(unsigned(large) | (unsigned(small) << 8));
    lastRumble.store(GetTickCount64());
    packets.fetch_add(1);
}
static bool loadVigem() {
    if (v.dll)
        return true;
    v.dll = LoadLibraryExW((root + L"\\ViGEmClient.dll").c_str(), nullptr,
                           LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!v.dll) {
        error(L"Cannot load ViGEmClient.dll (" + hex(GetLastError()) +
              L"). Keep it next to Thrusty.exe. See README for runtime troubleshooting.");
        return false;
    }
#define API(field, name)                                                                                     \
    v.field = reinterpret_cast<decltype(v.field)>(GetProcAddress(v.dll, name));                              \
    if (!v.field) {                                                                                          \
        FreeLibrary(v.dll);                                                                                  \
        v.dll = nullptr;                                                                                     \
        error(L"ViGEmClient.dll is missing a required export.");                                             \
        return false;                                                                                        \
    }
    API(alloc, "vigem_alloc");
    API(freeClient, "vigem_free");
    API(connect, "vigem_connect");
    API(disconnect, "vigem_disconnect");
    API(allocTarget, "vigem_target_x360_alloc");
    API(freeTarget, "vigem_target_free");
    API(add, "vigem_target_add");
    API(remove, "vigem_target_remove");
    API(update, "vigem_target_x360_update");
    API(getIndex, "vigem_target_x360_get_user_index");
    API(reg, "vigem_target_x360_register_notification");
    API(unreg, "vigem_target_x360_unregister_notification");
#undef API
    return true;
}
static void testXbox() {
    if (!running) {
        error(L"Start the bridge before testing Xbox rumble.");
        return;
    }
    if (!xinput) {
        xinput = LoadLibraryExW(L"xinput1_4.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (xinput)
            setVibration = reinterpret_cast<decltype(setVibration)>(GetProcAddress(xinput, "XInputSetState"));
    }
    ULONG idx = 0;
    DWORD e = v.getIndex(v.client, v.target, &idx);
    if (e != OK || !setVibration) {
        error(L"Xbox rumble test unavailable. Check the controller driver.");
        return;
    }
    xboxIndex = idx;
    Vibration pulse{28000, 16000};
    e = setVibration(xboxIndex, &pulse);
    if (e != ERROR_SUCCESS) {
        error(L"XInput could not send rumble: " + hex(e));
        return;
    }
    xboxTestUntil = GetTickCount64() + 1000;
    status(L"Xbox rumble test running. Watch incoming message count on Feedback.");
}
static void start() {
    if (calibrating) {
        error(L"Finish or cancel calibration before starting.");
        return;
    }
    if (!readControls())
        return;
    if (!acquireWheel(cfg.enable))
        return;
    if (!loadVigem())
        return;
    v.client = v.alloc();
    if (!v.client) {
        error(L"Could not allocate virtual-controller client.");
        return;
    }
    DWORD e = v.connect(v.client);
    if (e != OK) {
        stopBridge();
        error(L"ViGEmBus is not ready (" + hex(e) +
              L"). Install the bundled driver from Setup, restart Windows if requested, and try again.");
        return;
    }
    connected = true;
    v.target = v.allocTarget();
    if (!v.target) {
        stopBridge();
        error(L"Could not allocate Xbox 360 controller.");
        return;
    }
    e = v.add(v.client, v.target);
    if (e != OK) {
        stopBridge();
        error(L"Could not attach Xbox controller: " + hex(e));
        return;
    }
    added = true;
    packets = 0;
    motors = 0;
    lastRumble = 0;
    e = v.reg(v.client, v.target, rumble, nullptr);
    notified = e == OK;
    running = true;
    forceStarted = GetTickCount64();
    SetWindowTextW(startButton, L"Stop bridge");
    EnableWindow(deviceBox, FALSE);
    save();
    status(!notified                      ? L"Input bridge running; rumble callback failed. See log."
           : cfg.enable && !feedbackReady ? L"Input bridge running; wheel force effect unavailable."
                                          : L"Bridge running. Open GeForce NOW and select gamepad controls.");
    if (!notified)
        log(L"Notification registration failed: " + hex(e));
}
static int axisValue(int n) {
    const LONG axes[] = {state.lX,  state.lY,  state.lZ,           state.lRx,
                         state.lRy, state.lRz, state.rglSlider[0], state.rglSlider[1]};
    return axes[bounded(n, 0, 7)];
}
static void calibration() {
    if (running) {
        error(L"Stop the bridge before calibration.");
        return;
    }
    if (!calibrating) {
        if (!readControls())
            return;
        if (!wheel && !acquireWheel(false))
            return;
        if (!hasState) {
            error(L"Wait for live wheel readings, then try again.");
            return;
        }
        for (int i = 0; i < 8; i++)
            calRest[i] = calMin[i] = calMax[i] = axisValue(i);
        calibrating = true;
        SetWindowTextW(GetDlgItem(win, 104), L"Finish calibration");
        status(L"Turn fully left/right; press and release both pedals. Then Finish calibration.");
    } else {
        int s = cfg.axes[0], a = cfg.axes[1], b = cfg.axes[2];
        int fullA =
            std::abs(calMax[a] - calRest[a]) > std::abs(calMin[a] - calRest[a]) ? calMax[a] : calMin[a];
        int fullB =
            std::abs(calMax[b] - calRest[b]) > std::abs(calMin[b] - calRest[b]) ? calMax[b] : calMin[b];
        if (calRest[s] - calMin[s] < 1000 || calMax[s] - calRest[s] < 1000 ||
            std::abs(fullA - calRest[a]) < 1000 || std::abs(fullB - calRest[b]) < 1000) {
            error(L"Not enough movement. Check selected axes, turn both ways and press each pedal fully. "
                  L"Calibration is still recording; Cancel leaves your old settings intact.");
            return;
        }
        cfg.lo = calMin[s];
        cfg.mid = calRest[s];
        cfg.hi = calMax[s];
        cfg.rest[0] = calRest[a];
        cfg.rest[1] = calRest[b];
        cfg.full[0] = fullA;
        cfg.full[1] = fullB;
        calibrating = false;
        SetWindowTextW(GetDlgItem(win, 104), L"Calibrate...");
        if (!save())
            return;
        status(L"Calibration saved. Check steering and pedals in the live meters.");
    }
}
static void tick() {
    if (!wheel)
        return;
    HRESULT h = wheel->Poll();
    if (SUCCEEDED(h))
        h = wheel->GetDeviceState(sizeof(state), &state);
    if (FAILED(h)) {
        if (hasState || running) {
            stopBridge();
            closeWheel();
            calibrating = false;
            SetWindowTextW(GetDlgItem(win, 104), L"Calibrate...");
            status(L"Wheel input lost. Output and forces stopped. Click Refresh to reconnect.");
        } else {
            wheel->Acquire();
        }
        return;
    }
    hasState = true;
    int raw[8];
    for (int i = 0; i < 8; i++) {
        raw[i] = axisValue(i);
        if (calibrating) {
            calMin[i] = std::min(calMin[i], raw[i]);
            calMax[i] = std::max(calMax[i], raw[i]);
        }
    }
    double x = thrusty::steering(raw[cfg.axes[0]], cfg.lo, cfg.mid, cfg.hi, cfg.dead / 100.0,
                                 cfg.curve / 100.0, cfg.invert[0]);
    Report report{};
    report.lx = thrusty::stick(x);
    report.rt = thrusty::pedal(raw[cfg.axes[1]], cfg.rest[0], cfg.full[0], cfg.dead / 100.0, cfg.invert[1]);
    report.lt = thrusty::pedal(raw[cfg.axes[2]], cfg.rest[1], cfg.full[1], cfg.dead / 100.0, cfg.invert[2]);
    report.buttons = thrusty::pov(state.rgdwPOV[0]);
    for (int i = 0; i < 12; i++)
        if (cfg.map[i] > 0 && (state.rgbButtons[cfg.map[i] - 1] & 0x80))
            report.buttons |= i == 11 ? 0x1000 : masks[i];
    ULONGLONG now = GetTickCount64();
    if (xboxTestUntil && now >= xboxTestUntil) {
        Vibration zero{};
        setVibration(xboxIndex, &zero);
        xboxTestUntil = 0;
    }
    unsigned m = motors.load();
    ULONGLONG last = lastRumble.load();
    bool stale = last && now - last > 5000;
    if (stale)
        m = 0;
    if (running) {
        DWORD e = v.update(v.client, v.target, report);
        if (e != OK) {
            stopBridge();
            status(L"Virtual controller disconnected. Bridge stopped: " + hex(e));
            return;
        }
    }
    bool testing = testUntil > now;
    int low = m & 255, high = (m >> 8) & 255;
    if (effect && cfg.enable && (running || testing)) {
        if (testing) {
            low = 120;
            high = 70;
        }
        // Centering follows physical steering, independent of game-axis inversion/curve.
        double physical = thrusty::steering(raw[cfg.axes[0]], cfg.lo, cfg.mid, cfg.hi, 0, 1, false);
        double velocity = motion.sample(physical, now / 1000.0);
        DICONSTANTFORCE cf{};
        cf.lMagnitude = thrusty::force(physical, low, high, now / 1000.0, cfg.gain, testing ? 0 : cfg.spring,
                                       cfg.reverse, cfg.reach, cfg.assist, velocity, cfg.boost);
        if (!testing)
            cf.lMagnitude = LONG(cf.lMagnitude * thrusty::clamp((now - forceStarted) / 500.0, 0, 1));
        DIEFFECT e{};
        e.dwSize = sizeof(e);
        e.dwDuration = 100000;
        e.cbTypeSpecificParams = sizeof(cf);
        e.lpvTypeSpecificParams = &cf;
        h = effect->SetParameters(&e, DIEP_TYPESPECIFICPARAMS | DIEP_DURATION | DIEP_START);
        if (FAILED(h)) {
            effect->Stop();
            effect->Release();
            effect = nullptr;
            feedbackReady = false;
            status(L"Wheel feedback failed; input bridge continues. " + hex(h));
        }
    } else if (effect)
        effect->Stop();
    static ULONGLONG lastUi = 0;
    if (now - lastUi < 80)
        return;
    lastUi = now;
    SendMessageW(meters[0], PBM_SETPOS, int((x + 1) * 500), 0);
    SendMessageW(meters[1], PBM_SETPOS, report.rt * 1000 / 255, 0);
    SendMessageW(meters[2], PBM_SETPOS, report.lt * 1000 / 255, 0);
    SendMessageW(meters[3], PBM_SETPOS, low * 1000 / 255, 0);
    SendMessageW(meters[4], PBM_SETPOS, high * 1000 / 255, 0);
    std::wstringstream line;
    line << L"Steering " << report.lx << L"     Throttle " << int(report.rt) << L"     Brake "
         << int(report.lt) << L"\r\nButtons pressed: ";
    bool any = false;
    for (int i = 0; i < 128; i++) {
        if (state.rgbButtons[i] & 0x80) {
            line << i + 1 << L" ";
            any = true;
        }
    }
    if (!any) {
        line << L"none";
    }
    line << L"     POV: "
         << ((state.rgdwPOV[0] & 0xffff) == 0xffff ? L"center" : std::to_wstring(state.rgdwPOV[0]));
    SetWindowTextW(liveText, line.str().c_str());
    std::wstringstream axes;
    for (int i = 0; i < 8; i++) {
        axes << axisNames[i] << L" = " << raw[i] << L"     ";
        if (i == 3)
            axes << L"\r\n";
    }
    SetWindowTextW(rawText, axes.str().c_str());
    std::wstringstream f;
    f << L"Wheel force output: " << (feedbackReady ? L"available" : L"off / unavailable")
      << L"\r\nRumble messages received: " << packets.load() << L"     Motors: " << low << L" / " << high
      << L"\r\n";
    if (!notified)
        f << L"Start bridge to listen for Xbox vibration.";
    else if (!last)
        f << L"Waiting for vibration from the game / GeForce NOW.";
    else if (stale)
        f << L"No fresh rumble for 5 seconds; vibration timed out.";
    else
        f << L"Last rumble message " << (now - last) << L" ms ago.";
    SetWindowTextW(feedbackText, f.str().c_str());
}
static void launch(const wchar_t *file, const wchar_t *args, const wchar_t *dir, int show) {
    auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(win, L"open", file, args, dir, show));
    if (result <= 32)
        error(L"Windows could not open the requested file or page. Error " + std::to_wstring(result));
}
static void buildUI() {
    font = CreateFontW(-S(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
    titleFont =
        CreateFontW(-S(30), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    smallFont =
        CreateFontW(-S(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
    tabs = control(WC_TABCONTROLW, L"", WS_TABSTOP, 16, 90, 750, 33, 10);
    for (const wchar_t *name : {L"Drive", L"Buttons & axes", L"Feedback & setup"}) {
        TCITEMW t{};
        t.mask = TCIF_TEXT;
        t.pszText = (LPWSTR)name;
        TabCtrl_InsertItem(tabs, TabCtrl_GetItemCount(tabs), &t);
    }
    label(L"THRUSTMASTER DEVICE", 32, 140, 300, 22, 0);
    deviceBox = combo(32, 169, 567, 100, 0);
    button(L"Refresh", 611, 168, 134, 26, 101, 0);
    control(L"BUTTON", L"Live controller output", BS_GROUPBOX, 30, 215, 716, 208, 0, 0);
    const wchar_t *meterNames[] = {L"Steering / left stick", L"Accelerator / RT", L"Brake / LT", L"Low motor",
                                   L"High motor"};
    for (int i = 0; i < 5; i++) {
        int p = i < 3 ? 0 : 2;
        int y = i < 3 ? 248 + i * 39 : 387 + (i - 3) * 33;
        label(meterNames[i], 48, y, 170, 20, p);
        meters[i] = control(PROGRESS_CLASSW, L"", PBS_SMOOTH, 220, y, 500, 22, 0, p);
        SendMessageW(meters[i], PBM_SETRANGE32, 0, 1000);
        SendMessageW(meters[i], PBM_SETBARCOLOR, 0, RGB(58, 140, 34));
    }
    liveText = control(L"STATIC", L"Waiting for wheel readings...", 0, 48, 369, 672, 46, 0, 0);
    control(L"BUTTON", L"Calibration", BS_GROUPBOX, 30, 440, 716, 128, 0, 0);
    label(L"Center the wheel and release both pedals before clicking Calibrate.\r\nThen sweep the wheel "
          L"fully both ways and press each pedal fully.",
          48, 467, 669, 42, 0);
    button(L"Calibrate...", 48, 523, 165, 27, 104, 0);
    button(L"Cancel calibration", 225, 523, 165, 27, 105, 0);
    label(L"Start Thrusty before launching your GeForce NOW game.\r\nUse the game's controller layout; the "
          L"game sees an Xbox 360 pad.",
          34, 589, 705, 45, 0);
    label(L"INPUT ASSIGNMENTS", 32, 140, 340, 20, 1);
    const wchar_t *names[] = {L"Steering", L"Accelerator", L"Brake"};
    for (int i = 0; i < 3; i++) {
        int x = 32 + i * 239;
        label(names[i], x, 174, 140, 20, 1);
        axisBoxes[i] = combo(x, 200, 131, 200 + i, 1);
        for (auto n : axisNames)
            item(axisBoxes[i], n);
        SendMessageW(axisBoxes[i], CB_SETCURSEL, cfg.axes[i], 0);
        inverseBoxes[i] = check(L"Invert", x + 139, 200, 86, 210 + i, 1, cfg.invert[i]);
    }
    rawText = control(L"STATIC", L"Raw axes appear here while connected.", 0, 32, 240, 714, 43, 0, 1);
    label(L"Deadzone % (0-25)", 32, 298, 165, 20, 1);
    deadEdit = edit(198, 295, 55, 220, 1, cfg.dead);
    label(L"Steering curve % (50-250; 100 = linear)", 280, 298, 340, 22, 1);
    curveEdit = edit(630, 295, 64, 221, 1, cfg.curve);
    label(L"BUTTON MAP  /  press a wheel button and read its number on Drive", 32, 344, 713, 22, 1);
    for (int i = 0; i < 12; i++) {
        int col = i / 6, row = i % 6, x = 32 + col * 363, y = 378 + row * 35;
        label(mapNames[i], x, y + 3, 175, 20, 1);
        mapBoxes[i] = combo(x + 179, y, 159, 300 + i, 1);
        item(mapBoxes[i], L"Unassigned");
        for (int n = 1; n <= 128; n++)
            item(mapBoxes[i], L"Wheel button " + std::to_wstring(n));
        SendMessageW(mapBoxes[i], CB_SETCURSEL, cfg.map[i], 0);
    }
    label(L"The first POV hat maps to the D-pad, including diagonals.\r\nAxis changes reset that axis "
          L"calibration. Stop the bridge, save, then calibrate.",
          32, 601, 710, 42, 1);
    feedbackCheck = check(L"Enable wheel feedback (start low)", 32, 143, 340, 400, 2, cfg.enable);
    assistCheck = check(L"Power-steering feel", 385, 143, 340, 413, 2, cfg.assist);
    label(L"Rumble strength % (0-35)", 32, 188, 230, 22, 2);
    gainEdit = edit(274, 184, 66, 401, 2, cfg.gain);
    label(L"Centering strength % (0-100)", 32, 230, 239, 22, 2);
    springEdit = edit(274, 226, 66, 402, 2, cfg.spring);
    label(L"Spring reach % (5-100)", 385, 188, 245, 22, 2);
    reachEdit = edit(650, 184, 66, 412, 2, cfg.reach);
    reverseCheck = check(L"Reverse force direction", 385, 228, 325, 403, 2, cfg.reverse);
    label(L"Near-center boost % (0-100)", 32, 274, 239, 22, 2);
    boostEdit = edit(274, 270, 66, 414, 2, cfg.boost);
    label(L"Try 40% for firmer small corrections.", 385, 274, 350, 22, 2);
    label(L"Boost firms the middle only; 0% retains the previous feel. It is not speed-dependent.", 32, 306,
          714, 20, 2);
    button(L"Test wheel pulse (1 second)", 32, 332, 249, 29, 404, 2);
    button(L"STOP FORCES", 295, 332, 155, 29, 405, 2);
    feedbackText =
        control(L"STATIC", L"Start bridge to listen for Xbox vibration.", 0, 32, 464, 710, 66, 0, 2);
    button(L"Install ViGEmBus...", 32, 543, 210, 30, 406, 2);
    button(L"Thrustmaster drivers", 255, 543, 221, 30, 407, 2);
    button(L"Windows controllers", 489, 543, 256, 30, 408, 2);
    button(L"Read setup guide", 32, 587, 210, 30, 409, 2);
    button(L"Open diagnostic log", 255, 587, 221, 30, 410, 2);
    button(L"Test Xbox rumble", 489, 587, 256, 30, 411, 2);
    label(L"Windows 10/11 x64  |  ViGEmBus is a retired third-party dependency.", 32, 630, 710, 21, 2);
    startButton = button(L"Start bridge", 20, 674, 170, 35, 500);
    button(L"Save settings", 202, 674, 150, 35, 501);
    button(L"STOP ALL", 614, 674, 150, 35, 502);
    label(L"Ctrl + Alt + F12 = stop all", 372, 684, 238, 22, -1);
    statusText = control(L"STATIC", L"Starting...", SS_SUNKEN, 16, 726, 750, 38, 0);
    pageTo(0);
}
static void cancelCalibration() {
    calibrating = false;
    SetWindowTextW(GetDlgItem(win, 104), L"Calibrate...");
}
static LRESULT CALLBACK wndproc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        win = w;
        buildUI();
        return 0;
    case WM_ERASEBKGND: {
        RECT r;
        GetClientRect(w, &r);
        FillRect((HDC)wp, &r, background);
        return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        SetBkColor((HDC)wp, RGB(236, 233, 216));
        SetTextColor((HDC)wp, RGB(20, 33, 56));
        return (LRESULT)background;
    }
    case WM_PAINT: {
        PAINTSTRUCT p;
        HDC dc = BeginPaint(w, &p);
        RECT r;
        GetClientRect(w, &r);
        TRIVERTEX a[2] = {{0, 0, 0x0800, 0x4200, 0xBB00, 0}, {r.right, S(76), 0x4200, 0x9700, 0xF100, 0}};
        GRADIENT_RECT gr = {0, 1};
        GradientFill(dc, a, 2, &gr, 1, GRADIENT_FILL_RECT_H);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 255, 255));
        SelectObject(dc, titleFont);
        TextOutW(dc, S(23), S(12), L"Thrusty", 7);
        SelectObject(dc, font);
        const wchar_t *sub = L"Wheel to Xbox 360  |  T300 RS control panel";
        TextOutW(dc, S(25), S(48), sub, lstrlenW(sub));
        SelectObject(dc, smallFont);
        TextOutW(dc, S(680), S(28), L"v" THRUSTY_VERSION_W, lstrlenW(L"v" THRUSTY_VERSION_W));
        EndPaint(w, &p);
        return 0;
    }
    case WM_NOTIFY:
        if (((NMHDR *)lp)->hwndFrom == tabs && ((NMHDR *)lp)->code == TCN_SELCHANGE) {
            pageTo(TabCtrl_GetCurSel(tabs));
            return 0;
        }
        break;
    case WM_HOTKEY:
        if (wp == 1) {
            stopBridge();
            cancelCalibration();
            status(L"Emergency stop: output and forces disabled.");
            return 0;
        }
        break;
    case WM_TIMER:
        if (uiSmoke && wp == 2) {
            DestroyWindow(w);
            return 0;
        }
        tick();
        return 0;
    case WM_POWERBROADCAST:
        if (wp == PBT_APMSUSPEND) {
            stopBridge();
            closeWheel();
            cancelCalibration();
            status(L"Suspended. Refresh and restart bridge after wake.");
        }
        return TRUE;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id == 100 && HIWORD(wp) == CBN_SELCHANGE) {
            cancelCalibration();
            stopBridge();
            if (acquireWheel(false))
                status(L"Wheel selected. Verify calibration for this device.");
            return 0;
        }
        if (HIWORD(wp) != BN_CLICKED)
            return 0;
        switch (id) {
        case 101:
            cancelCalibration();
            refresh();
            break;
        case 104:
            calibration();
            break;
        case 105:
            cancelCalibration();
            status(L"Calibration cancelled. Previous calibration retained.");
            break;
        case 500:
            if (running) {
                stopBridge();
                status(L"Bridge stopped.");
            } else
                start();
            break;
        case 501:
            if (running || calibrating) {
                error(L"Stop the bridge and finish calibration before saving settings.");
                break;
            }
            if (!readControls())
                break;
            stopForces();
            status(L"Settings saved. Start the bridge to apply wheel feedback.");
            break;
        case 502:
            stopBridge();
            cancelCalibration();
            status(L"Stopped all output and wheel forces.");
            break;
        case 404:
            if (running || calibrating) {
                error(L"Stop the bridge and finish calibration before testing.");
                break;
            }
            if (!readControls())
                break;
            if (!cfg.enable) {
                error(L"Check Enable wheel feedback before testing.");
                break;
            }
            if (acquireWheel(true)) {
                if (feedbackReady) {
                    testUntil = GetTickCount64() + 1000;
                    status(L"Playing a one-second wheel pulse.");
                } else
                    error(L"The driver did not provide constant-force output. Install the T300 driver and "
                          L"close other wheel apps.");
            }
            break;
        case 405:
            cfg.enable = 0;
            SendMessageW(feedbackCheck, BM_SETCHECK, BST_UNCHECKED, 0);
            stopForces();
            save();
            status(L"Wheel feedback disabled. Input bridge may continue.");
            break;
        case 406:
            stopBridge();
            closeWheel();
            cancelCalibration();
            launch((root + L"\\drivers\\ViGEmBus_1.22.0.exe").c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case 407:
            launch(L"https://support.thrustmaster.com/en/product/t300rs-en/", nullptr, nullptr,
                   SW_SHOWNORMAL);
            break;
        case 408:
            stopBridge();
            closeWheel();
            cancelCalibration();
            launch(L"control.exe", L"joy.cpl", nullptr, SW_SHOWNORMAL);
            status(L"Close Game Controllers, then Refresh to reconnect.");
            break;
        case 409:
            launch((root + L"\\README.html").c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case 411:
            testXbox();
            break;
        case 410:
            launch(L"notepad.exe", (L"\"" + logPath + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
            break;
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(w);
        return 0;
    case WM_DESTROY:
        KillTimer(w, timerId);
        stopBridge();
        closeWheel();
        if (di) {
            di->Release();
            di = nullptr;
        }
        if (v.dll)
            FreeLibrary(v.dll);
        if (xinput)
            FreeLibrary(xinput);
        if (hotkey)
            UnregisterHotKey(w, 1);
        timeEndPeriod(1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR commandLine, int show) {
    wchar_t executable[MAX_PATH];
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    wstring folder = executable;
    folder = folder.substr(0, folder.find_last_of(L"\\"));
    if (wstring(commandLine) == L"--self-test")
        return runNativeSmoke(instance, folder);
    uiSmoke = wstring(commandLine) == L"--ui-smoke";
    HANDLE singleton = CreateMutexW(nullptr, FALSE, L"Local\\Thrusty.ControlPanel");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Thrusty is already running.", L"Thrusty", MB_OK);
        if (singleton)
            CloseHandle(singleton);
        return 0;
    }
    SetProcessDPIAware();
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    HDC screen = GetDC(nullptr);
    double dpi = GetDeviceCaps(screen, LOGPIXELSX) / 96.0;
    ReleaseDC(nullptr, screen);
    uiScale = std::max(
        .6, std::min({dpi, (work.right - work.left - 32) / 800.0, (work.bottom - work.top - 48) / 780.0}));
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    root = path;
    root = root.substr(0, root.find_last_of(L"\\"));
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path);
    wstring data = wstring(path) + L"\\Thrusty";
    CreateDirectoryW(data.c_str(), nullptr);
    configPath = data + L"\\settings.ini";
    logPath = data + L"\\Thrusty.log";
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (GetFileAttributesExW(logPath.c_str(), GetFileExInfoStandard, &attr) &&
        (attr.nFileSizeHigh || attr.nFileSizeLow > 1048576))
        DeleteFileW(logPath.c_str());
    load();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX cc{sizeof(cc), ICC_TAB_CLASSES | ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&cc);
    background = CreateSolidBrush(RGB(236, 233, 216));
    WNDCLASSW wc{};
    wc.lpfnWndProc = wndproc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.lpszClassName = L"ThrustyWindow";
    wc.hbrBackground = background;
    RegisterClassW(&wc);
    RECT bounds{0, 0, S(782), S(780)};
    AdjustWindowRectEx(&bounds, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
    win = CreateWindowExW(0, wc.lpszClassName, L"Thrusty - T300 RS Control Panel",
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT,
                          CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top, nullptr,
                          nullptr, instance, nullptr);
    if (!win)
        return 1;
    ShowWindow(win, show);
    UpdateWindow(win);
    HRESULT h = DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8W, (void **)&di, nullptr);
    if (FAILED(h)) {
        startupExit = 16;
        if (!uiSmoke)
            error(L"DirectInput could not initialize: " + hex(h));
        DestroyWindow(win);
    } else {
        hotkey = !uiSmoke && RegisterHotKey(win, 1, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_F12);
        timeBeginPeriod(1);
        timerId = SetTimer(win, 1, 10, nullptr);
        if (!timerId) {
            startupExit = 17;
            if (!uiSmoke)
                error(L"Windows could not start the input timer.");
            DestroyWindow(win);
        } else if (uiSmoke) {
            if (!SetTimer(win, 2, 1500, nullptr)) {
                startupExit = 17;
                DestroyWindow(win);
            }
        } else
            refresh();
        if (!hotkey && !uiSmoke)
            error(L"Ctrl+Alt+F12 could not be registered. Use STOP ALL in this window.");
    }
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(win, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    DeleteObject(font);
    DeleteObject(titleFont);
    DeleteObject(smallFont);
    DeleteObject(background);
    CoUninitialize();
    if (singleton)
        CloseHandle(singleton);
    return startupExit;
}
