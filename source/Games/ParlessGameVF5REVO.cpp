#include "ParlessGameVF5REVO.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <thread>
#include "StringHelpers.h"

using namespace hook;
using namespace Memory;

ParlessGameVF5REVO::t_orgVF5REVOAddFileEntry ParlessGameVF5REVO::orgVF5REVOAddFileEntry = NULL;
ParlessGameVF5REVO::t_orgVF5REVOAddFileEntry* ParlessGameVF5REVO::hookVF5REVOAddFileEntry = NULL;
ParlessGameVF5REVO::t_orgVF5REVOROMLoadFile ParlessGameVF5REVO::orgVF5REVOROMAddFileEntry = NULL;
ParlessGameVF5REVO::t_orgVF5REVOROMLoadStreamFile ParlessGameVF5REVO::orgVF5REVOROMLoadStreamFile = NULL;

inline std::uint8_t* PatternScan(void* module, const char* signature)
{
    static auto pattern_to_byte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char*>(pattern);
        auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
        };

    auto dosHeader = (PIMAGE_DOS_HEADER)module;
    auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

    auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    auto patternBytes = pattern_to_byte(signature);
    auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

    auto s = patternBytes.size();
    auto d = patternBytes.data();

    for (auto i = 0ul; i < sizeOfImage - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (scanBytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            return &scanBytes[i];
        }
    }
    return nullptr;
}

bool vf5fs_initialization_wait()
{
	void* moduleAddr = nullptr;
	while (true)
	{
		moduleAddr = GetModuleHandle(L"vf5fs-pxd-w64-d3d12_SteamRetail");

		if (moduleAddr != nullptr)
			break;
	}

	std::cout << "VF5FS Module loaded, hooking..." << std::endl;

	void* fileLoadAddr = PatternScan(moduleAddr, "48 83 EC ? 45 8B C8 C6 44 24 28 ?");
	void* musicPlayFunc = PatternScan(moduleAddr, "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 41 56 48 83 EC ? 45 33 F6");

    if (MH_CreateHook(fileLoadAddr, &ParlessGameVF5REVO::VF5RevoROMAddFileEntry, reinterpret_cast<LPVOID*>(&ParlessGameVF5REVO::orgVF5REVOROMAddFileEntry)) != MH_OK)
    {
        std::cout << "Hook creation failed. Aborting.\n";
        return false;
    }

    if (MH_EnableHook(fileLoadAddr) != MH_OK)
    {
        std::cout << "Hook could not be enabled. Aborting.\n";
        return false;
    }

    std::cout << "Initialized ROM file direction hook.\n";

	if (MH_CreateHook(musicPlayFunc, &ParlessGameVF5REVO::VF5RevoROMLoadStreamFile, reinterpret_cast<LPVOID*>(&ParlessGameVF5REVO::orgVF5REVOROMLoadStreamFile)) != MH_OK)
	{
		std::cout << "Music hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(musicPlayFunc) != MH_OK)
	{
		std::cout << "Music hook could not be enabled. Aborting.\n";
		return false;
	}

	std::cout << "Initialized ROM stream file direction hook.\n";
}

bool ParlessGameVF5REVO::hook_add_file()
{
	void* renameFilePathsFunc;
	void* musicPlayFunc;

	renameFilePathsFunc = get_pattern("48 8B C4 4C 89 48 20 89 50 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 68 FE FF FF", 0);

	Sleep(5000);

	//Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

	//ReadCall(renameFilePathsFunc, orgVF5REVOAddFileEntry);
	//InjectHook(renameFilePathsFunc, trampoline->Jump(VF5REVOAddFileEntry));


	if (MH_CreateHook(renameFilePathsFunc, &VF5REVOAddFileEntry, reinterpret_cast<LPVOID*>(&orgVF5REVOAddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(renameFilePathsFunc) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}


	std::thread thread(vf5fs_initialization_wait);
	thread.detach();

	return true;
}

bool ParlessGameVF5REVO::RenameROMFilePath(void* a1, char* fpath, int a3)
{
	using namespace std;

	string override = "";
	bool overridden = false;

	char* datapath;
	string path(fpath);

	if (path.starts_with("."))
		path = string(fpath + 1);

	if (!path.starts_with("/"))
		path.insert(0, "/");

	size_t indexOfData = firstIndexOf(path, "rom/");
	//firstIndexOf

	if (indexOfData != -1)
	{
		vector<int> splits;

		path = CBaseParlessGame::instance->translate_path(path, indexOfData);

		if (!overridden)
		{
			// Get the path starting from data/
			string dataPath = path;//.substr(indexOfData);
			string dataPath_lowercase = dataPath;
			std::for_each(dataPath_lowercase.begin(), dataPath_lowercase.end(), [](char& w) { w = std::tolower(w); });

			parless_stringmap::const_iterator match = CBaseParlessGame::instance->fileModMap.find(dataPath_lowercase);

			if (match != CBaseParlessGame::instance->fileModMap.end())
			{
				override = path;

				// Redirect the path from data/ to mods/<ModName>/
				//override.erase(indexOfData - 1, 5);
				override.insert(indexOfData - 1, "mods/" + match->second);

				if (CBaseParlessGame::instance->isUwp)
				{
					override = CBaseParlessGame::instance->asiPath + override.substr(indexOfData);
					indexOfData = CBaseParlessGame::instance->asiPath.length();
				}

				if (filesystem::exists(override))
				{
					overridden = true;
					//std::filesystem::path absolut = std::filesystem::absolute(override);
					//override = absolut.string().c_str();

					override.insert(0, "../../");

					override.copy(fpath, override.length());
					fpath[override.length()] = '\0';
				}
				else
				{
					cout << "Mod file not found: " << override << std::endl;
				}
			}
		}
	}

	/*
	if (CBaseParlessGame::instance->logAll)
	{
		if (indexOfData != -1)
		{
			if (indexOfData == -1)
			{
				indexOfData = 0;
			}

			std::lock_guard<loggingStream> g_(*CBaseParlessGame::instance->allFilepaths);
			(**CBaseParlessGame::instance->allFilepaths) << fpath + indexOfData << std::endl;
		}
	}
	*/

	return overridden;
}