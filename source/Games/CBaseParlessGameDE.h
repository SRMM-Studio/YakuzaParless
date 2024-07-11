#pragma once
#include "CBaseParlessGame.h"

class CBaseParlessGameDE : public CBaseParlessGame
{
public:
	const char* modded_ubik_path = "data/../mods/Parless/ubik/\0";
	virtual std::string translate_path(std::string path, int indexOfData) override;

	bool redirectUbik;
};
