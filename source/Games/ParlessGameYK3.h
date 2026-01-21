#include "CBaseParlessGameDE.h"

class ParlessGameYK3 : public CBaseParlessGameDE
{
public:
	std::string get_name() override
	{
		return "Yakuza Kiwami 3";
	}

	parless_stringmap get_game_map(Locale locale) override
	{
		std::string curLoc;
		std::vector<const char*> locInfWealthVec{ "de", "en", "es", "fr", "it", "ja", "ko", "zh", "zhs", "zht", "ru", "pt"};

		parless_stringmap result = parless_stringmap();
		result["/entity"] = "/entity_bis";
		result["/ui.bis/texture"] = "/ui.bis.common/texture";

		for (int i = 0; i < locInfWealthVec.size(); i++)
		{
			curLoc = std::string(locInfWealthVec[i]);
			result["/db.bis/" + curLoc] = "/db.bis." + curLoc;
			result["/ui.bis/" + curLoc] = "/ui.bis." + curLoc;
		}

		return result;
	}

	bool hook_add_file() override;

	typedef int (*t_orgYK3AddFileEntry)(short* a1, int a2, char* a3, char** a4);
	typedef int (*t_orgYK3GetEntityPath)(char* in_path, unsigned int e_kind, unsigned int stageid, unsigned int daynight, void* uid);
	
	static t_orgYK3AddFileEntry orgYK3AddFileEntry;
	static t_orgYK3AddFileEntry(*hookYK3AddFileEntry);

	static t_orgYK3GetEntityPath orgYK3GetEntityPath;
	static t_orgYK3GetEntityPath(*hookYK3GetEntityPath);

	static int YK3AddFileEntry(short* a1, int a2, char* a3, char** a4)
	{
		orgYK3AddFileEntry(a1, a2, a3, a4);
		RenameFilePaths((char*)a1);
		return strlen((char*)a1);
	}

	static int YK3GetEntityPath(char* in_path, unsigned int e_kind, unsigned int stageid, unsigned int daynight, __int64* uid)
	{
		orgYK3GetEntityPath(in_path, e_kind, stageid, daynight, uid);
		RenameFilePaths(in_path);

		return 0;
	}
};