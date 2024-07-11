#include "ParlessGameLJ.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include "Utils/buffer.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

ParlessGameLJ::t_orgLJAddFileEntry ParlessGameLJ::orgLJAddFileEntry = NULL;
ParlessGameLJ::t_orgLJAddFileEntry(*ParlessGameLJ::hookLJAddFileEntry) = NULL;

ParlessGameLJ::t_orgLJGetEntityPath ParlessGameLJ::orgLJGetEntityPath = NULL;
ParlessGameLJ::t_orgLJGetEntityPath(*ParlessGameLJ::hookLJGetEntityPath) = NULL;

void write_int(uintptr_t addr, int val)
{
	unsigned long OldProtection;
	VirtualProtect((LPVOID)(addr), 4, PAGE_EXECUTE_READWRITE, &OldProtection);

	int* ptr = (int*)addr;
	*ptr = val;

	VirtualProtect((LPVOID)(addr), 4, OldProtection, NULL);
}

inline void write_relative_addr(void* instruction_start, intptr_t target, int instruction_length = 7)
{
	intptr_t instruction_end = (intptr_t)((unsigned long long)instruction_start + instruction_length);
	unsigned int* offset = (unsigned int*)((unsigned long long)instruction_start + (instruction_length - 4));

	int calcOffset = target - instruction_end;
	write_int((intptr_t)offset, calcOffset);
}

bool ParlessGameLJ::hook_add_file()
{
	hookLJAddFileEntry = (t_orgLJAddFileEntry*)pattern("41 57 48 8D A8 68 FE FF FF 48 81 EC 58 02 00 00 C5 F8 29 70 A8").get_first(-20);

	if (MH_CreateHook(hookLJAddFileEntry, &LJAddFileEntry, reinterpret_cast<LPVOID*>(&orgLJAddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookLJAddFileEntry) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	// For some unknown reason, the pointer here is pointing to an interrupt (0xCC) that replaced the first byte of the trampoline space
	*((char*)orgLJAddFileEntry) = 0x48;

	hook_BindCpk = (t_CriBind*)get_pattern("41 57 48 8B EC 48 83 EC 70 4C 8B 6D 58", -26);
	org_BindDir = (t_CriBind)get_pattern("41 57 48 83 EC 30 48 8B 74 24 78 33 ED", -23);

	if (MH_CreateHook(hook_BindCpk, &BindCpk, reinterpret_cast<LPVOID*>(&org_BindCpk)) != MH_OK)
	{
		std::cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hook_BindCpk) != MH_OK)
	{
		std::cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	hookLJGetEntityPath = (t_orgLJGetEntityPath*)pattern("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC ? 41 8B F8 48 8B D9 0F B7 F2").get_first(0);

	if (MH_CreateHook(hookLJGetEntityPath, &LJGetEntityPath, reinterpret_cast<LPVOID*>(&orgLJGetEntityPath)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookLJGetEntityPath) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	// Same issue as above
	*((char*)org_BindCpk) = 0x48;

	std::cout << "Applied CPK directory bind hook.\n";

	void* addr = pattern("48 8D 35 ? ? ? ? 48 8B C6 33 FF").get_first(0);


	void* buffer = AllocateBuffer(addr);
	unsigned int diff = (int64_t)addr - (intptr_t)GetModuleHandle(NULL);

	memcpy_s(buffer, strlen(modded_ubik_path), modded_ubik_path, strlen(modded_ubik_path));

	write_relative_addr(addr, (intptr_t)buffer, 7);
	//Memory::WriteOffsetValue(addr, modded_ubik_path);

	return true;
}