/*
 * g710.exe - makes the Logitech G710+ G and M keys usable without Logitech's software.
 *
 * Three sets of G-key assignments, chosen with M1, M2, M3; the chosen M key lights up.
 *   - A plain key (F13, A, 7...) goes into the keyboard's own G-key table (vendor feature report
 *     0x09), so the keyboard itself sends it. The table resets to 1-6 when the keyboard is plugged
 *     in, so this program writes it again on start, after a replug, after the PC wakes up, and
 *     checks it every 30 s.
 *   - A key with Ctrl/Shift/Alt/Win can't go in that table: its slot is left empty and this
 *     program sends the combination itself when it reads the G key (input report 0x03).
 * MR sends one fixed key, Ctrl+F19 by default.
 *
 * Protocol notes: PROTOCOL.md. How to build and use: README.md.
 */
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <wchar.h>
#include <stdio.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "setupapi")
#pragma comment(lib, "hid")
#pragma comment(lib, "advapi32")

#define APP_NAME L"G710 keys"
#define RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define WM_APP_TRAY (WM_APP + 1)
#define WM_APP_STATUS (WM_APP + 2) /* wParam: 1 keyboard found, 0 not found; lParam: active set 0-2 */
#define CMD_LOGON 1
#define CMD_OPEN_INI 2
#define CMD_QUIT 3
#define CHECK_MS 30000
#define RETRY_MS 2000

#define MOD_CTRL 1
#define MOD_SHIFT_ 2
#define MOD_ALT_ 4
#define MOD_WIN_ 8

/* One key: HID usage (for the keyboard's table), Windows virtual-key code and modifiers (for SendInput). */
typedef struct { BYTE usage; WORD vk; BYTE mods; } Key;

static const wchar_t *G_NAMES[6] = {L"G1", L"G2", L"G3", L"G4", L"G5", L"G6"};
static const wchar_t *SET_NAMES[3] = {L"M1", L"M2", L"M3"};
static const wchar_t *DEFAULTS[3][6] = {
    {L"F13", L"F14", L"F15", L"F16", L"F17", L"F18"},
    {L"F19", L"F20", L"F21", L"F22", L"F23", L"F24"},
    {L"Ctrl+F13", L"Ctrl+F14", L"Ctrl+F15", L"Ctrl+F16", L"Ctrl+F17", L"Ctrl+F18"},
};
static const wchar_t *MR_DEFAULT = L"Ctrl+F19";

static Key g_sets[3][6], g_mr;
static volatile LONG g_set; /* 0-2: M1-M3 */
static wchar_t g_exe[MAX_PATH], g_ini[MAX_PATH];
static HWND g_wnd;
static HANDLE g_quit, g_recheck;
static UINT g_taskbar_created;
static NOTIFYICONDATAW g_nid;
static BOOL g_found;

/* Accepts F1-F24, A-Z, 0-9 or "none", optionally after Ctrl+, Shift+, Alt+, Win+. */
static BOOL ParseKey(const wchar_t *text, Key *k)
{
    wchar_t buf[64];
    wcsncpy_s(buf, 64, text, _TRUNCATE);
    k->usage = 0; k->vk = 0; k->mods = 0;
    wchar_t *ctx = NULL, *last = NULL;
    for (wchar_t *t = wcstok_s(buf, L"+", &ctx); t; t = wcstok_s(NULL, L"+", &ctx)) {
        while (*t == L' ') t++;
        for (size_t n = wcslen(t); n && t[n - 1] == L' '; n--) t[n - 1] = 0;
        if (last) {
            if (!_wcsicmp(last, L"Ctrl") || !_wcsicmp(last, L"Control")) k->mods |= MOD_CTRL;
            else if (!_wcsicmp(last, L"Shift")) k->mods |= MOD_SHIFT_;
            else if (!_wcsicmp(last, L"Alt")) k->mods |= MOD_ALT_;
            else if (!_wcsicmp(last, L"Win")) k->mods |= MOD_WIN_;
            else return FALSE;
        }
        last = t;
    }
    if (!last || !*last || !_wcsicmp(last, L"none")) { k->mods = 0; return TRUE; }
    if ((last[0] == L'F' || last[0] == L'f') && last[1]) {
        wchar_t *end;
        long n = wcstol(last + 1, &end, 10);
        if (*end || n < 1 || n > 24) return FALSE;
        k->usage = (BYTE)(n <= 12 ? 0x3A + n - 1 : 0x68 + n - 13);
        k->vk = (WORD)(VK_F1 + n - 1);
        return TRUE;
    }
    if (last[1]) return FALSE;
    wchar_t c = towupper(last[0]);
    if (c >= L'A' && c <= L'Z') { k->usage = (BYTE)(0x04 + c - L'A'); k->vk = (WORD)c; return TRUE; }
    if (c >= L'1' && c <= L'9') { k->usage = (BYTE)(0x1E + c - L'1'); k->vk = (WORD)c; return TRUE; }
    if (c == L'0') { k->usage = 0x27; k->vk = L'0'; return TRUE; }
    return FALSE;
}

static void LoadKeys(void)
{
    wchar_t buf[64];
    for (int s = 0; s < 3; s++)
        for (int i = 0; i < 6; i++) {
            GetPrivateProfileStringW(SET_NAMES[s], G_NAMES[i], DEFAULTS[s][i], buf, 64, g_ini);
            if (!ParseKey(buf, &g_sets[s][i])) ParseKey(DEFAULTS[s][i], &g_sets[s][i]);
        }
    GetPrivateProfileStringW(L"MR", L"key", MR_DEFAULT, buf, 64, g_ini);
    if (!ParseKey(buf, &g_mr)) ParseKey(MR_DEFAULT, &g_mr);
}

static void SaveSet(int set)
{
    wchar_t v[4];
    swprintf(v, 4, L"%d", set + 1);
    WritePrivateProfileStringW(L"state", L"set", v, g_ini);
}

/* What the keyboard's own table should hold for this key: only plain keys fit. */
static BYTE TableUsage(const Key *k) { return k->mods ? 0 : k->usage; }

/* Opens the G710+ vendor collection (interface 1, usage page 0xFF00). Returns INVALID_HANDLE_VALUE if absent. */
static HANDLE OpenVendor(USHORT *in_len, USHORT *feat_len)
{
    GUID guid;
    HidD_GetHidGuid(&guid);
    HDEVINFO devs = SetupDiGetClassDevsW(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
    HANDLE found = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA ifd = {sizeof ifd};
    for (DWORD i = 0; found == INVALID_HANDLE_VALUE && SetupDiEnumDeviceInterfaces(devs, NULL, &guid, i, &ifd); i++) {
        DWORD need = 0;
        SetupDiGetDeviceInterfaceDetailW(devs, &ifd, NULL, 0, &need, NULL);
        SP_DEVICE_INTERFACE_DETAIL_DATA_W *det = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)HeapAlloc(GetProcessHeap(), 0, need);
        if (!det) continue;
        det->cbSize = sizeof *det;
        if (SetupDiGetDeviceInterfaceDetailW(devs, &ifd, det, need, NULL, NULL)) {
            wchar_t low[512];
            wcsncpy_s(low, 512, det->DevicePath, _TRUNCATE);
            _wcslwr_s(low, 512);
            if (wcsstr(low, L"vid_046d&pid_c24d") && wcsstr(low, L"mi_01")) {
                HANDLE h = CreateFileW(det->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                if (h != INVALID_HANDLE_VALUE) {
                    PHIDP_PREPARSED_DATA pp;
                    HIDP_CAPS caps;
                    BOOL ok = FALSE;
                    if (HidD_GetPreparsedData(h, &pp)) {
                        if (HidP_GetCaps(pp, &caps) == HIDP_STATUS_SUCCESS && caps.UsagePage == 0xFF00) {
                            *in_len = caps.InputReportByteLength;
                            *feat_len = caps.FeatureReportByteLength;
                            ok = TRUE;
                        }
                        HidD_FreePreparsedData(pp);
                    }
                    if (ok) found = h; else CloseHandle(h);
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, det);
    }
    SetupDiDestroyDeviceInfoList(devs);
    return found;
}

/* Brings the keyboard in line with the active set: its G-key table (report 0x09) and M lights (report 0x06,
 * 0x10 M1, 0x20 M2, 0x40 M3; 0x80 would be MR). Writes only what differs. */
static void ApplyDevice(HANDLE h, USHORT feat_len)
{
    BYTE buf[64];
    if (feat_len < 7 || feat_len > sizeof buf) return;
    int set = (int)g_set;

    ZeroMemory(buf, sizeof buf);
    buf[0] = 0x09;
    if (HidD_GetFeature(h, buf, feat_len)) {
        BOOL same = TRUE;
        for (int i = 0; i < 6; i++) {
            BYTE want = TableUsage(&g_sets[set][i]);
            if (buf[1 + i] != want) same = FALSE;
            buf[1 + i] = want;
        }
        if (!same) HidD_SetFeature(h, buf, feat_len);
    }

    BYTE leds = (BYTE)(0x10 << set);
    ZeroMemory(buf, sizeof buf);
    buf[0] = 0x06;
    if (!HidD_GetFeature(h, buf, feat_len) || buf[1] != leds) {
        ZeroMemory(buf, sizeof buf);
        buf[0] = 0x06;
        buf[1] = leds;
        HidD_SetFeature(h, buf, feat_len);
    }
}

/* Presses (up = FALSE) or releases a key with its modifiers in one SendInput call. */
static void SendCombo(const Key *k, BOOL up)
{
    static const struct { BYTE mod; WORD vk; } MODS[4] = {
        {MOD_CTRL, VK_CONTROL}, {MOD_SHIFT_, VK_SHIFT}, {MOD_ALT_, VK_MENU}, {MOD_WIN_, VK_LWIN}};
    if (!k->vk) return;
    INPUT in[5];
    int n = 0;
    WORD order[5];
    for (int i = 0; i < 4; i++)
        if (k->mods & MODS[i].mod) order[n++] = MODS[i].vk;
    order[n++] = k->vk;
    for (int i = 0; i < n; i++) {
        WORD vk = up ? order[n - 1 - i] : order[i];
        ZeroMemory(&in[i], sizeof in[i]);
        in[i].type = INPUT_KEYBOARD;
        in[i].ki.wVk = vk;
        in[i].ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        in[i].ki.dwFlags = (up ? KEYEVENTF_KEYUP : 0) | (vk == VK_LWIN ? KEYEVENTF_EXTENDEDKEY : 0);
    }
    SendInput((UINT)n, in, sizeof in[0]);
}

static DWORD WINAPI ReaderThread(LPVOID unused)
{
    (void)unused;
    while (WaitForSingleObject(g_quit, 0) != WAIT_OBJECT_0) {
        USHORT in_len = 0, feat_len = 0;
        HANDLE h = OpenVendor(&in_len, &feat_len);
        if (h == INVALID_HANDLE_VALUE) {
            PostMessageW(g_wnd, WM_APP_STATUS, 0, g_set);
            WaitForSingleObject(g_quit, RETRY_MS);
            continue;
        }
        LoadKeys();
        ApplyDevice(h, feat_len);
        PostMessageW(g_wnd, WM_APP_STATUS, 1, g_set);

        OVERLAPPED ov = {0};
        ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        BYTE buf[64], g_prev = 0, m_prev = 0;
        Key g_down[6] = {0}; /* combinations this program pressed for each G key, to release the same ones */
        BOOL mr_down = FALSE, quit = FALSE;
        while (!quit && in_len >= 3 && in_len <= sizeof buf) {
            DWORD n = 0;
            ResetEvent(ov.hEvent);
            if (!ReadFile(h, buf, in_len, NULL, &ov) && GetLastError() != ERROR_IO_PENDING) break;
            HANDLE waits[3] = {ov.hEvent, g_quit, g_recheck};
            DWORD r = WaitForMultipleObjects(3, waits, FALSE, CHECK_MS);
            if (r != WAIT_OBJECT_0) CancelIoEx(h, &ov);
            BOOL got = GetOverlappedResult(h, &ov, &n, TRUE);
            if (!got && GetLastError() != ERROR_OPERATION_ABORTED) break; /* keyboard unplugged */
            if (got && n >= 3 && buf[0] == 0x03) {
                BYTE g = buf[1] & 0x3F, m = buf[2] & 0xF0;
                for (int i = 0; i < 6; i++) {
                    BYTE bit = (BYTE)(1 << i);
                    if ((g & bit) && !(g_prev & bit)) {
                        const Key *k = &g_sets[g_set][i];
                        if (k->mods && k->vk) { g_down[i] = *k; SendCombo(k, FALSE); }
                    } else if (!(g & bit) && (g_prev & bit) && g_down[i].vk) {
                        SendCombo(&g_down[i], TRUE);
                        g_down[i].vk = 0;
                    }
                }
                for (int s = 0; s < 3; s++) {
                    BYTE bit = (BYTE)(0x10 << s);
                    if ((m & bit) && !(m_prev & bit) && g_set != s) {
                        InterlockedExchange(&g_set, s);
                        SaveSet(s);
                        PostMessageW(g_wnd, WM_APP_STATUS, 1, s);
                    }
                }
                BOOL mr = (m & 0x80) != 0;
                if (mr != mr_down) { SendCombo(&g_mr, !mr); mr_down = mr; }
                if (m != m_prev) ApplyDevice(h, feat_len);
                g_prev = g;
                m_prev = m;
            }
            if (r == WAIT_OBJECT_0 + 1) quit = TRUE;
            else if (r != WAIT_OBJECT_0) { LoadKeys(); ApplyDevice(h, feat_len); } /* recheck or 30 s timeout */
        }
        for (int i = 0; i < 6; i++)
            if (g_down[i].vk) SendCombo(&g_down[i], TRUE);
        if (mr_down) SendCombo(&g_mr, TRUE);
        CloseHandle(ov.hEvent);
        CloseHandle(h);
        if (quit) break;
    }
    return 0;
}

static BOOL LogonEnabled(void)
{
    wchar_t val[MAX_PATH + 4];
    DWORD size = sizeof val;
    return RegGetValueW(HKEY_CURRENT_USER, RUN_KEY, APP_NAME, RRF_RT_REG_SZ, NULL, val, &size) == ERROR_SUCCESS;
}

static void SetLogon(BOOL on)
{
    if (on) {
        wchar_t val[MAX_PATH + 4];
        swprintf(val, MAX_PATH + 4, L"\"%s\"", g_exe);
        RegSetKeyValueW(HKEY_CURRENT_USER, RUN_KEY, APP_NAME, REG_SZ, val, (DWORD)((wcslen(val) + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteKeyValueW(HKEY_CURRENT_USER, RUN_KEY, APP_NAME);
    }
}

/* A small blue "G" for the tray. */
static HICON MakeIcon(void)
{
    int cx = GetSystemMetrics(SM_CXSMICON), cy = GetSystemMetrics(SM_CYSMICON);
    HDC screen = GetDC(NULL), dc = CreateCompatibleDC(screen);
    HBITMAP color = CreateCompatibleBitmap(screen, cx, cy), mask = CreateBitmap(cx, cy, 1, 1, NULL);
    HGDIOBJ old = SelectObject(dc, color);
    RECT r = {0, 0, cx, cy};
    HBRUSH bg = CreateSolidBrush(RGB(0, 103, 192));
    FillRect(dc, &r, bg);
    DeleteObject(bg);
    HFONT font = CreateFontW(-cy, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HGDIOBJ old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    DrawTextW(dc, L"G", 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_font);
    DeleteObject(font);
    SelectObject(dc, mask);
    PatBlt(dc, 0, 0, cx, cy, BLACKNESS);
    SelectObject(dc, old);
    ICONINFO ii = {TRUE, 0, 0, mask, color};
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(color);
    DeleteObject(mask);
    DeleteDC(dc);
    ReleaseDC(NULL, screen);
    return icon;
}

static void TrayUpdate(DWORD op)
{
    if (g_found)
        swprintf(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"%s: set M%d active", APP_NAME, (int)g_set + 1);
    else
        swprintf(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"%s: G710+ not found", APP_NAME);
    Shell_NotifyIconW(op, &g_nid);
}

static void ShowMenu(void)
{
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (LogonEnabled() ? MF_CHECKED : 0), CMD_LOGON, L"Start at logon");
    AppendMenuW(menu, MF_STRING, CMD_OPEN_INI, L"Open settings file");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, CMD_QUIT, L"Quit");
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_wnd);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_wnd, NULL);
    DestroyMenu(menu);
    if (cmd == CMD_LOGON) SetLogon(!LogonEnabled());
    else if (cmd == CMD_OPEN_INI) ShellExecuteW(NULL, L"open", L"notepad.exe", g_ini, NULL, SW_SHOWNORMAL);
    else if (cmd == CMD_QUIT) DestroyWindow(g_wnd);
}

static LRESULT CALLBACK WndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_APP_TRAY:
        if (lp == WM_RBUTTONUP || lp == WM_LBUTTONUP) ShowMenu();
        return 0;
    case WM_APP_STATUS:
        g_found = (BOOL)wp;
        TrayUpdate(NIM_MODIFY);
        return 0;
    case WM_POWERBROADCAST:
        if (wp == PBT_APMRESUMEAUTOMATIC || wp == PBT_APMRESUMESUSPEND) SetEvent(g_recheck);
        return TRUE;
    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    if (g_taskbar_created && msg == g_taskbar_created) { TrayUpdate(NIM_ADD); return 0; }
    return DefWindowProcW(w, msg, wp, lp);
}

/* Settings file: g710.ini next to the exe if one is there (portable use), otherwise %APPDATA%\G710 keys\g710.ini. */
static void FindIni(void)
{
    wcscpy_s(g_ini, MAX_PATH, g_exe);
    wchar_t *slash = wcsrchr(g_ini, L'\\');
    if (slash) wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - g_ini), L"g710.ini");
    if (GetFileAttributesW(g_ini) != INVALID_FILE_ATTRIBUTES) return;
    wchar_t dir[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dir)) &&
        wcscat_s(dir, MAX_PATH, L"\\G710 keys") == 0) {
        CreateDirectoryW(dir, NULL);
        swprintf(g_ini, MAX_PATH, L"%s\\g710.ini", dir);
    }
}

/* Logitech Gaming Software writes its own G-key table and would undo ours: say so in the tray.
 * G Hub isn't listed: it doesn't support the G710+ as far as known (not tested). */
static void WarnIfLogitechRuns(void)
{
    static const wchar_t *NAMES[] = {L"LCore.exe"};
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe = {sizeof pe};
    const wchar_t *found = NULL;
    for (BOOL ok = Process32FirstW(snap, &pe); ok && !found; ok = Process32NextW(snap, &pe))
        for (int i = 0; i < ARRAYSIZE(NAMES); i++)
            if (!_wcsicmp(pe.szExeFile, NAMES[i])) found = NAMES[i];
    CloseHandle(snap);
    if (!found) return;
    g_nid.uFlags |= NIF_INFO;
    g_nid.dwInfoFlags = NIIF_WARNING;
    wcscpy_s(g_nid.szInfoTitle, ARRAYSIZE(g_nid.szInfoTitle), L"Logitech software is running");
    swprintf(g_nid.szInfo, ARRAYSIZE(g_nid.szInfo),
             L"%s changes the G keys too, so the two will undo each other. Quit it, or stop it starting with Windows.", found);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags &= ~NIF_INFO;
}

/* Returns TRUE when it created the file, i.e. on first run. */
static BOOL WriteDefaultIni(void)
{
    if (GetFileAttributesW(g_ini) != INVALID_FILE_ATTRIBUTES) return FALSE;
    static const char text[] =
        "; G710 keys settings. Saved changes apply within 30 seconds, or on the next M key press.\r\n"
        "; A key is F1-F24, A-Z, 0-9 or none, optionally after Ctrl+, Shift+, Alt+, Win+ (Ctrl+F13).\r\n"
        "; Plain keys are sent by the keyboard itself; keys with Ctrl/Shift/Alt/Win by g710.exe.\r\n"
        "[M1]\r\nG1=F13\r\nG2=F14\r\nG3=F15\r\nG4=F16\r\nG5=F17\r\nG6=F18\r\n"
        "[M2]\r\nG1=F19\r\nG2=F20\r\nG3=F21\r\nG4=F22\r\nG5=F23\r\nG6=F24\r\n"
        "[M3]\r\nG1=Ctrl+F13\r\nG2=Ctrl+F14\r\nG3=Ctrl+F15\r\nG4=Ctrl+F16\r\nG5=Ctrl+F17\r\nG6=Ctrl+F18\r\n"
        "[MR]\r\nkey=Ctrl+F19\r\n"
        "; The active set, remembered by g710.exe.\r\n[state]\r\nset=1\r\n";
    HANDLE f = CreateFileW(g_ini, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return FALSE;
    DWORD written;
    WriteFile(f, text, (DWORD)(sizeof text - 1), &written, NULL);
    CloseHandle(f);
    return TRUE;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show)
{
    (void)prev; (void)cmd; (void)show;
    HANDLE single = CreateMutexW(NULL, FALSE, L"Local\\G710KeysListener");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"G710 keys is already running (blue G in the tray).", APP_NAME, MB_ICONINFORMATION);
        return 0;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    GetModuleFileNameW(NULL, g_exe, MAX_PATH);
    FindIni();
    BOOL first_run = WriteDefaultIni();
    if (LogonEnabled()) SetLogon(TRUE); /* keeps the logon entry pointing here if the exe was moved */
    LoadKeys();
    int set = (int)GetPrivateProfileIntW(L"state", L"set", 1, g_ini) - 1;
    g_set = (set >= 0 && set <= 2) ? set : 0;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"G710KeysWindow";
    RegisterClassW(&wc);
    /* A hidden top-level window: message-only windows don't get WM_POWERBROADCAST or TaskbarCreated. */
    g_wnd = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, APP_NAME, WS_POPUP, 0, 0, 0, 0, NULL, NULL, inst, NULL);
    g_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");
    ChangeWindowMessageFilterEx(g_wnd, g_taskbar_created, MSGFLT_ALLOW, NULL);

    g_nid.cbSize = sizeof g_nid;
    g_nid.hWnd = g_wnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP_TRAY;
    g_nid.hIcon = MakeIcon();
    TrayUpdate(NIM_ADD);

    g_quit = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_recheck = CreateEventW(NULL, FALSE, FALSE, NULL);
    HANDLE thread = CreateThread(NULL, 64 * 1024, ReaderThread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

    if (first_run &&
        MessageBoxW(NULL,
                    L"G710 keys is running: it's the blue G in the tray, where you can change the keys or quit.\n\n"
                    L"Start it automatically when you log on to Windows?",
                    APP_NAME, MB_YESNO | MB_ICONQUESTION) == IDYES)
        SetLogon(TRUE);
    WarnIfLogitechRuns();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    SetEvent(g_quit);
    WaitForSingleObject(thread, 5000);
    CloseHandle(single);
    return 0;
}
