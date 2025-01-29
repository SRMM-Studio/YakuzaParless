#include "CBaseParlessGameDE.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <thread>
#include <algorithm>
#include <filesystem>

CBaseParlessGameDE::t_orgVF5FSROMLoadFile CBaseParlessGameDE::orgVF5FSROMAddFileEntry = NULL;
CBaseParlessGameDE::t_orgVF5FSROMLoadStreamFile CBaseParlessGameDE::orgVF5FSROMLoadStreamFile = NULL;

std::string CBaseParlessGameDE::translate_path(std::string path, int indexOfData)
{
	path = translate_path_original(path, indexOfData);

	if (firstIndexOf(path, "data/entity", indexOfData) != -1 && endsWith(path, ".txt"))
	{
		string loc = "/ja/";
		path = rReplace(path, loc, "/");
	}

	return path;
}

void CBaseParlessGameDE::v5fs_procedure()
{
	std::thread vf5fsThread = std::thread(&CBaseParlessGameDE::v5fs_thread);
	vf5fsThread.detach();
}

void CBaseParlessGameDE::v5fs_thread() 
{
start:
	std::wstring foundModuleName;

	void* moduleAddr = nullptr;
	while (true)
	{
		moduleAddr = GetModuleHandle(L"vf5fs-pxd-w64-d3d12_SteamRetail");

		if (moduleAddr != nullptr)
		{
			foundModuleName = L"vf5fs-pxd-w64-d3d12_SteamRetail";
			break;
		}

		moduleAddr = GetModuleHandle(L"vf5fs-pxd-w64-retail");

		if (moduleAddr != nullptr)
		{
			foundModuleName = L"vf5fs-pxd-w64-retail";
			break;
		}


		moduleAddr = GetModuleHandle(L"vf5fs-pxd-w64-Retail Steam");

		if (moduleAddr != nullptr)
		{
			foundModuleName = L"vf5fs-pxd-w64-Retail Steam";
			break;
		}
	}

	std::wcout << L"VF5FS Module loaded. Name: " << foundModuleName << L", hooking..." << std::endl;

	void* fileLoadAddr = PatternScan(moduleAddr, "48 83 EC ? 45 8B C8 C6 44 24 28 ?");
	void* musicPlayFunc = PatternScan(moduleAddr, "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 41 56 48 83 EC ? 45 33 F6");

	//arghhh yakuza 6!
	if (musicPlayFunc == nullptr && foundModuleName == L"vf5fs-pxd-w64-Retail Steam")
	{
		musicPlayFunc = (byte*)moduleAddr + 0x18F780;
	}

	if (MH_CreateHook(fileLoadAddr, &CBaseParlessGameDE::VF5FSROMAddFileEntry, reinterpret_cast<LPVOID*>(&CBaseParlessGameDE::orgVF5FSROMAddFileEntry)) != MH_OK)
	{
		std::cout << "Hook creation failed. Aborting.\n";
		return;
	}

	if (MH_EnableHook(fileLoadAddr) != MH_OK)
	{
		std::cout << "Hook could not be enabled. Aborting.\n";
		return;
	}

	std::cout << "Initialized ROM file direction hook.\n";

	if (MH_CreateHook(musicPlayFunc, &CBaseParlessGameDE::VF5FSROMLoadStreamFile, reinterpret_cast<LPVOID*>(&CBaseParlessGameDE::orgVF5FSROMLoadStreamFile)) != MH_OK)
	{
		std::cout << "Music hook creation failed. Aborting.\n";
		return;
	}

	if (MH_EnableHook(musicPlayFunc) != MH_OK)
	{
		std::cout << "Music hook could not be enabled. Aborting.\n";
		return;
	}

	std::cout << "Initialized ROM stream file direction hook.\n";

	while (true)
	{
		Sleep(1000);

		if (GetModuleHandle(foundModuleName.c_str()) == NULL)
		{
			std::wcout << "V5FS Module was unloaded, waiting for it to be loaded again...\n";
			goto start;
			break;
		}
	}
}

bool CBaseParlessGameDE::VF5FSRenameROMFilePath(void* a1, char* fpath, int a3)
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

	return overridden;
}