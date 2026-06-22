#include "LinuxMLO.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>

namespace Parless
{
	using namespace std;

    using wine_get_unix_file_name_t = char* (CDECL*)(LPCWSTR);

	void HandleLinuxMLO()
	{
		// Current working directory
        string shimPath(MAX_PATH, '\0');

        const DWORD shimPathLength = GetModuleFileNameA(nullptr, shimPath.data(), static_cast<DWORD>(shimPath.size()));

        shimPath.resize(shimPathLength);

        string exeDir = shimPath.substr(0, shimPath.find_last_of("\\/"));
        string cmd = exeDir + "\\ShinRyuModManager-CE";
        string unixCmd = ToUnixViaWine(cmd);
        string unixDir = unixCmd.substr(0, unixCmd.find_last_of('/'));
        string scriptWin = exeDir + "\\.srmm-run.sh";
        string scriptUnix = unixDir + "/.srmm-run.sh";
        string doneWin = exeDir + "\\.srmm-done";

        DeleteFileA(doneWin.c_str());

        // Creates script for running SRMM-CE properly
        {
            ofstream s(scriptWin, std::ios::binary);
            s << "#!/bin/sh\n";
            s << "cd \"" << unixDir << "\" || exit 91\n";
            s << "unset LD_PRELOAD LD_LIBRARY_PATH LD_AUDIT\n";
            s << "export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1\n";
            s << "./ShinRyuModManager-CE -s\n";
            s << "ec=$?\n";
            s << "echo \"$ec\" > .srmm-done\n";
            s << "exit $ec\n";
        }

        // Ensures the script is runnable
        {
            string chmodCmd = R"(start.exe /wait /unix /bin/chmod +x ")" + scriptUnix + "\"";
            vector b(chmodCmd.begin(), chmodCmd.end());

            b.push_back('\0');

            STARTUPINFOA si2 = { sizeof(si2) };
            PROCESS_INFORMATION pi2 = {};

            if (CreateProcessA(nullptr, b.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si2, &pi2)) {
                WaitForSingleObject(pi2.hProcess, INFINITE);
                CloseHandle(pi2.hThread);
                CloseHandle(pi2.hProcess);
            }
        }

        string cmdLine = R"(start.exe /wait /unix ")" + scriptUnix + "\"";

        SetCurrentDirectoryA(exeDir.c_str());

        vector buf(cmdLine.begin(), cmdLine.end());

        buf.push_back('\0');

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr,
            FALSE, 0, nullptr, exeDir.c_str(), &si, &pi)) {
            return;
        }

        // `start.exe /unix` returns almost immediately, so we wait for the "done" file below
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        // Waits for the "done" file to be created
        const DWORD startTick = GetTickCount();
        constexpr  DWORD timeoutMs = 10 * 60 * 1000; // 10 minutes hard limit

        while (GetTickCount() - startTick <= timeoutMs) {
            if (DWORD attrs = GetFileAttributesA(doneWin.c_str());
                attrs != INVALID_FILE_ATTRIBUTES) {
                ifstream f(doneWin);

                if (int ec = -1; f >> ec) {
                    break;
                }
            }

            Sleep(100);
        }

        // Clean up
        DeleteFileA(doneWin.c_str());
        DeleteFileA(scriptUnix.c_str());
	}

	string ToUnixViaWine(const string& win_path)
	{
        const HMODULE k32 = GetModuleHandleA("kernel32.dll");
        const auto fn = reinterpret_cast<wine_get_unix_file_name_t>(GetProcAddress(k32, "wine_get_unix_file_name"));

        if (!fn) {
            return {};
        }

        const int wlen = MultiByteToWideChar(CP_UTF8, 0, win_path.c_str(), -1, nullptr, 0);
        wstring w(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, win_path.c_str(), -1, w.data(), wlen);

        char* p = fn(w.c_str());

        if (!p)
            return {};

        string result = p;

        HeapFree(GetProcessHeap(), 0, p);

        return result;
	}
}
