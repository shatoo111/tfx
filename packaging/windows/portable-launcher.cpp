#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

static DWORD run(const fs::path& program, const std::wstring& arguments,
                 const fs::path& directory, DWORD flags) {
    std::wstring command = L"\"" + program.wstring() + L"\" " + arguments;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(program.c_str(), command.data(), nullptr, nullptr, FALSE,
                        flags, nullptr, directory.c_str(), &startup, &process)) {
        throw std::system_error(GetLastError(), std::system_category());
    }
    CloseHandle(process.hThread);
    if (!flags) AllowSetForegroundWindow(process.dwProcessId);
    DWORD result = 1;
    WaitForSingleObject(process.hProcess, INFINITE);
    GetExitCodeProcess(process.hProcess, &result);
    CloseHandle(process.hProcess);
    return result;
}

static fs::path localAppData() {
    PWSTR directory = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &directory))) {
        throw std::runtime_error("LocalAppData is unavailable.");
    }
    fs::path path(directory);
    CoTaskMemFree(directory);
    return path;
}

static fs::path systemTar() {
    wchar_t directory[MAX_PATH];
    const UINT length = GetSystemDirectoryW(directory, MAX_PATH);
    if (!length || length >= MAX_PATH) {
        throw std::runtime_error("The Windows system directory is unavailable.");
    }
    return fs::path(directory) / L"tar.exe";
}

static void writeArchive(const fs::path& archive) {
    const HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(101), RT_RCDATA);
    if (!resource) throw std::runtime_error("Embedded archive not found.");
    const DWORD size = SizeofResource(nullptr, resource);
    const void* bytes = LockResource(LoadResource(nullptr, resource));
    if (!size || !bytes) throw std::runtime_error("Embedded archive is unavailable.");
    const HANDLE file = CreateFileW(archive.c_str(), GENERIC_WRITE, 0, nullptr,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::system_error(GetLastError(), std::system_category());
    }
    DWORD written = 0;
    const BOOL success = WriteFile(file, bytes, size, &written, nullptr);
    CloseHandle(file);
    if (!success || written != size) throw std::runtime_error("Could not save the archive.");
}

static void extract(const fs::path& root, const fs::path& install) {
    fs::create_directories(root);
    GUID guid{};
    wchar_t name[40]{};
    if (FAILED(CoCreateGuid(&guid)) || !StringFromGUID2(guid, name, 40)) {
        throw std::runtime_error("Could not create a staging directory name.");
    }
    const fs::path stage = root / name;
    if (!fs::create_directory(stage)) throw std::runtime_error("Staging directory already exists.");
    struct StagingDirectory {
        const fs::path& path;
        ~StagingDirectory() {
            std::error_code ignored;
            fs::remove_all(path, ignored);
        }
    } cleanup{stage};

    const fs::path archive = stage / L"payload.tar.gz";
    const fs::path unpacked = stage / L"app";
    fs::create_directory(unpacked);
    writeArchive(archive);
    if (run(systemTar(), L"-xf \"" + archive.wstring() + L"\"", unpacked,
            CREATE_NO_WINDOW) != 0 ||
        !fs::is_regular_file(unpacked / L"bin" / L"tfx.exe")) {
        throw std::runtime_error("Could not extract the application.");
    }
    std::error_code error;
    fs::rename(unpacked, install, error);
    if (error && !fs::is_regular_file(install / L"bin" / L"tfx.exe")) {
        throw std::system_error(error);
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR arguments, int) {
    try {
        const fs::path root = localAppData() / L"TelegramEffectsPortable";
        const fs::path install = root / PAYLOAD_VERSION;
        const fs::path program = install / L"bin" / L"tfx.exe";
        if (!fs::is_regular_file(program)) extract(root, install);
        return static_cast<int>(run(program, arguments, program.parent_path(), 0));
    } catch (const std::exception& error) {
        MessageBoxA(nullptr, error.what(), "Telegram Effects", MB_OK | MB_ICONERROR);
        return 1;
    }
}
