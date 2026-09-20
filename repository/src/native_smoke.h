#pragma once
// Hardware-free native smoke test for Windows CI. It does not connect a target,
// install a driver, acquire a wheel, or send force feedback.
inline int runNativeSmoke(HINSTANCE instance, const std::wstring &folder) {
    struct ComScope {
        HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ~ComScope() {
            if (SUCCEEDED(result))
                CoUninitialize();
        }
    } com;
    HMODULE client = LoadLibraryExW((folder + L"\\ViGEmClient.dll").c_str(), nullptr,
                                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!client)
        return 10;
    const char *symbols[] = {"vigem_alloc",
                             "vigem_free",
                             "vigem_connect",
                             "vigem_disconnect",
                             "vigem_target_x360_alloc",
                             "vigem_target_free",
                             "vigem_target_add",
                             "vigem_target_remove",
                             "vigem_target_x360_update",
                             "vigem_target_x360_register_notification",
                             "vigem_target_x360_unregister_notification",
                             "vigem_target_x360_get_user_index"};
    for (auto name : symbols) {
        if (!GetProcAddress(client, name)) {
            FreeLibrary(client);
            return 11;
        }
    }
    auto alloc = reinterpret_cast<void *(__cdecl *)()>(GetProcAddress(client, "vigem_alloc"));
    auto release = reinterpret_cast<void(__cdecl *)(void *)>(GetProcAddress(client, "vigem_free"));
    auto targetAlloc =
        reinterpret_cast<void *(__cdecl *)()>(GetProcAddress(client, "vigem_target_x360_alloc"));
    auto targetRelease =
        reinterpret_cast<void(__cdecl *)(void *)>(GetProcAddress(client, "vigem_target_free"));
    void *c = alloc();
    void *t = targetAlloc();
    bool memoryOk = c && t;
    if (t)
        targetRelease(t);
    if (c)
        release(c);
    FreeLibrary(client);
    if (!memoryOk)
        return 12;
    IDirectInput8W *input = nullptr;
    HRESULT result = DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8W,
                                        reinterpret_cast<void **>(&input), nullptr);
    if (FAILED(result))
        return 13;
    input->Release();
    HMODULE x = LoadLibraryExW(L"xinput1_4.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!x)
        return 14;
    bool xOk = GetProcAddress(x, "XInputSetState") != nullptr;
    FreeLibrary(x);
    return xOk ? 0 : 15;
}
