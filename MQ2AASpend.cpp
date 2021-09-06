// MQ2AASpend.cpp : Redguides exclusive ini based auto AA purchaser
// v1.0 - Sym - 04-23-2012
// v1.1 - Sym - 05-26-2012 - Updated to spends aa on normal level ding and basic error checking on ini string
// v2.0 - Eqmule 11-23-2015 - Updated to not rely on the UI or macrocalls.
// v2.1 - Eqmule 04-23-2016 - Updated to bruteforce AA in the correct order.
// v2.1 - Eqmule 05-18-2016 - Added a feature to bruteforce Bonus AA first.
// v2.2 - Eqmule 07-22-2016 - Added string safety.
// v2.3 - Eqmule 08-30-2016 - Fixed buy aa so it will take character level into account when buying aa's
// v2.4 - Eqmule 09-25-2016 - Fixed buy aa so it will not try to buy aa's with an ID of 0 (like Hidden Communion of the Cheetah)
// v2.5 - Eqmule 12-17-2016 - Added SpendOrder as a ini setting. Defualt is still class,focus,arch,gen,special.
// v2.6 - Sic    01-04-2020 - Updated Bonus to reflect AutoGrant as of 12/19/2019 gives autogrant up to and including TBM, xpac #22

#include <mq/Plugin.h>

PreSetup("MQ2AASpend");
PLUGIN_VERSION(2.6);

// The Alternate Ability length converted to decimal is length 10 for a total of 20ish chars, add some padding in case that changes.
constexpr int MAX_BUYLINE = 32;

struct AAInfo
{
	std::string name;
	std::string level;

	AAInfo(std::string name_, std::string level_)
		: name(std::move(name_)), level(std::move(level_)) {}
};
std::vector<AAInfo> vAAList;

bool bAutoSpend = false;
bool bAutoSpendNow = false;
bool bBruteForce = false;
bool bBruteForceBonusFirst = false;
bool bInitDone = false;
bool bDebug = false;
int iBankPoints = 0;
std::string SpendOrderString = "35214"; // default order is: class, focus, arch, gen, special
std::vector<int> SpendOrder = { 3, 5, 2, 1, 4 };
int curAutoSpendIndex = 0;
int curBruteForceIndex = 0;
int doBruteForce = 0;
int doBruteForceBonusFirst = 0;

// AutoGrant is now an "Automatic Script" per DBG current expansion - 4.
unsigned int iAutoGrantExpansion = NUM_EXPANSIONS - 4;
bool doAutoSpend = false;
int iAbilityToPurchase = 0;
int tCount = 0;

void ShowHelp()
{
	WriteChatf("\atMQ2AASpend :: v%1.2f :: by Sym\ax", MQ2Version);
	WriteChatf("/aaspend :: Lists command syntax");
	if (bInitDone)
	{
		WriteChatf("/aaspend status :: Shows current status");
		WriteChatf("/aaspend add \"AA Name\" maxlevel :: Adds AA Name to ini file, will not purchase past max level. Use M to specify max level");
		WriteChatf("/aaspend del \"AA Name\" :: Deletes AA Name from ini file");
		WriteChatf("/aaspend buy \"AA Name\" :: Buys a particular AA. Ignores bank setting.");
		WriteChatf("/aaspend brute on|off|now :: Buys first available ability on ding, or now if specified. Default \ar*OFF*\ax");
		WriteChatf("/aaspend bonus on|off|now :: Buys first available ability BONUS AA on ding, or now if specified. Default \ar*OFF*\ax");
		WriteChatf("/aaspend auto on|off|now :: Autospends AA's based on ini on ding, or now if specified. Default \ag*ON*\ax");
		WriteChatf("/aaspend bank # :: Keeps this many AA points banked before auto/brute spending. Default \ay0\ax.");
		WriteChatf("/aaspend order ##### :: Sets the Spend Order.\ay Default is 35214 (class, focus, arch, gen, special)\ax.");
	}
}

void ShowStatus()
{
	WriteChatf("\atMQ2AASpend :: Status\ax");
	WriteChatf("Brute Force Mode: %s", bBruteForce ? "\agon\ax" : "\aroff\ax");
	WriteChatf("Brute Force Bonus AA First: %s", bBruteForceBonusFirst ? "\agon\ax" : "\aroff\ax");
	WriteChatf("Auto Spend Mode: %s", bAutoSpend ? "\agon\ax" : "\aroff\ax");
	WriteChatf("Banking \ay%d\ax points", iBankPoints);
	WriteChatf("Spend Order \ag%s\ax", SpendOrderString.c_str());
	WriteChatf("INI has \ay%d\ax abilities listed", vAAList.size());
}

void UpdateSpendOrder()
{
	SpendOrder.clear();
	if (SpendOrderString.empty())
	{
		SpendOrder = { 3, 5, 2, 1, 4 };
		return;
	}

	for (char ch : SpendOrderString)
	{
		int val = ch - '0';

		if (val >= 0 && val <= 10)
			SpendOrder.push_back(val);
	}
}

void Update_INIFileName()
{
	sprintf_s(INIFileName, "%s\\%s_%s.ini", gPathConfig, EQADDR_SERVERNAME, GetCharInfo()->Name);
}

void SaveINI()
{
	Update_INIFileName();

	// write settings
	std::string sectionName = "MQ2AASpend_Settings";
	WritePrivateProfileSection(sectionName, "", INIFileName);
	WritePrivateProfileBool(sectionName, "AutoSpend", bAutoSpend, INIFileName);
	WritePrivateProfileBool(sectionName, "BruteForce", bBruteForce, INIFileName);
	WritePrivateProfileBool(sectionName, "BruteForceBonusFirst", bBruteForceBonusFirst, INIFileName);
	WritePrivateProfileInt(sectionName, "BankPoints", iBankPoints, INIFileName);
	WritePrivateProfileString(sectionName, "SpendOrder", SpendOrderString, INIFileName);

	// write all abilites
	sectionName = "MQ2AASpend_AAList";
	WritePrivateProfileSection(sectionName, "", INIFileName);

	for (size_t a = 0; a < vAAList.size(); a++)
	{
		AAInfo& info = vAAList[a];

		WritePrivateProfileString(sectionName, std::to_string(a),
			fmt::format("{}|{}", info.name, info.level), INIFileName);
	}

	if (bInitDone) WriteChatf("\atMQ2AASpend :: Settings updated\ax");
}

void LoadINI()
{
	Update_INIFileName();

	// get settings
	std::string sectionName = "MQ2AASpend_Settings";
	bAutoSpend = GetPrivateProfileBool(sectionName, "AutoSpend", true, INIFileName);
	bBruteForce = GetPrivateProfileBool(sectionName, "BruteForce", false, INIFileName);
	bBruteForceBonusFirst = GetPrivateProfileBool(sectionName, "BruteForceBonusFirst", false, INIFileName);
	iBankPoints = GetPrivateProfileInt(sectionName, "BankPoints", 0, INIFileName);
	SpendOrderString = GetPrivateProfileString(sectionName, "SpendOrder", "35214", INIFileName);
	UpdateSpendOrder();

	// get all abilites
	std::vector<std::string> AAList = GetPrivateProfileKeys("MQ2AASpend_AAList", INIFileName);

	// clear lists
	vAAList.clear();
	int count = 0;

	// loop through all entries under _AAList
	// values are terminated by \0, final value is teminated with \0\0
	// values look like
	// 1=Gate|M
	// 2=Innate Agility|5
	for (const std::string& keyName : AAList)
	{
		std::string item = GetPrivateProfileString("MQ2AASpend_AAList", keyName, std::string(), INIFileName);

		// split entries on =
		std::vector<std::string_view> pieces = mq::split_view(item, '|');

		if (pieces.empty())
		{
			WriteChatf("\arMQ2AASpend :: %s=%s isn't in valid format, skipping...\ax", keyName.c_str(), item.c_str());
			continue;
		}

		if (count++ > 500)
		{
			WriteChatf("\arMQ2AASpend :: There is a limit of 500 abilities in the ini file. Remove some maxxed entries if you want to add new abilities.\ax");
			break;
		}

		for (size_t i = 0; i < pieces.size();)
		{
			// Odd entries are the names. Add it to the list
			std::string_view name = pieces[i++];

			// next is maxlevel of ability. Add it to the level list.
			if (i < pieces.size())
			{
				std::string_view level = pieces[i++];
				vAAList.emplace_back(std::string(name), std::string(level));
			}
		}
	}

	// flag first load init as done
	SaveINI();
	bInitDone = true;
}

void SpendFromINI()
{
	if (!pLocalPlayer) return;
	if (!bAutoSpendNow && GetPcProfile()->AAPoints < iBankPoints) return;
	if (vAAList.empty()) return;

	bool bBuy = true;
	int aaType = 0;
	int aaCost = 0;
	int curLevel = 0;
	int maxLevel = 0;
	int level = pLocalPlayer->Level;

	for (const AAInfo& info : vAAList)
	{
		const std::string& vRef = info.name;
		const std::string& vLevelRef = info.level;

		if (bDebug)
			WriteChatf("MQ2AASpend :: Have %s from ini, checking...", vRef.c_str());
		bBuy = true;

		int index = GetAAIndexByName((PCHAR)vRef.c_str());
		if (CAltAbilityData* pAbility = GetAAByIdWrapper(index, level))
		{
			aaType = pAbility->Type;
			if (!aaType)
			{
				if (bDebug)
					WriteChatf("MQ2AASpend :: %s is \arNOT available\ax for purchase.", vRef.c_str());
				continue;
			}

			aaCost = pAbility->Cost;
			curLevel = pAbility->CurrentRank;
			maxLevel = pAbility->MaxRank;

			if (CAltAbilityData* pNextAbility = GetAAByIdWrapper(pAbility->NextGroupAbilityId, level))
				pAbility = pNextAbility;

			if (!pAltAdvManager->CanTrainAbility(pLocalPC, pAbility))
			{
				if (bDebug)
					WriteChatf("MQ2AASpend :: %s is \arNOT available\ax for purchase.", vRef.c_str());
				continue;
			}

			if (bDebug)
				WriteChatf("MQ2AASpend :: %s is \agavailable\ax for purchase. Cost is %d, tab is %d", vRef.c_str(), aaCost, aaType);
			if (bDebug)
				WriteChatf("MQ2AASpend :: %s current level is %d", vRef.c_str(), curLevel);

			if (curLevel == maxLevel)
			{
				if (bDebug)
					WriteChatf("MQ2AASpend :: %s is currently maxxed, skipping", vRef.c_str());
				continue;
			}
			else
			{
				if (!_stricmp(vLevelRef.c_str(), "M"))
				{
					bBuy = true;
				}
				else
				{
					int b = GetIntFromString(vLevelRef.c_str(), MAX_PC_LEVEL + 1);
					if (curLevel >= b)
					{
						if (bDebug)
							WriteChatf("MQ2AASpend :: %s max level has been reached per ini setting, skipping", vRef.c_str());
						continue;
					}
				}
			}

			if (GetPcProfile()->AAPoints >= aaCost)
			{
				if (bBuy)
				{
					WriteChatf("MQ2AASpend :: Attempting to purchase level %d of %s for %d point%s.", curLevel + 1, vRef.c_str(), aaCost, aaCost > 1 ? "s" : "");
					if (!bDebug)
					{
						char szCommand[MAX_BUYLINE] = { 0 };
						sprintf_s(szCommand, "/alt buy %d", pAbility->ID);

						DoCommand(GetCharInfo()->pSpawn, szCommand);
					}
					else
					{
						WriteChatf("MQ2AASpend :: Debugging so I wont actually buy %s", vRef.c_str());
					}

					break;
				}
			}
			else
			{
				WriteChatf("MQ2AASpend :: Not enough points to buy %s, skipping", vRef.c_str());
			}
		}
	}
}

std::map<DWORD, CAltAbilityData*> generalmap;   // 1
std::map<DWORD, CAltAbilityData*> archtypemap;  // 2
std::map<DWORD, CAltAbilityData*> classmap;     // 3
std::map<DWORD, CAltAbilityData*> specialmap;   // 4
std::map<DWORD, CAltAbilityData*> focusmap;     // 5

std::map<DWORD, CAltAbilityData*>& GetMapByOrder(int order)
{
	switch (order)
	{
	case 1:
		return generalmap;
	case 2:
		return archtypemap;
	case 3:
		return classmap;
	case 4:
		return specialmap;
	case 5:
		return focusmap;
	default:
		return classmap;
	}
}

CAltAbilityData* GetFirstPurchasableAA(bool bBonus, bool bNext)
{
	if (!pLocalPlayer || !pLocalPC) return nullptr;

	generalmap.clear();
	archtypemap.clear();
	classmap.clear();
	specialmap.clear();
	focusmap.clear();

	int level = pLocalPlayer->Level;
	CAltAbilityData* pPrevAbility = nullptr;

	for (int nAbility = 0; nAbility < NUM_ALT_ABILITIES; nAbility++)
	{
		if (CAltAbilityData* pAbility = GetAAByIdWrapper(nAbility, level))
		{
			pPrevAbility = pAbility;
			CAltAbilityData* pNextAbility = GetAAByIdWrapper(pAbility->NextGroupAbilityId, level);
			if (pNextAbility)
				pAbility = pNextAbility;

			if (bBonus)
			{
				if (pAbility->Expansion <= iAutoGrantExpansion)
					continue;
			}

			// Check if we can train it.
			if (pAltAdvManager->CanTrainAbility(pLocalPC, pAbility))
			{
				if (pAbility->ID == 0)
					continue;// hidden communion of the cheetah is not valid and has this id, but is still sent down to the client

				if (bDebug)
				{
					if (const char* AAName = pDBStr->GetString(pAbility->nName, eAltAbilityName))
					{
						WriteChatf("Adding \ay%s\ax (expansion:\ag%d\ax) to the \ao%d\ax map", AAName, pAbility->Expansion, pAbility->Type);
					}
				}

				if (bNext)
					pPrevAbility = pAbility;

				switch (pAbility->Type)
				{
				case 1:
					generalmap[pAbility->ID] = pPrevAbility;
					break;
				case 2:
					archtypemap[pAbility->ID] = pPrevAbility;
					break;
				case 3:
					classmap[pAbility->ID] = pPrevAbility;
					break;
				case 4:
					specialmap[pAbility->ID] = pPrevAbility;
					break;
				case 5:
					focusmap[pAbility->ID] = pPrevAbility;
					break;
				default:
					WriteChatf("Type %d Not Found mq2aaspend in GetFirstPurchasableAA.", pAbility->Type);
					break;
				}
			}
		}
	}

	// spend in the specified order:
	for (int order : SpendOrder)
	{
		std::map<DWORD, CAltAbilityData*>& retmap = GetMapByOrder(order);
		if (!retmap.empty())
		{
			return retmap.begin()->second;
		}
	}

	if (!bBruteForce)
		WriteChatf("I couldn't find any AA's to purchase, maybe you have them all?");

	return nullptr;
}

bool BuySingleAA(CAltAbilityData* pAbility, bool bDryRun)
{
	if (!pLocalPlayer) return false;
	if (!pAbility) return false;

	int aaType = 0;
	int aaCost = 0;
	int level = pLocalPlayer->Level;

	int curLevel = 0;

	if (const char* AAName = pDBStr->GetString(pAbility->nName, eAltAbilityName))
	{
		aaType = pAbility->Type;
		if (!aaType)
		{
			WriteChatf("MQ2AASpend :: Unable to purchase %s", AAName);
			return false;
		}

		if (CAltAbilityData* pNextAbility = GetAAByIdWrapper(pAbility->NextGroupAbilityId, level))
			pAbility = pNextAbility;

		curLevel = pAbility->CurrentRank;
		aaCost = pAbility->Cost;

		if (!pAltAdvManager->CanTrainAbility(pLocalPC, pAbility))
		{
			WriteChatf("MQ2AASpend :: You can't train %s for some reason, aborting", AAName);
			return false;
		}

		if (GetPcProfile()->AAPoints >= aaCost)
		{
			WriteChatf("MQ2AASpend :: Attempting to purchase level %d of %s for %d point%s.", curLevel, AAName, aaCost, aaCost > 1 ? "s" : "");

			if (!bDryRun)
			{
				char szCommand[MAX_BUYLINE] = { 0 };
				sprintf_s(szCommand, "/alt buy %d", pAbility->ID);

				DoCommand(GetCharInfo()->pSpawn, szCommand);
			}
			else
			{
				WriteChatf("MQ2AASpend :: Debugging so I wont actually buy %s", AAName);
			}

			return true;
		}
		else
		{
			WriteChatf("MQ2AASpend :: Not enough points to buy %s, aborting", AAName);
		}
	}

	return false;
}

void BruteForcePurchase(int mode, bool bonusMode)
{
	if (mode != 2 && GetPcProfile()->AAPoints < iBankPoints)
		return;

	DebugSpew("MQ2AASpend :: Starting Brute Force Purchase");

	if (CAltAbilityData* pBruteAbility = GetFirstPurchasableAA(bonusMode, false))
	{
		if (const char* AAName = pDBStr->GetString(pBruteAbility->nName, eAltAbilityName))
		{
			BuySingleAA(pBruteAbility, bDebug);
		}
	}

	DebugSpew("MQ2AASpend :: Brute Force Purchase Complete");
}

PLUGIN_API void SetGameState(int GameState)
{
	if (GameState == GAMESTATE_INGAME)
	{
		if (!bInitDone) LoadINI();
	}
	else if (GameState != GAMESTATE_LOGGINGIN)
	{
		if (bInitDone) bInitDone = false;
	}
}

void SpendCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	CHAR szArg1[MAX_STRING] = { 0 };
	CHAR szArg2[MAX_STRING] = { 0 };
	CHAR szArg3[MAX_STRING] = { 0 };
	GetArg(szArg1, szLine, 1);
	GetArg(szArg2, szLine, 2);
	GetArg(szArg3, szLine, 3);

	if (!_strcmpi(szArg1, ""))
	{
		ShowHelp();
		return;
	}

	if (!_strcmpi(szArg1, "debug"))
	{
		if (!_strcmpi(szArg2, "on"))
		{
			bDebug = true;
		}
		else if (!_strcmpi(szArg2, "off"))
		{
			bDebug = false;
		}

		WriteChatf("MQ2AASpend :: Debug is %s", bDebug ? "\agON\ax" : "\arOFF\ax");
		return;
	}

	if (!_strcmpi(szArg1, "status"))
	{
		ShowStatus();
		return;
	}

	if (!_strcmpi(szArg1, "load"))
	{
		LoadINI();
		return;
	}

	if (!_strcmpi(szArg1, "save"))
	{
		SaveINI();
		return;
	}

	if (!_strcmpi(szArg1, "brute"))
	{
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend brute on|off|now");
			WriteChatf("       This will auto buy AA's based on ini values on next ding, or now if now specified.");
		}
		else if (!_strcmpi(szArg2, "on"))
		{
			WriteChatf("MQ2AASpend :: Brute Force Mode Enabled -- Buying AA's in order of appearence, from class to general, then special");
			bBruteForce = true;
			bBruteForceBonusFirst = false;
			bAutoSpend = false;
		}
		else if (!_strcmpi(szArg2, "off"))
		{
			WriteChatf("MQ2AASpend :: Brute Force Mode Disabled");
			bBruteForce = false;
		}
		else if (!_strcmpi(szArg2, "now"))
		{
			WriteChatf("MQ2AASpend :: Brute Force Mode Active Now");
			doBruteForce = 2;
		}

		return;
	}

	if (!_strcmpi(szArg1, "bonus"))
	{
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend bonus on|off|now");
			WriteChatf("       This will auto buy AA's based on bonusAA first, in other words, from expansions post autogrant only.");
		}
		else if (!_strcmpi(szArg2, "on"))
		{
			WriteChatf("MQ2AASpend :: Brute Force Bonus Mode Enabled -- Buying AA's from expansions post autogrant only.");

			bBruteForce = false;
			bBruteForceBonusFirst = true;
			bAutoSpend = false;
		}
		else if (!_strcmpi(szArg2, "off"))
		{
			WriteChatf("MQ2AASpend :: Brute Force Bonus Mode Disabled");

			bBruteForceBonusFirst = false;
		}
		else if (!_strcmpi(szArg2, "now"))
		{
			WriteChatf("MQ2AASpend :: Brute Force Bonus Mode Active Now");

			doBruteForceBonusFirst = 2;
		}

		return;
	}

	if (!_strcmpi(szArg1, "auto"))
	{
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend auto on|off|now");
			WriteChatf("       This will auto buy AA's based on ini values on next ding, or now if now specified.");
		}
		else if (!_strcmpi(szArg2, "on"))
		{
			WriteChatf("MQ2AASpend :: Auto Mode Enabled -- Buying AA's based on ini values");

			bAutoSpend = true;
			bBruteForce = false;
			bBruteForceBonusFirst = false;
			bAutoSpendNow = false;
		}
		else if (!_strcmpi(szArg2, "off"))
		{
			WriteChatf("MQ2AASpend :: Auto Mode Disabled");

			bAutoSpend = false;
			bAutoSpendNow = false;
		}
		else if (!_strcmpi(szArg2, "now"))
		{
			WriteChatf("MQ2AASpend :: Auto Mode Active Now");

			if (!vAAList.empty())
			{
				doAutoSpend = true;
				bAutoSpendNow = true;
			}
		}

		return;
	}

	if (!_strcmpi(szArg1, "bank"))
	{
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend bank #");
			WriteChatf("       This will keep a minimum of # aa points before auto spending or brute force spending.");
		}
		else
		{
			iBankPoints = GetIntFromString(szArg2, 0);
			WriteChatf("MQ2AASpend :: Banking %d points", iBankPoints);

			SaveINI();
		}

		return;
	}

	if (!_strcmpi(szArg1, "order"))
	{
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend order \ag#####\ax where ##### is 5 numbers representing 5 tabs in the aa window.");
			WriteChatf("       General is 1 ArchType is 2 Class is 3 Special is 4 and Focus is 5.");
			WriteChatf("       Example: Lets say you want to spend in this order: Class, Focus, Archtype, General and last Special, you would then type: \ay/aaspend order 35214\ax");
		}
		else
		{
			SpendOrderString = szArg2;
			WriteChatf("MQ2AASpend :: New Spend Order is %s", szArg2);

			UpdateSpendOrder();
			SaveINI();
		}

		return;
	}

	if (!_strcmpi(szArg1, "add"))
	{
		if (!_strcmpi(szArg2, "") || !_strcmpi(szArg3, ""))
		{
			WriteChatf("Usage: /aaspend add \"AA Name\" maxlevel");
			WriteChatf("        maxlevel can be a number or m or M for max available.");
		}
		else
		{
			for (const AAInfo& info : vAAList)
			{
				if (ci_equals(szArg2, info.name))
				{
					WriteChatf("MQ2AASpend :: Ability %s already exists", info.name.c_str());
					return;
				}
			}

			vAAList.emplace_back(szArg2, szArg3);

			WriteChatf("MQ2AASpend :: Added %s levels of %s to ini", szArg3, szArg2);
			SaveINI();
		}

		return;
	}

	if (!_strcmpi(szArg1, "del"))
	{
		int delIndex = -1;
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend del \"AA Name\"");
		}
		else
		{
			auto iter = std::find_if(vAAList.begin(), vAAList.end(), [&](const AAInfo& info) { return ci_equals(szArg2, info.name); });

			if (iter != vAAList.end())
			{
				vAAList.erase(iter);

				WriteChatf("MQ2AASpend :: Deleted ability %s from ini", szArg2);
				SaveINI();
			}
			else
			{
				WriteChatf("MQ2AASpend :: Abilty %s not found in ini", szArg2);
			}
		}

		return;
	}

	if (!_strcmpi(szArg1, "buy"))
	{
		if (!_strcmpi(szArg2, ""))
		{
			WriteChatf("Usage: /aaspend buy \"AA Name\"");
		}
		else if (pLocalPlayer)
		{
			int index = GetAAIndexByName(szArg2);

			if (CAltAbilityData* pAbility = GetAAByIdWrapper(index, pLocalPlayer->Level))
			{
				BuySingleAA(pAbility, false);
			}
		}

		return;
	}
	
	ShowHelp();
}

// This is called every time MQ pulses
PLUGIN_API void OnPulse()
{
	static int Pulse = 0;

	if (GetGameState() != GAMESTATE_INGAME)
		return;

	// Process every 50 pulses
	if (++Pulse < 50)
		return;

	Pulse = 0;

	if (doBruteForce != 0)
	{
		BruteForcePurchase(doBruteForce, false);
		doBruteForce = 0;
	}

	if (doBruteForceBonusFirst != 0)
	{
		BruteForcePurchase(doBruteForce, true);
		doBruteForceBonusFirst = 0;
	}

	if (doAutoSpend && !vAAList.empty())
	{
		SpendFromINI();
		doAutoSpend = false;
		bAutoSpendNow = false;
	}
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if ((strstr(Line, "You have gained") && strstr(Line, "ability point") && strstr(Line, "You now have")) || strstr(Line, "You have reached the AA point cap"))
	{
		if (bBruteForce)
			doBruteForce = 1;
		if (bBruteForceBonusFirst)
			doBruteForceBonusFirst = 1;
		if (bAutoSpend && !vAAList.empty())
			doAutoSpend = true;
	}

	if (strstr(Line, "Welcome to level"))
	{
		if (bBruteForce)
			doBruteForce = 1;
		if (bBruteForceBonusFirst)
			doBruteForceBonusFirst = 1;
		if (bAutoSpend && !vAAList.empty())
			doAutoSpend = true;
	}

	return 0;
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2AASpend");
	AddCommand("/aaspend", SpendCommand);
	ShowHelp();
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2AASpend");
	RemoveCommand("/aaspend");
}
