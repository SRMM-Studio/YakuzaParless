#include "ParlessGameYK2R.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

int (*ParlessGameYK2R::orgYK2AddFileEntry)(short* a1, int a2, char* a3, char** a4) = NULL;
uint64_t(*ParlessGameYK2R::orgYK2SprintfAwb)(uint64_t param_1, uint64_t param_2, uint64_t param_3, uint64_t param_4) = NULL;

std::string ParlessGameYK2R::translate_path(std::string path, int indexOfData)
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

bool ParlessGameYK2R::hook_add_file()
{

	void* renameFilePathsFunc;
	uint8_t STR_LEN_ADD = 0x40;

	void* hookYK2AddFileEntry = pattern("4C 89 4C 24 ? 89 54 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 8B 04 24 44 8B E2 C5 F8 29 B5 ? ? ? ? C5 F8 29 BD ? ? ? ? C5 78 29 85 ? ? ? ? C5 78 29 8D ? ? ? ? C5 78 29 95 ? ? ? ? C5 78 29 9D ? ? ? ? C5 78 29 A5 ? ? ? ? C5 78 29 AD ? ? ? ? C5 78 29 B5 ? ? ? ? C5 78 29 BD ? ? ? ? 4C 8B F1 33 DB 33 C0").get_first(0);

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