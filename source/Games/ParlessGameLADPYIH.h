#include "CBaseParlessGameDE.h"

class ParlessGameLADPYIH : public CBaseParlessGameDE
{
public:
	std::string get_name() override
	{
		return "Like A Dragon: Pirate Yakuza in Hawaii";
	}

	parless_stringmap get_game_map(Locale locale) override
	{
		std::string curLoc;
		std::vector<const char*> locInfWealthVec{ "de", "en", "es", "fr", "it", "ja", "ko", "zh", "zhs", "ru", "pt"};

		parless_stringmap result = parless_stringmap();
		result["/entity"] = "/entity_spr";
		result["/ui.spr/texture"] = "/ui.spr.common/texture";

		for (int i = 0; i < locInfWealthVec.size(); i++)
		{
			curLoc = std::string(locInfWealthVec[i]);
			result["/db.spr/" + curLoc] = "/db.spr." + curLoc;
			result["/ui.spr/" + curLoc] = "/ui.spr." + curLoc;
		}

		return result;
	}

	bool hook_add_file() override;

	typedef int (*t_orgPYIHAddFileEntry)(short* a1, int a2, char* a3, char** a4);
	typedef int (*t_orgPYIHGetEntityPath)(char* in_path, unsigned int e_kind, unsigned int stageid, unsigned int daynight, void* uid);
	
	static t_orgPYIHAddFileEntry orgPYIHAddFileEntry;
	static t_orgPYIHAddFileEntry(*hookPYIHAddFileEntry);

	static t_orgPYIHGetEntityPath orgPYIHGetEntityPath;
	static t_orgPYIHGetEntityPath(*hookPYIHGetEntityPath);

	static int PYIHAddFileEntry(short* a1, int a2, char* a3, char** a4)
	{
		orgPYIHAddFileEntry(a1, a2, a3, a4);
		RenameFilePaths((char*)a1);
		return strlen((char*)a1);
	}

	static int PYIHGetEntityPath(char* in_path, unsigned int e_kind, unsigned int stageid, unsigned int daynight, __int64* uid)
	{
		orgPYIHGetEntityPath(in_path, e_kind, stageid, daynight, uid);
		RenameFilePaths(in_path);

		return 0;
	}
};