#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <string>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR arguments, int) {
    wchar_t path[32768]{};
    if (!GetModuleFileNameW(nullptr, path, 32768)) return 1;
    const auto root = std::filesystem::path(path).parent_path();
    std::filesystem::path game;
    // A packaged copy runs its adjacent game. In a checkout prefer the current
    // build over an older exported alpha in dist.
    for (const auto* relative : {
        L"isla_viva_native.exe",
        L"native/build/Release/isla_viva_native.exe",
        L"native/build/Debug/isla_viva_native.exe",
        L"native/build/isla_viva_native.exe",
        L"distribution/packages/current/isla_viva_native.exe"
    }) {
        const auto candidate = root / relative;
        if (std::filesystem::is_regular_file(candidate)) {
            game = candidate;
            break;
        }
    }
    if (game.empty()) {
        MessageBoxW(nullptr,
            L"No se encuentra la alpha. Extrae el paquete completo o compila el proyecto con tools\\run_native.ps1 -BuildOnly.",
            L"Isla Viva", MB_OK | MB_ICONERROR);
        return 1;
    }
    std::wstring command = L"\"" + game.wstring() + L"\" " + arguments;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(game.c_str(), command.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, game.parent_path().c_str(), &startup, &process)) {
        MessageBoxW(nullptr, L"No se pudo iniciar Isla Viva.", L"Isla Viva", MB_OK | MB_ICONERROR);
        return 1;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
}


