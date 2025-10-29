#include "ParlessGameY5.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

__int64 (*ParlessGameY5::orgY5AddFileEntry)(__int64 a1, __int64 filepath, __int64 a3, int a4, __int64 a5, __int64 a6, int a7, __int64 a8, int a9, char a10, int a11, char a12, int a13, char a14);

char* (*ParlessGameY5::hook_Y5AddStreamFile)(__int64 unknown, char* filePath) = NULL;
char* (*ParlessGameY5::org_Y5AddStreamFile)(__int64 unknown, char* filePath) = NULL;
__int64 (*ParlessGameY5::orgY5MotionArchiveEntry)(_int64 a1, char* filePath, int a3, int a4, int a5, char a6) = NULL;

ParlessGameY5::t_Y5TougiGenerateContestantPool ParlessGameY5::hook_Y5TougiGenerateContestantPool = NULL;
ParlessGameY5::t_Y5TougiGenerateContestantPool ParlessGameY5::org_Y5TougiGenerateContestantPool = NULL;

ParlessGameY5::t_Y5AddUSMFile ParlessGameY5::hook_Y5AddUSMFile = NULL;
ParlessGameY5::t_Y5AddUSMFile ParlessGameY5::org_Y5AddUSMFile = NULL;

uint8_t** ParlessGameY5::tougiContestantCountAddresses = NULL;

bool ParlessGameY5::hook_add_file()
{
	void* renameFilePathsFunc;

	Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

    renameFilePathsFunc = get_pattern("48 89 4C 24 20 49 8B D7 48 8B 0D ? ? ? ?", 15);
    ReadCall(renameFilePathsFunc, orgY5AddFileEntry);

    InjectHook(renameFilePathsFunc, trampoline->Jump(Y5AddFileEntry));

    hook_BindCpk = (t_CriBind*)get_pattern("41 57 48 8B EC 48 83 EC 70 4C 8B 6D 58 33 DB", -26);
    org_BindDir = (t_CriBind)get_pattern("41 57 48 83 EC 30 48 8B 74 24 78 33 ED", -23);

    if (MH_CreateHook(hook_BindCpk, &BindCpk, reinterpret_cast<LPVOID*>(&org_BindCpk)) != MH_OK)
    {
        cout << "BindCPK Hook creation failed. Aborting.\n";
        return false;
    }

    if (MH_EnableHook(hook_BindCpk) != MH_OK)
    {
        cout << "BindCPK Hook could not be enabled. Aborting.\n";
        return false;
    }

    org_BindCpk = (t_CriBind)((char*)org_BindCpk + 1);

    hook_Y5AddStreamFile = (t_Y5AddStreamFile)get_pattern("40 53 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 80 06 00 00");
    hook_Y5AddUSMFile = (t_Y5AddUSMFile)ReadCallFrom(get_pattern("E8 ? ? ? ? E8 ? ? ? ? 48 8D 4F"));

    if (MH_CreateHook(hook_Y5AddStreamFile, &ParlessGameY5::Y5AddStreamFile, reinterpret_cast<LPVOID*>(&org_Y5AddStreamFile)) != MH_OK)
    {
        cout << "Y5AddStreamFile Hook could not be enabled. Aborting.\n";
        return false;
    }

    if (MH_EnableHook(hook_Y5AddStreamFile) != MH_OK)
    {
        cout << "Y5AddStreamFile could not be enabled. Aborting.\n";
        return false;
    }

    if (MH_CreateHook(hook_Y5AddUSMFile, &ParlessGameY5::Y5AddUSMFile, reinterpret_cast<LPVOID*>(&org_Y5AddUSMFile)) != MH_OK)
    {
        cout << "Y5AddUSMFile Hook could not be enabled. Aborting.\n";
        return false;
    }

    if (MH_EnableHook(hook_Y5AddUSMFile) != MH_OK)
    {
        cout << "Y5AddUSMFile could not be enabled. Aborting.\n";
        return false;
    }

    /*
    void* renameMotionArchiveFunc = get_pattern("48 89 4C 24 ? 55 56 57 41 56 41 57 48 83 EC ? 48 C7 44 24 ? ? ? ? ? 48 89 9C 24 ? ? ? ? 41 8B F02");

    if (MH_CreateHook(renameMotionArchiveFunc, &ParlessGameY5::Y5MotionArchiveEntry, reinterpret_cast<LPVOID*>(&orgY5MotionArchiveEntry)) != MH_OK)
        return false;

    if (MH_EnableHook(renameMotionArchiveFunc) != MH_OK)
        return false;
        */

    DoEssentialFixes();

    return true;
}

void ParlessGameY5::DoEssentialFixes()
{
    //Unhardcode the hardcoded limit of 52 coliseum fighters
    hook_Y5TougiGenerateContestantPool = (t_Y5TougiGenerateContestantPool)get_pattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 49 8B 80");

    MH_CreateHook(hook_Y5TougiGenerateContestantPool, Y5GenerateTougiContestantPool, reinterpret_cast<LPVOID*>(&org_Y5TougiGenerateContestantPool));
    MH_EnableHook(hook_Y5TougiGenerateContestantPool);

    tougiContestantCountAddresses = new uint8_t*[4];
    tougiContestantCountAddresses[0] = (uint8_t*)get_pattern("83 FF ? 72 ? 6B 05") + 0x2;
    tougiContestantCountAddresses[1] = (uint8_t*)get_pattern("83 FF ? 72 ? EB ? 44 8D 4F") + 0x2;
    tougiContestantCountAddresses[2] = (uint8_t*)get_pattern("41 B8 ? ? ? ? 45 3B C8 77") + 0x2;
    tougiContestantCountAddresses[3] = (uint8_t*)get_pattern("FF C6 83 FE ? 0F 82 ? ? ? ? 48 8B 5C 24") + 0x4;
}