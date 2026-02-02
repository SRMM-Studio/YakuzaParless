#pragma once
#include "CBaseParlessGame.h"

class CBaseParlessGameDE : public CBaseParlessGame
{
public:
	const char* modded_ubik_path = "data/../mods/Parless/ubik/\0";
	virtual std::string translate_path(std::string path, int indexOfData) override;

	typedef __int64 (*t_orgVF5FSROMLoadFile)(void* a1, char* path, int a3);
	static t_orgVF5FSROMLoadFile orgVF5FSROMAddFileEntry;

	typedef __int64 (*t_orgVF5FSROMLoadStreamFile)(void* a1, char* path, int a3);
	static t_orgVF5FSROMLoadStreamFile orgVF5FSROMLoadStreamFile;

	virtual bool enable_vf5fs_support(bool arcadeSupportEnabled) 
	{
		return arcadeSupportEnabled;
	}

	virtual void v5fs_procedure();
	static void v5fs_thread();

	static bool VF5FSRenameROMFilePath(void* a1, char* path, int a3);

	static __int64 VF5FSROMAddFileEntry(void* a1, char* path, int a3)
	{
		char* buf = (char*)malloc(260);
		strcpy_s(buf, 260, path);

		bool success = VF5FSRenameROMFilePath(a1, buf, a3);

		std::string str;

		__int64 result;

		str = std::string(buf);

		if (CBaseParlessGame::instance->logAll)
		{
			std::lock_guard<loggingStream> g_(*CBaseParlessGame::instance->allFilepaths);
			(**CBaseParlessGame::instance->allFilepaths) << str << std::endl;
		}

		result = orgVF5FSROMAddFileEntry(a1, buf, a3);
		free(buf);

		return result;
	}

	static __int64 VF5FSROMLoadStreamFile(void* a1, char* path, int a3)
	{
		char* buf = (char*)malloc(260);
		strcpy_s(buf, 260, path);

		bool success = VF5FSRenameROMFilePath(a1, buf, a3);

		std::string str;

		__int64 result;

		str = std::string(buf);

		if (CBaseParlessGame::instance->logAll)
		{
			std::lock_guard<loggingStream> g_(*CBaseParlessGame::instance->allFilepaths);
			(**CBaseParlessGame::instance->allFilepaths) << str << std::endl;
		}

		result = orgVF5FSROMLoadStreamFile(a1, buf, a3);
		free(buf);

		return result;
	}


	bool redirectUbik;
};
