#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ShellAPI.h>
#include <mutex>
#include <thread>

#include "ParlessGames.h"

static HMODULE hDLLModule;

namespace Parless
{
	const char* VERSION = "2.2.6";
	
	t_CriBind(*hook_BindCpk);
	t_CriBind org_BindCpk = NULL;
	t_CriBind org_BindDir = NULL;
	BYTE* (*orgVFeSAddFileEntry)(BYTE* a1, int a2);

	//t_CriBind(*hookJE_BindCpk) = (t_Bind*)0x141855794;
	//t_CriBind orgJE_BindDir = (t_Bind)0x141852f3c;

	typedef bool (*t_DEIsDemo)();

	CBaseParlessGame* currentParlessGame;
	Game currentGame;
	Locale currentLocale;
	const char* versionStr;

	bool isDemo;

	bool modsLoaded;

	std::vector<string> mods;
	stringmap gameMap;
	unordered_map<string, vector<string>> cpkModMap;

	const int DIR_WORKSIZE = 0x58;
	unordered_map<string, int> cpkBindIdMap;

	unordered_map<string, int> parlessPathMap;

	int gameMapMaxSplits = 3;

	uint8_t STR_LEN_ADD = 0x40;

	string asiPath;

	// Initialized from the INI
	bool loadMods;
	bool loadParless;
	bool rebuildMLO;
	bool enableReload;
	bool v5fsArcade;

	// Mod paths will be relative to the ASI's directory instead of the game's directory (to support undumped UWP games)
	bool isUwp;
	bool isXbox;
	bool isGOG;

	bool hasRepackedPars;
	// Logging streams
	loggingStream modOverrides;
	loggingStream parlessOverrides;
	loggingStream allFilepaths;

	// Logging variables
	bool logMods;
	bool logParless;
	bool logAll;
	bool ignoreNonPaths;


	bool rebuildingMLO = false;

	/// <summary>
	/// Renames file paths to load files from the mods directory or .parless paths instead of pars.
	/// </summary>
	/// <returns>true if the path was overridden</returns>
	bool RenameFilePaths(char* filepath)
	{
		string override = "";
		bool overridden = false;

		char* datapath;
		string path(filepath);

		size_t indexOfData = firstIndexOf(path, "data/");

		if (indexOfData == -1 && loadMods)
		{
			// File might be in mods instead, which means we're receiving it modified
			indexOfData = firstIndexOf(path, "mods/");

			if (indexOfData != -1)
			{
				// Replace the mod path with data
				path = removeModPath(path, indexOfData);
			}
		}

		if (indexOfData != -1)
		{
			vector<int> splits;

			if (loadParless)
			{
				// Remove parless if it's there
				path = removeParlessPath(path, indexOfData);
			}

			path = currentParlessGame->translate_path(path, indexOfData);

			if (hasRepackedPars)
			{
				if (endsWith(path, ".par") || endsWith(path, ".cpk"))
				{
					override = path;

					// Redirect the path from data/ to mods/Parless/
					override.erase(indexOfData, 4);
					override.insert(indexOfData, "mods/Parless");

					if (isUwp)
					{
						override = asiPath + override.substr(indexOfData);
						indexOfData = asiPath.length();
					}

					if (filesystem::exists(override))
					{
						overridden = true;

						override.copy(filepath, override.length());
						filepath[override.length()] = '\0';

						if (logMods)
						{
							std::lock_guard<loggingStream> g_(modOverrides);
							(*modOverrides) << filepath + indexOfData << std::endl;
						}
					}
					else
					{
						if(logParless)
							cout << "Par not found: " << override << std::endl;
					}
				}
			}

			if (loadParless && !overridden)
			{
				int parlessIndex = -1;
				unordered_map<string, int>::const_iterator parlessPathMatch = parlessPathMap.find(pathWithoutFilename(path).substr(indexOfData + 4));

				if (parlessPathMatch != parlessPathMap.end())
				{
					parlessIndex = parlessPathMatch->second + indexOfData + 4;

					override = path;

					// Check if file exists in .parless path
					override.insert(parlessIndex, ".parless");

					if (filesystem::exists(override))
					{
						overridden = true;

						override.copy(filepath, override.length());
						filepath[override.length()] = '\0';

						if (logParless)
						{
							std::lock_guard<loggingStream> g_(parlessOverrides);
							(*parlessOverrides) << filepath + indexOfData << std::endl;
						}
					}
					else
					{
						cout << ".parless file not found: " << override << std::endl;
					}
				}
			}

			if (loadMods && !overridden)
			{
				// Get the path starting from data/
				string dataPath = path.substr(indexOfData + 4);
				string dataPath_lowercase = dataPath;
				std::for_each(dataPath_lowercase.begin(), dataPath_lowercase.end(), [](char& w) { w = std::tolower(w); });

				stringmap::const_iterator match = currentParlessGame->fileModMap.find(dataPath_lowercase);

				if (match != currentParlessGame->fileModMap.end())
				{
					override = path;

					// Redirect the path from data/ to mods/<ModName>/
					override.erase(indexOfData, 4);
					override.insert(indexOfData, "mods/" + match->second);

					if (isUwp)
					{
						override = asiPath + override.substr(indexOfData);
						indexOfData = asiPath.length();
					}

					if (filesystem::exists(override))
					{
						overridden = true;

						override.copy(filepath, override.length());
						filepath[override.length()] = '\0';

						if (logMods)
						{
							std::lock_guard<loggingStream> g_(modOverrides);
							(*modOverrides) << filepath + indexOfData << std::endl;
						}
					}
					else
					{
						cout << "Mod file not found: " << override << std::endl;
					}
				}
			}
		}

		if (logAll)
		{
			if (indexOfData != -1 || !ignoreNonPaths)
			{
				if (indexOfData == -1)
				{
					indexOfData = 0;
				}

				std::lock_guard<loggingStream> g_(allFilepaths);
				(*allFilepaths) << filepath + indexOfData << std::endl;
			}
		}

		return overridden;
	}

	void BindCpkPaths(void* param_1, void* param_2, const char* filepath, int param_7)
	{
		string path(filepath);

		size_t indexOfData = firstIndexOf(path, "data/");

		if (indexOfData == -1 && loadMods)
		{
			// File might be in mods instead, which means we're receiving it modified
			indexOfData = firstIndexOf(path, "mods/");

			if (indexOfData != -1)
			{
				// Replace the mod path with data
				path = removeModPath(path, indexOfData);
			}
		}

		if (indexOfData != -1)
		{
			vector<int> splits;

			if (loadParless)
			{
				// Remove parless if it's there
				path = removeParlessPath(path, indexOfData);
			}

			if (!gameMap.empty())
			{
				path = currentParlessGame->translate_path(path, indexOfData);
			}

			if (loadMods)
			{
				// Get the path starting from data/
				string dataPath = path.substr(indexOfData + 4);
				string dataPath_lowercase = dataPath;
				std::for_each(dataPath_lowercase.begin(), dataPath_lowercase.end(), [](char& w) { w = std::tolower(w); });

				auto match = cpkModMap.find(dataPath_lowercase);
				if (match != cpkModMap.end())
				{
					vector<string> modVec = match->second;
					for (int i = 0; i < modVec.size(); i++)
					{
						string override = path;

						// Redirect the path from data/ to mods/<ModName>/
						override.erase(indexOfData, 4);
						override.insert(indexOfData, "mods/" + modVec[i]);

						override.erase(override.size() - 4);

						auto idMatch = cpkBindIdMap.find(override);
						if (idMatch != cpkBindIdMap.end())
						{
							cout << "Mod CPK directory already bound with ID " << idMatch->second << ": " << override << std::endl;
							continue;
						}

						if (filesystem::exists(override))
						{
							cpkBindIdMap[override] = 0;
							int result = currentParlessGame->org_BindDir(param_1, param_2, override.c_str(), malloc(DIR_WORKSIZE), DIR_WORKSIZE, &cpkBindIdMap[override], param_7);

							if (result != 0)
							{
								cout << "CRI ERROR: " << result << endl;
							}

							printf_s("Bound directory \"%s\" with ID %d\n", override.c_str() + indexOfData, cpkBindIdMap[override]);

							if (logMods)
							{
								std::lock_guard<loggingStream> g_(modOverrides);
								(*modOverrides) << override.c_str() + indexOfData << std::endl;
							}
						}
						else
						{
							cout << "Mod CPK directory not found: " << override << std::endl;
						}
					}
				}
			}
		}
	}

	BYTE* VFeSAddFileEntry(BYTE* a1, int a2)
	{
		BYTE* result = orgVFeSAddFileEntry(a1, a2);
		RenameFilePaths((char*)result);
		return result;
	}

	int BindCpk(void* param_1, void* param_2, const char* path, void* param_4, int param_5, void* bindId, int param_7)
	{
		cout << endl;
		cout << "Binding CPK: " << path << endl;
		BindCpkPaths(param_1, param_2, path, param_7);

		return currentParlessGame->org_BindCpk(param_1, param_2, path, param_4, param_5, bindId, param_7);
	}
}

void RebuildMLO()
{
	STARTUPINFOA info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	char cmdArgs[] = "ShinRyuModManager.exe -s";
	if (CreateProcessA("ShinRyuModManager.exe", cmdArgs, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		std::cout << " Failed to start the process! Error Code: " << GetLastError();
	}
}

void ReadModLoadOrder()
{
	using namespace Parless;

	const char* MLO_MAGIC = "_OLM";
	string MLO_FILE = asiPath + "/YakuzaParless.mlo";

	if (!filesystem::exists(MLO_FILE))
		return;

	ifstream mlo(MLO_FILE, ios::binary | ios::in);

	if (!mlo)
		return;

	char magic[5];
	mlo.read(magic, 4);
	magic[4] = '\0';

	if (strcmp(magic, MLO_MAGIC))
		return;

	// Skip endianness
	mlo.seekg(4, SEEK_CUR);

	uint32_t version;
	mlo.read((char*)&version, sizeof(version));

	// Skip filesize
	mlo.seekg(4, SEEK_CUR);

	if (version == 0x20000)
	{
		uint32_t modNameStart;
		uint32_t modCount;

		uint32_t fileNameStart;
		uint32_t fileCount;

		uint32_t parlessPathStart;
		uint32_t parlessPathCount;

		uint32_t cpkFolderStart;
		uint32_t cpkFolderCount;

		mlo.read((char*)&modNameStart, sizeof(modNameStart));
		mlo.read((char*)&modCount, sizeof(modCount));

		mlo.read((char*)&fileNameStart, sizeof(fileNameStart));
		mlo.read((char*)&fileCount, sizeof(fileCount));

		mlo.read((char*)&parlessPathStart, sizeof(parlessPathStart));
		mlo.read((char*)&parlessPathCount, sizeof(parlessPathCount));

		mlo.read((char*)&cpkFolderStart, sizeof(cpkFolderStart));
		mlo.read((char*)&cpkFolderCount, sizeof(cpkFolderCount));

		// Skip padding
		mlo.seekg(0x10, SEEK_CUR);

		uint16_t length;
		char* name;

		mlo.seekg(modNameStart);

		for (int i = 0; i < modCount; i++)
		{
			mlo.read((char*)&length, sizeof(length));

			name = new char[length];
			mlo.read(name, length);

			mods.push_back(string(name));
			delete[] name;
		}

		mlo.seekg(fileNameStart);

		uint16_t index;
		for (int i = 0; i < fileCount; i++)
		{
			mlo.read((char*)&index, sizeof(index));
			mlo.read((char*)&length, sizeof(length));

			name = new char[length];
			mlo.read(name, length);

			currentParlessGame->fileModMap[string(name)] = mods[index];
			delete[] name;
		}

		mlo.seekg(parlessPathStart);

		for (int i = 0; i < parlessPathCount; i++)
		{
			mlo.read((char*)&index, sizeof(index));
			mlo.read((char*)&length, sizeof(length));

			name = new char[length];
			mlo.read(name, length);

			parlessPathMap[string(name)] = index;
			delete[] name;
		}

		mlo.seekg(cpkFolderStart);

		uint16_t count;
		for (int i = 0; i < fileCount; i++)
		{
			mlo.read((char*)&count, sizeof(count));
			mlo.read((char*)&length, sizeof(length));

			name = new char[length];
			mlo.read(name, length);

			vector<string> modIndices;
			for (int j = 0; j < count; j++)
			{
				mlo.read((char*)&index, sizeof(index));
				modIndices.push_back(mods[index]);
			}

			cpkModMap[string(name)] = modIndices;
			delete[] name;
		}

	}
}

void InitializeScripts()
{
	typedef int(__stdcall* asi_init)();

	//Iterate the filemap for any ASI scripts. Then load them
	for (auto it = Parless::currentParlessGame->fileModMap.begin(); it != Parless::currentParlessGame->fileModMap.end(); it++) {

		if (endsWith(it->first, ".asi"))
		{
			string path;
			path += string("mods/") + string(it->second) + string(it->first);

			string initStr;
			initStr += "Initializing script file " + path;
			cout << initStr << std::endl;

			HINSTANCE asiScript = LoadLibraryA(path.c_str());

			if (asiScript)
			{
				asi_init asiInitFunc = (asi_init)GetProcAddress(asiScript, "InitializeASI");

				if (asiInitFunc)
					asiInitFunc();
			}
			else
			{
				int errCode = GetLastError();
				cout << "Script LoadLibrary fail: " << path.c_str() << " Error Code: " << errCode << endl;

				string errorMsg;

				switch (errCode)
				{
				default:
				{
					errorMsg += string("Failed to load ASI script ");
					errorMsg += path;
					errorMsg += string(" Error Code ");
					errorMsg += std::to_string(errCode);
					break;
				};

				case 225:
				{
					errorMsg += "Failed to load ASI script because it was marked as a virus.\nMost likely a false positive! Try unblocking and trying again.";
					break;
				};
				}


				MessageBoxA(0, errorMsg.c_str(), "ASI Error", 0);
			}
		}
	}
}

void Reload()
{
	using namespace Parless;

	if (Parless::currentParlessGame == nullptr)
		return;

	//Initialize the virtual->real map
	gameMap = currentParlessGame->get_game_map(currentLocale);

	// Rebuild the MLO file
	if (rebuildMLO)
	{
		//Rebuilding MLO is only problematic when we initialize through winmm.dll
		/*
		if (std::filesystem::exists("winmm.dll"))
		{
			cout << "Not rebuilding MLO because it is unsupported by this game." << endl;
		}
		*/
		//else
		//{
			cout << "Rebuilding MLO... ";
			RebuildMLO();
			cout << "DONE!\n";
		//}
	}

	// Read MLO file
	cout << "Reading MLO... ";
	ReadModLoadOrder();
	cout << "DONE!\n";


	//ASI Script reloading is not supported

	// These maps are filled after reading the MLO
	if (!parlessPathMap.size())
	{
		cout << "No \".parless\" paths were found in the MLO.\n";
		loadParless = false;
	}
	if (!Parless::currentParlessGame->fileModMap.size())
	{
		cout << "No mod files were found in the MLO.\n";
		loadMods = false;
	}

	// Check if repacked pars exist (old engine only)
	hasRepackedPars = filesystem::is_directory("mods/Parless");

	if (!hasRepackedPars)
	{
		cout << "No repacked pars were found.\n";
	}

	cout << endl;
}


bool is_key_down(int key)
{
	return GetAsyncKeyState(key) == -32767;
}

bool is_key_held(int key)
{
	return (GetAsyncKeyState(key) & 0x8000) == 0x8000;
}

void input_thread() {

	while (true)
	{
		if (is_key_held(VK_LMENU))
			if (is_key_held(VK_LCONTROL))
				if (is_key_down(0x52))
				{
					std::cout << "Reloading...\n";
					Reload();
					std::cout << "Reloaded\n";
				}
	}
}

void OnInitializeHook()
{
	std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(GetModuleHandle(nullptr), ".text");

	using namespace Memory::VP;
	using namespace hook;

	using namespace Parless;

	const char* FILE_LOAD_MSG = "Applied file loading hook.\n";
	const char* CPK_LOAD_MSG = "Applied CPK loading hook.\n";
	const char* CPK_BIND_MSG = "Applied CPK directory bind hook.\n";
	const char* ADX_LOAD_MSG = "Applied ADX loading hook.\n";
	const char* AWB_LOAD_MSG = "Applied AWB loading hook.\n";
	const char* PATH_EXT_MSG = "Applied file path extension hook.\n";

	// Read INI variables

	// Obtain a path to the ASI
	wchar_t wcModulePath[MAX_PATH];
	GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
	PathRenameExtensionW(wcModulePath, L".ini");

	// Store the ASI's path for later
	wstring asiPathW(wcModulePath);
	asiPath = string(asiPathW.begin(), asiPathW.end());
	replace(asiPath.begin(), asiPath.end(), '\\', '/');
	asiPath = pathWithoutFilename(asiPath);

	if (GetPrivateProfileIntW(L"Parless", L"TempDisabled", 0, wcModulePath))
	{
		WritePrivateProfileStringW(L"Parless", L"TempDisabled", L"0", wcModulePath);
		return;
	}

	if (GetPrivateProfileIntW(L"Parless", L"ParlessEnabled", 1, wcModulePath) == 0)
	{
		return;
	}

	// LooseFilesEnabled is set to 0 by default in the INI
	loadParless = GetPrivateProfileIntW(L"Overrides", L"LooseFilesEnabled ", 1, wcModulePath);
	loadMods = GetPrivateProfileIntW(L"Overrides", L"ModsEnabled", 1, wcModulePath);
	rebuildMLO = GetPrivateProfileIntW(L"Overrides", L"RebuildMLO", 0, wcModulePath);
	enableReload = GetPrivateProfileIntW(L"Debug", L"ReloadingEnabled", 0, wcModulePath);
	v5fsArcade = GetPrivateProfileIntW(L"Debug", L"V5FSArcadeSupport", 0, wcModulePath);

	int localeValue = GetPrivateProfileIntW(L"Overrides", L"Locale", 0, wcModulePath);

	logMods = GetPrivateProfileIntW(L"Logs", L"LogMods", 0, wcModulePath);
	logParless = GetPrivateProfileIntW(L"Logs", L"LogParless", 0, wcModulePath);
	logAll = GetPrivateProfileIntW(L"Logs", L"LogAll", 0, wcModulePath);
	ignoreNonPaths = GetPrivateProfileIntW(L"Logs", L"IgnoreNonPaths", 1, wcModulePath);

	isUwp = GetPrivateProfileIntW(L"UWP", L"UWPGame", -1, wcModulePath) != -1;


	// Get the name of the current game
	wchar_t exePath[MAX_PATH + 1];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	wstring wstr(exePath);
	string currentGameName = basenameBackslashNoExt(string(wstr.begin(), wstr.end()));

	if (currentGameName == "startup")
		return;

	//Important for winmm
	if (startsWith(currentGameName, "RyuMod"))
		return;

	if (startsWith(currentGameName, "ShinRyu"))
		return;


	if (GetPrivateProfileIntW(L"Debug", L"ConsoleEnabled", 0, wcModulePath))
	{
		// Open debugging console
		AllocConsole();
		FILE* fDummy;
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
	}

	if (wstr.find(L"WindowsApps") != std::wstring::npos || wstr.find(L"Xbox") != std::wstring::npos || std::filesystem::exists("MicrosoftGame.config"))
		Parless::isXbox = true;

	if (GetModuleHandle(L"galaxy64") != nullptr)
		Parless::isGOG = true;

	cout << "Game Name: " << currentGameName << endl;

	if (Parless::isXbox)
		versionStr = "Gamepass/Xbox";
	else if (Parless::isGOG)
		versionStr = "GOG";
	else
		versionStr = "Steam";

	cout << versionStr << endl;

	currentGame = getGame(currentGameName);
	currentParlessGame = get_parless_game(currentGame);
	currentGameName = currentParlessGame->get_name();	
	currentLocale = localeValue < 4 && localeValue >= 0 ? Locale(localeValue) : Locale::English;

	currentParlessGame->locale = currentLocale;
	currentParlessGame->isUwp = isUwp;
	currentParlessGame->isXbox = isXbox;
	currentParlessGame->isGOG = isGOG;
	currentParlessGame->parless_rename_func = &RenameFilePaths;
	currentParlessGame->parless_cpk_bind_path_func = &BindCpkPaths;
	currentParlessGame->logAll = logAll;
	currentParlessGame->logMods = logMods;
	currentParlessGame->logParless;
	currentParlessGame->modOverrides = &modOverrides;
	currentParlessGame->parlessOverrides = &parlessOverrides;
	currentParlessGame->allFilepaths = &allFilepaths;
	CBaseParlessGame::instance = currentParlessGame;

	currentParlessGame->init();

	cout << "YakuzaParless v" << VERSION << "\n\n";
	cout << "Detected game: " << currentGameName << (isUwp ? " UWP version" : "") << endl;

	if (currentParlessGame->get_name() == "Unsupported")
	{
		cout << currentGameName << " is unsupported. Aborting." << endl;
		return;
	}

	//BREAKS GAIDEN AND 8
	Trampoline* trampoline = nullptr;

	Reload();

	//Initialize Extensions/Script Mods
	cout << "Initializing mod scripts... ";
	InitializeScripts();
	cout << "DONE!\n";

	if (!(loadParless || loadMods || hasRepackedPars || logAll))
	{
		cout << "No mods were loaded. LogAll is disabled. No patches will be applied. Aborting.\n";
	}
	else
	{
		char pathToLog[MAX_PATH];

		// Open log streams if logging is enabled, remove them otherwise
		asiPath.copy(pathToLog, asiPath.length());
		pathToLog[asiPath.length()] = '\0';
		strcat_s(pathToLog, "/modOverrides.txt");
		
		if (logMods) 
			(*modOverrides).open(pathToLog, ios::out);
		else 
			remove(pathToLog);

		asiPath.copy(pathToLog, asiPath.length());
		pathToLog[asiPath.length()] = '\0';
		strcat_s(pathToLog, "/parlessOverrides.txt");
		
		if (logParless && loadParless) 
			(*parlessOverrides).open(pathToLog, ios::out);
		else 
			remove(pathToLog);

		asiPath.copy(pathToLog, asiPath.length());
		pathToLog[asiPath.length()] = '\0';
		strcat_s(pathToLog, "/allFilespaths.txt");
		
		if (logAll) 
			(*allFilepaths).open(pathToLog, ios::out);
		else 
			remove(pathToLog);

		void* renameFilePathsFunc;

		if (MH_Initialize() != MH_OK)
		{
			cout << "Minhook initialization failed. Aborting.\n";
			return;
		}

		//V5FS
		if (currentGame >= Game::Yakuza6)
		{
			CBaseParlessGameDE* deGame = (CBaseParlessGameDE*)currentParlessGame;

			if (deGame->enable_vf5fs_support(v5fsArcade))
			{
				deGame->v5fs_procedure();
			}
		}
		
		//UBIK time
		if (currentGame >= Game::LostJudgment)
		{
			CBaseParlessGameDE* deGame = (CBaseParlessGameDE*)currentParlessGame;

			if (std::filesystem::is_directory("mods/Parless/ubik"))
			{
				deGame->redirectUbik = true;
				std::cout << "Redirect Ubik\n";
			}
		}

		if (!currentParlessGame->hook_add_file())
		{
			std::wstringstream stream;
			stream << L"Hooking failed. YakuzaParless/SRMM will not work.\nDetected Version: " << versionStr << "\nEXE Name: " << basenameBackslashNoExt(string(wstr.begin(), wstr.end())).c_str();

			MessageBox(nullptr, stream.str().c_str(), L"YakuzaParless Error", 0);
			cout << "Hooking failed. Aborting.\n";
			return;
		}

		switch (currentGame)
		{
		case Game::VFeSports:
		{
			renameFilePathsFunc = get_pattern("40 BA 10 04 00 00 E8", 6);
			ReadCall(renameFilePathsFunc, orgVFeSAddFileEntry);

			InjectHook(renameFilePathsFunc, trampoline->Jump(VFeSAddFileEntry));
			cout << FILE_LOAD_MSG;

			// Not really sure if this affects anything, but just in case
			auto stringLenAddr = get_pattern("48 89 9C 24 A0 04 00 00 48 8B F9 B9 68 00 00 00 E8", 0xC);
			Patch(stringLenAddr, 0x68 + STR_LEN_ADD);
			cout << PATH_EXT_MSG;
			break;
		}
		}

	}

	modsLoaded = true;

	if (Parless::enableReload)
	{
		std::thread thread(input_thread);
		thread.detach();
		std::cout << "Reload ENABLED\n";
	}

	cout << "Hook function finished.\n";
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hDLLModule = hinstDLL;
		OnInitializeHook();
	}
	return TRUE;
}

//Exports
extern "C"
{
	__declspec(dllexport) const char* YP_GET_VERSION()
	{
		return Parless::VERSION;
	}

	__declspec(dllexport) bool YP_ARE_MODS_LOADED()
	{
		return Parless::modsLoaded;
	}

	__declspec(dllexport) unsigned int YP_GET_NUM_MODS()
	{
		return Parless::mods.size();
	}

	__declspec(dllexport) unsigned int YP_GET_NUM_MOD_FILES()
	{
		if (Parless::currentParlessGame == nullptr)
			return 0;

		return Parless::currentParlessGame->fileModMap.size();
	}


	__declspec(dllexport) const char* YP_GET_MOD_NAME(unsigned int modIndex)
	{
		if (modIndex > YP_GET_NUM_MODS())
			return "";

		return Parless::mods[modIndex].c_str();
	}

	__declspec(dllexport) const char* YP_GET_MOD_FILE(unsigned int fileIdx)
	{
		if (fileIdx > YP_GET_NUM_MODS())
			return "";

		if (Parless::currentParlessGame == nullptr)
			return "";

		auto it = Parless::currentParlessGame->fileModMap.begin();
		std::advance(it, fileIdx);

		return it->first.c_str();
	}
	__declspec(dllexport) const char* YP_GET_FILE_PATH(char* file)
	{
		Parless::RenameFilePaths(file);
		return file;
	}

}
