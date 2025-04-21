#include "ParlessGameYK2.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

char* (*ParlessGameYK2::orgYK2AddFileEntry)(char* a1, uint64_t a2, char* a3, char* a4) = NULL;
uint64_t(*ParlessGameYK2::orgYK2SprintfAwb)(uint64_t param_1, uint64_t param_2, uint64_t param_3, uint64_t param_4) = NULL;

std::string ParlessGameYK2::translate_path(std::string path, int indexOfData)
{
	path = translate_path_original(path, indexOfData);

	if (firstIndexOf(path, "data/entity", indexOfData) != -1 && endsWith(path, ".txt"))
	{
		string loc = "/ja/";

		if (locale == Locale::English)
			loc = "/en/";

		path = rReplace(path, loc, "/");
	}

	return path;
}

bool ParlessGameYK2::hook_add_file()
{

	void* renameFilePathsFunc;
	uint8_t STR_LEN_ADD = 0x40;

	void* hookYK2AddFileEntry = pattern("48 8B C4 89 50 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 18 FF FF FF").get_first(0);

	if (MH_CreateHook(hookYK2AddFileEntry, &YK2AddFileEntry, reinterpret_cast<LPVOID*>(&orgYK2AddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookYK2AddFileEntry) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	return true;
}