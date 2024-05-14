#include "CBaseParlessGameDE.h"

class ParlessGameLJ : public CBaseParlessGameDE
{
public:
	std::string get_name() override
	{
		return "Lost Judgment";
	}

	parless_stringmap get_game_map(Locale locale) override
	{
		std::string curLoc;
		std::vector<const char*> locJudgeVec{ "de", "en", "es", "fr", "it", "ja", "ko", "zh", "zhs" };

		parless_stringmap result = parless_stringmap();
		result["/entity"] = "/entity_coyote";
		result["/ui.coyote/texture"] = "/ui.coyote.common/texture";

		for (int i = 0; i < locJudgeVec.size(); i++)
		{
			curLoc = std::string(locJudgeVec[i]);
			result["/db.coyote/" + curLoc] = "/db.coyote." + curLoc;
			result["/ui.coyote/" + curLoc] = "/ui.coyote." + curLoc;
		}

		return result;
	}

	bool hook_add_file() override;

	typedef int (*t_orgLJAddFileEntry)(short* a1, int a2, char* a3, char** a4);
	typedef int (*t_orgLJGetEntityPath)(char* in_path, unsigned int e_kind, unsigned int stageid, unsigned int daynight, void* uid);

	static t_orgLJAddFileEntry orgLJAddFileEntry;
	static t_orgLJAddFileEntry(*hookLJAddFileEntry);

	static t_orgLJGetEntityPath orgLJGetEntityPath;
	static t_orgLJGetEntityPath(*hookLJGetEntityPath);

	static int LJAddFileEntry(short* a1, int a2, char* a3, char** a4)
	{
		orgLJAddFileEntry(a1, a2, a3, a4);
		RenameFilePaths((char*)a1);
		return strlen((char*)a1);
	}

	static int LJGetEntityPath(char* in_path, unsigned int e_kind, unsigned int stageid, unsigned int daynight, __int64* uid)
	{
		orgLJGetEntityPath(in_path, e_kind, stageid, daynight, uid);
		RenameFilePaths(in_path);

		return 0;
	}
};