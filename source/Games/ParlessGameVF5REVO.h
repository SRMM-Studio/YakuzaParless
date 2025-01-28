#include "CBaseParlessGameDE.h"
#include <algorithm>
#include <string>
#include <filesystem>

class ParlessGameVF5REVO : public CBaseParlessGameDE
{
	virtual std::string get_name() override
	{
		return "Virtua Fighter 5 R.E.V.O.";
	};

	virtual parless_stringmap get_game_map(Locale locale) override
	{
		std::string curLoc;
		std::vector<const char*> locY7Vec{ "de", "en", "es", "fr", "it", "ja", "ko", "pt", "ru", "zh", "zhs", "pt"};

		parless_stringmap result = parless_stringmap();
		result["/entity"] = "/entity_adam";

		for (int i = 0; i < locY7Vec.size(); i++)
		{
			curLoc = std::string(locY7Vec[i]);
			result["/db.adam/" + curLoc] = "/db.adam." + curLoc + "/" + curLoc;
			result["/ui.adam/" + curLoc] = "/ui.adam." + curLoc + "/" + curLoc;
		}

		return result;
	}

	virtual bool hook_add_file() override;

private:
	typedef char* (*t_orgVF5REVOAddFileEntry)(short* a1, int a2, char* a3, char** a4);
	static t_orgVF5REVOAddFileEntry orgVF5REVOAddFileEntry;
	static t_orgVF5REVOAddFileEntry(*hookVF5REVOAddFileEntry);

public:
	typedef __int64 (*t_orgVF5REVOROMLoadFile)(void* a1, char* path, int a3);
	static t_orgVF5REVOROMLoadFile orgVF5REVOROMAddFileEntry;
	static t_orgVF5REVOROMLoadFile(*hookVF5REVOROMAddFileEntry);

	static int VF5REVOAddFileEntry(short* a1, int a2, char* a3, char** a4)
	{
		orgVF5REVOAddFileEntry(a1, a2, a3, a4);
		RenameFilePaths((char*)a1);
		return strlen((char*)a1);
	}
	
	static bool RenameROMFilePath(void* a1, char* fpath, int a3);

	static __int64 VF5RevoROMAddFileEntry(void* a1, char* path, int a3)
	{
		char* buf = (char*)malloc(260);
		strcpy_s(buf, 260, path);

		bool success = RenameROMFilePath(a1, buf, a3);

		std::string str;

		__int64 result;

		str = std::string(buf);

		if (CBaseParlessGame::instance->logAll)
		{
			std::lock_guard<loggingStream> g_(*CBaseParlessGame::instance->allFilepaths);
			(**CBaseParlessGame::instance->allFilepaths) << str << std::endl;
		}

		result = orgVF5REVOROMAddFileEntry(a1, buf, a3);
		free(buf);

		return result;
	}
};