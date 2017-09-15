// MQ2AASpend.cpp : NotAddicted exclusive ini based auto AA purchaser
// v1.0 - Sym - 04-23-2012
// v1.1 - Sym - 05-26-2012 - Updated to spends aa on normal level ding and basic error checking on ini string
// v2.0 - Eqmule 11-23-2015 - Updated to not rely on the UI or macrocalls.
// v2.1 - Eqmule 04-23-2016 - Updated to bruteforce AA in the correct order.
// v2.1 - Eqmule 05-18-2016 - Added a feature to bruteforce Bonus AA first.
// v2.2 - Eqmule 07-22-2016 - Added string safety.
// v2.3 - Eqmule 08-30-2016 - Fixed buy aa so it will take character level into account when buying aa's
// v2.4 - Eqmule 09-25-2016 - Fixed buy aa so it will not try to buy aa's with an ID of 0 (like Hidden Communion of the Cheetah)
// v2.5 - Eqmule 12-17-2016 - Added SpendOrder as a ini setting. Defualt is still class,focus,arch,gen,special.

#include "../MQ2Plugin.h"
using namespace std;
#include <vector>
PLUGIN_VERSION(2.5);

PreSetup("MQ2AASpend");
#pragma warning(disable:4018)

// Function Prototypes
#define SKIP_PULSES 50
vector <string> vAAList;
vector <string> vAALevel;
vector <string> vDelayCommand;
vector <long> vDelayTimeout;

CHAR szCommand[MAX_STRING] = {0};

long SkipPulse = 0;
bool bAutoSpend = false;
bool bAutoSpendNow = false;
bool bBruteForce = false;
bool bBruteForceBonusFirst = false;
bool bInitDone = false;
bool bDebug = false;
char iBankPoints[MAX_STRING];
char SpendOrder[MAX_STRING] = { "35214" };//default order is class focus arch gen and last special
char szTemp[MAX_STRING];
char szItem[MAX_STRING];
char szList[MAX_STRING];
int curAutoSpendIndex = 0;
int curBruteForceIndex = 0;
int doBruteForce = 0;
int doBruteForceBonusFirst = 0;

bool doAutoSpend = false;
int iAbilityToPurchase = 0;
int tCount = 0;


void ShowHelp() {
    WriteChatf("\atMQ2AASpend :: v1.0 :: by Sym for NotAddicted.com\ax");
    WriteChatf("\atMQ2AASpend :: v%1.2f :: by Eqmule\ax", MQ2Version);
    WriteChatf("/aaspend :: Lists command syntax");
    WriteChatf("/aaspend status :: Shows current status");
    WriteChatf("/aaspend add \"AA Name\" maxlevel :: Adds AA Name to ini file, will not purchase past max level. Use M to specify max level");
    WriteChatf("/aaspend del \"AA Name\" :: Deletes AA Name from ini file");
    WriteChatf("/aaspend buy \"AA Name\" :: Buys a particular AA. Ignores bank setting.");
    WriteChatf("/aaspend brute on|off|now :: Buys first available ability on ding, or now if specified. Default \ar*OFF*\ax");
    WriteChatf("/aaspend bonus on|off|now :: Buys first available ability BONUS AA on ding, or now if specified. Default \ar*OFF*\ax");
    WriteChatf("/aaspend auto on|off|now :: Autospends AA's based on ini on ding, or now if specified. Default \ag*ON*\ax");
    WriteChatf("/aaspend bank # :: Keeps this many AA points banked before auto/brute spending. Default \ay0\ax.");
	WriteChatf("/aaspend order ##### :: Sets the Spend Order.\ayDefault is 35214 (class,focus,arch,gen,special)\ax.");
    return;
}

void ShowStatus() {
    WriteChatf("\atMQ2AASpend :: Status\ax");
    WriteChatf("Brute Force Mode: %s",bBruteForce?"\agon\ax":"\aroff\ax");
    WriteChatf("Brute Force Bonus AA First: %s",bBruteForceBonusFirst?"\agon\ax":"\aroff\ax");
    WriteChatf("Auto Spend Mode: %s",bAutoSpend?"\agon\ax":"\aroff\ax");
    WriteChatf("Banking \ay%s\ax points", iBankPoints);
    WriteChatf("Spend Order \ag%s\ax", SpendOrder);
    WriteChatf("INI has \ay%d\ax abilities listed", vAAList.size());
}

void Update_INIFileName() {
    sprintf_s(INIFileName,"%s\\%s_%s.ini",gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
}

VOID SaveINI(VOID) {
    char szKey[MAX_STRING];
    Update_INIFileName();
    // write settings
    sprintf_s(szTemp,"MQ2AASpend_Settings");
    WritePrivateProfileSection(szTemp, "", INIFileName);
    WritePrivateProfileString(szTemp,"AutoSpend",bAutoSpend ? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"BruteForce",bBruteForce ? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"BruteForceBonusFirst",bBruteForceBonusFirst ? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"BankPoints",iBankPoints,INIFileName);
    WritePrivateProfileString(szTemp,"SpendOrder",SpendOrder,INIFileName);


    // write all abilites
    sprintf_s(szTemp,"MQ2AASpend_AAList");
    WritePrivateProfileSection(szTemp, "", INIFileName);
    if (vAAList.size()) {
        for (unsigned int a = 0; a < vAAList.size(); a++) {
            string& vRef = vAAList[a];
            string& vLevelRef = vAALevel[a];
            sprintf_s(szItem,"%s|%s",vRef.c_str(),vLevelRef.c_str());
            sprintf_s(szKey,"%d",a);
            WritePrivateProfileString(szTemp,szKey,szItem,INIFileName);
        }
    }
if (bInitDone) WriteChatf("\atMQ2AASpend :: Settings updated\ax");
}

VOID LoadINI(VOID) {

	Update_INIFileName();
	// get settings
	sprintf_s(szTemp, "MQ2AASpend_Settings");
	bAutoSpend = GetPrivateProfileInt(szTemp, "AutoSpend", 1, INIFileName) > 0 ? true : false;
	bBruteForce = GetPrivateProfileInt(szTemp, "BruteForce", 0, INIFileName) > 0 ? true : false;
	bBruteForceBonusFirst = GetPrivateProfileInt(szTemp, "BruteForceBonusFirst", 0, INIFileName) > 0 ? true : false;
	GetPrivateProfileString(szTemp, "BankPoints", "0", iBankPoints, MAX_STRING, INIFileName);
	GetPrivateProfileString(szTemp, "SpendOrder", "35214", SpendOrder, MAX_STRING, INIFileName);

	// get all abilites
	sprintf_s(szTemp, "MQ2AASpend_AAList");
	//GetPrivateProfileSection(szTemp,szList,MAX_STRING,INIFileName);
	GetPrivateProfileString(szTemp, NULL, "0", szList, MAX_STRING, INIFileName);


	// clear lists
	vAAList.clear();
	vAALevel.clear();

	char *p = (char*)szList;
	char *pch = 0;
	char *Next_Token1 = 0;
	size_t length = 0;
	int i = 0;
	// loop through all entries under _AAList
	// values are terminated by \0, final value is teminated with \0\0
	// values look like
	// 1=Gate|M
	// 2=Innate Agility|5
	if (strlen(p)) {
		while (*p) {
			length = strlen(p);
			GetPrivateProfileString(szTemp, p, "", szItem, MAX_STRING, INIFileName);
			// split entries on =
			if (!strstr(szItem, "|")) {
				WriteChatf("\arMQ2AASpend :: %s=%s isn't in valid format, deleting...\ax", p, szItem);
				p += length;
				p++;
				continue;
			}
			pch = strtok_s(szItem, "|", &Next_Token1);
			if (i++ > 500) {
				WriteChatf("\arMQ2AASpend :: There is a limit of 500 abilities in the ini file. Remove some maxxed entries if you want to add new abilities.\ax");
				break;
			}
			while (pch != NULL) {
				DebugSpew("pch = %s", pch);
				// Odd entries are the names. Add it to the list
				vAAList.push_back(pch);

				// next is maxlevel of ability. Add it to the level list.
				pch = strtok_s(NULL, "|", &Next_Token1);
				vAALevel.push_back(pch);

				// next name
				pch = strtok_s(NULL, "|", &Next_Token1);
			}
			p += length;
			p++;
		}
	}

	// flag first load init as done
	SaveINI();
	if (!bInitDone) ShowStatus();
	bInitDone = true;
}
bool CheckWindowValue(PCHAR szToCheck) {
	sprintf_s(szTemp, "%s", szToCheck);
	ParseMacroData(szTemp, sizeof(szTemp));
	//WriteChatf("%s :: %s", szToCheck, szTemp);
	if (strcmp(szTemp, "TRUE") == 0) return true;
	else if (strcmp(szTemp, "NULL") != 0) return false;
	else if (strcmp(szTemp, "NULL") == 0) return true;
	else return false;
}

void SpendFromINI() {

	if (!bAutoSpendNow && GetCharInfo2()->AAPoints < atoi(iBankPoints)) return;
	if (!vAAList.size()) return;

	bool bBuy = true;
	//    char *pch;
	int aaType = 0;
	int aaCost = 0;
	int curLevel = 0;
	int maxLevel = 0;
	int baseDelay = 0;
	int level = -1;
	if(PSPAWNINFO pMe = (PSPAWNINFO)pLocalPlayer) {
		level = pMe->Level;
	}
	for (unsigned int a = 0; a < vAAList.size(); a++) {
		string& vRef = vAAList[a];
		string& vLevelRef = vAALevel[a];
		if (bDebug)
			WriteChatf("MQ2AASpend :: Have %s from ini, checking...", vRef.c_str());
		bBuy = true;
		
		int index = GetAAIndexByName((PCHAR)vRef.c_str());
		if (PALTABILITY pAbility = GetAAByIdWrapper(index, level)) {
			aaType = pAbility->Type;
			if (!aaType) {
				if (bDebug)
					WriteChatf("MQ2AASpend :: %s is \arNOT available\ax for purchase.", vRef.c_str());
				continue;
			}
			aaCost = pAbility->Cost;
			curLevel = pAbility->CurrentRank;
			maxLevel = pAbility->MaxRank;

			PALTABILITY pNextAbility = GetAAByIdWrapper(pAbility->NextGroupAbilityId, level);
			if (pNextAbility)
				pAbility = pNextAbility;
			bool cantrain = pAltAdvManager->CanTrainAbility((PcZoneClient*)pPCData, (CAltAbilityData*)pAbility, 0, 0, 0);
			if (!cantrain) {
				if (bDebug)
					WriteChatf("MQ2AASpend :: %s is \arNOT available\ax for purchase.", vRef.c_str());
				continue;
			}
			if (bDebug)
				WriteChatf("MQ2AASpend :: %s is \agavailable\ax for purchase. Cost is %d, tab is %d", vRef.c_str(), aaCost, aaType);
			if (bDebug)
				WriteChatf("MQ2AASpend :: %s current level is %d", vRef.c_str(), curLevel);
			if (curLevel == maxLevel) {
				if (bDebug)
					WriteChatf("MQ2AASpend :: %s is currently maxxed, skipping", vRef.c_str());
				continue;
			} else {
				if (!_stricmp(vLevelRef.c_str(), "M")) {
					bBuy = true;
				} else {
					int a = atoi(vLevelRef.c_str());
					if (curLevel >= a) {
						if (bDebug)
							WriteChatf("MQ2AASpend :: %s max level has been reached per ini setting, skipping", vRef.c_str());
						continue;
					}
				}
			}
			if (GetCharInfo2()->AAPoints >= aaCost) {
				if (bBuy) {
					WriteChatf("MQ2AASpend :: Attempting to purchase level %d of %s for %d point%s.", curLevel + 1, vRef.c_str(), aaCost, aaCost > 1 ? "s" : "");
					if (!bDebug) {
						sprintf_s(szCommand, "/alt buy %d", pAbility->Index);
						DoCommand(GetCharInfo()->pSpawn, szCommand);
					}
					else {
						WriteChatf("MQ2AASpend :: Debugging so I wont actually buy %s", vRef.c_str());
					}
					break;
				}
			}
			else {
				WriteChatf("MQ2AASpend :: Not enough points to buy %s, skipping", vRef.c_str());
			}
		}
    }
    return;
}
std::map<DWORD,PALTABILITY>generalmap;//1
std::map<DWORD,PALTABILITY>archtypemap;//2
std::map<DWORD,PALTABILITY>classmap;//3
std::map<DWORD,PALTABILITY>specialmap;//4
std::map<DWORD,PALTABILITY>focusmap;//5

std::map<DWORD,PALTABILITY>&GetMapByOrder(int order)
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
PALTABILITY GetFirstPurchasableAA(bool bBonus)
{
	generalmap.clear();
	archtypemap.clear();
	classmap.clear();
	specialmap.clear();
	focusmap.clear();
	int level = -1;
	if(PSPAWNINFO pMe = (PSPAWNINFO)pLocalPlayer) {
		level = pMe->Level;
	}
	for (unsigned long nAbility = 0; nAbility < NUM_ALT_ABILITIES; nAbility++)
	{
		if (PALTABILITY pAbility = GetAAByIdWrapper(nAbility, level)) {
			PALTABILITY pNextAbility = GetAAByIdWrapper(pAbility->NextGroupAbilityId, level);
			if (pNextAbility)
				pAbility = pNextAbility;
			bool cantrain = pAltAdvManager->CanTrainAbility((PcZoneClient*)pPCData, (CAltAbilityData*)pAbility, 0, 0, 0);
			if (cantrain) {
				if (bBonus) {
					if (pAbility->Expansion < 18)//VoA
						continue;
				}
				if (pAbility->ID == 0)
					continue;//hidden communion of the cheetah is not valid and has this id, but is still sent down to the client
				if (bDebug) {
					if (char *AAName = pCDBStr->GetString(pAbility->nName, 1, NULL)) {
						WriteChatf("Adding %s (expansion:%d) to the %d map", AAName, pAbility->Expansion, pAbility->Type);
					}
				}
				switch (pAbility->Type)
				{
				case 1:
					generalmap[pAbility->ID] = pAbility;
					break;
				case 2:
					archtypemap[pAbility->ID] = pAbility;
					break;
				case 3:
					classmap[pAbility->ID] = pAbility;
					break;
				case 4:
					specialmap[pAbility->ID] = pAbility;
					break;
				case 5:
					focusmap[pAbility->ID] = pAbility;
					break;
				default:
					WriteChatf("Type %d Not Found mq2aaspend in GetFirstPurchasableAA.", pAbility->Type);
					break;
				}
			}
		}
	}
	//spend in this order:
	//class, focus, archtype, general, special
	std::string sorder = SpendOrder;
	int order[5];
	std::string temp;
	int j = 0;
	for (std::string::iterator i = sorder.begin(); i != sorder.end(); i++,j++) {
		temp = (*i);
		order[j] = atoi(temp.c_str());
	}
	for (int i = 0; i < 5; i++) {
		std::map<DWORD, PALTABILITY>&retmap = GetMapByOrder(order[i]);
		if (retmap.size())
			return retmap.begin()->second;
	}
	WriteChatf("I couldn't find any AA's to purchase, maybe you have them all?");
	return NULL;
}

void BuySingleAA(PALTABILITY pBruteAbility) {

    int baseDelay = 0;  
    int aaType = 0;
    int aaCost = 0;
	int level = -1;
	if(PSPAWNINFO pMe = (PSPAWNINFO)pLocalPlayer) {
		level = pMe->Level;
	}
    int curLevel = 0;
	if (PALTABILITY pAbility = pBruteAbility) {
		if (char *AAName = pCDBStr->GetString(pBruteAbility->nName, 1, NULL)) {
			aaType = pAbility->Type;
			if (!aaType) {
				WriteChatf("MQ2AASpend :: Unable to purchase %s", AAName);
				return;
			}
			aaCost = pAbility->Cost;
			curLevel = pAbility->CurrentRank;
			PALTABILITY pNextAbility = GetAAByIdWrapper(pAbility->NextGroupAbilityId, level);
			if (pNextAbility)
				pAbility = pNextAbility;
			bool cantrain = pAltAdvManager->CanTrainAbility((PcZoneClient*)pPCData, (CAltAbilityData*)pAbility, 0, 0, 0);
			if (!cantrain) {
				WriteChatf("MQ2AASpend :: You can't train %s for some reason , aborting", AAName);
				return;
			}
			if (GetCharInfo2()->AAPoints >= aaCost) {
				WriteChatf("MQ2AASpend :: Attempting to purchase level %d of %s for %d point%s.", curLevel + 1, AAName, aaCost, aaCost > 1 ? "s" : "");
				if (!bDebug) {
					sprintf_s(szCommand, "/alt buy %d", pAbility->Index);
					DoCommand(GetCharInfo()->pSpawn, szCommand);
				}
				else {
					WriteChatf("MQ2AASpend :: Debugging so I wont actually buy %s", AAName);
				}
			}
			else {
				WriteChatf("MQ2AASpend :: Not enough points to buy %s, aborting", AAName);
			}
		}
	}
}

void BruteForceBonusFirstPurchase(int mode)
{
	if (mode!=2 && GetCharInfo2()->AAPoints < atoi(iBankPoints))
		return;
	DebugSpew("MQ2AASpend :: Starting Brute Force Bonus First Purchase");
	int baseDelay = 0;
	bool buyAttempt = false;
	CHAR szCommand[MAX_STRING] = { 0 };
	if (PALTABILITY pBruteAbility = GetFirstPurchasableAA(true)) {
		if (char *AAName = pCDBStr->GetString(pBruteAbility->nName, 1, NULL)) {
			if (!bDebug) {
				BuySingleAA(pBruteAbility);
			}
			else {
				WriteChatf("DEBUG: Buying(not really) %s",AAName);
			}
		}
	}
	DebugSpew("MQ2AASpend :: Brute Force Purchase Complete");
}
void BruteForcePurchase(int mode)
{

	if (mode!=2 && GetCharInfo2()->AAPoints < atoi(iBankPoints))
		return;
	DebugSpew("MQ2AASpend :: Starting Brute Force Purchase");
	int baseDelay = 0;
	bool buyAttempt = false;
	CHAR szCommand[MAX_STRING] = { 0 };
	if (PALTABILITY pBruteAbility = GetFirstPurchasableAA(false)) {
		if (char *AAName = pCDBStr->GetString(pBruteAbility->nName, 1, NULL)) {
			if (!bDebug) {
				BuySingleAA(pBruteAbility);
			}
			else {
				WriteChatf("DEBUG: Buying(not really) %s",AAName);
			}
		}
	}
	DebugSpew("MQ2AASpend :: Brute Force Purchase Complete");
}

PLUGIN_API VOID SetGameState(DWORD GameState) {
    if(GameState==GAMESTATE_INGAME) {
        if (!bInitDone) LoadINI();
    } else if(GameState!=GAMESTATE_LOGGINGIN) {
        if (bInitDone) bInitDone=false;
    }
}


void SpendCommand(PSPAWNINFO pChar, PCHAR szLine) {

    CHAR szArg1[MAX_STRING] = {0};
    CHAR szArg2[MAX_STRING] = {0};
    CHAR szArg3[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);
    GetArg(szArg3, szLine, 3);

    if(!_strcmpi(szArg1, "")) {
        ShowHelp();
        return;
    }
    if(!_strcmpi(szArg1,"debug")) {
        if(!_strcmpi(szArg2, "on")) {
            bDebug = true;
        }
        else if(!_strcmpi(szArg2, "off")) {
            bDebug = false;
        }        
        WriteChatf("MQ2AASpend :: Debug is %s", bDebug?"\agON\ax":"\arOFF\ax");
    }

    if(!_strcmpi(szArg1, "status")) {
        ShowStatus();
        return;
    }

    if(!_strcmpi(szArg1, "load")) {
        LoadINI();
        return;
    }


    if(!_strcmpi(szArg1, "save")) {
        SaveINI();
        return;
    }

    if(!_strcmpi(szArg1,"brute")) {
        if(!_strcmpi(szArg2, "")) {
            WriteChatf("Usage: /aaspend brute on|off|now");
            WriteChatf("       This will auto buy AA's based on ini values on next ding, or now if now specified.");
            return;
        }
        if(!_strcmpi(szArg2, "on")) {
            WriteChatf("MQ2AASpend :: Brute Force Mode Enabled -- Buying AA's in order of appearence, from class to general, then special");
            bBruteForce = true;
			bBruteForceBonusFirst = false;
            bAutoSpend = false;
        }
        else if(!_strcmpi(szArg2, "off")) {
            WriteChatf("MQ2AASpend :: Brute Force Mode Disabled");
            bBruteForce = false;
        }        
        else if(!_strcmpi(szArg2, "now")) {
            WriteChatf("MQ2AASpend :: Brute Force Mode Active Now");
            doBruteForce = 2;
        }        
        return;
    }
	if(!_strcmpi(szArg1,"bonus")) {
        if(!_strcmpi(szArg2, "")) {
            WriteChatf("Usage: /aaspend bonus on|off|now");
            WriteChatf("       This will auto buy AA's based on bonusAA first, in other words, from expansions post autogrant only.");
            return;
        }
        if(!_strcmpi(szArg2, "on")) {
            WriteChatf("MQ2AASpend :: Brute Force Bonus Mode Enabled -- Buying AA's from expansions post autogrant only.");
			bBruteForce = false;
            bBruteForceBonusFirst = true;
            bAutoSpend = false;
        } else if(!_strcmpi(szArg2, "off")) {
            WriteChatf("MQ2AASpend :: Brute Force Bonus Mode Disabled");
            bBruteForceBonusFirst = false;
        } else if(!_strcmpi(szArg2, "now")) {
            WriteChatf("MQ2AASpend :: Brute Force Bonus Mode Active Now");
            doBruteForceBonusFirst = 2;
        }            
        return;
    }

    if(!_strcmpi(szArg1,"auto")) {
        if(!_strcmpi(szArg2, "")) {
            WriteChatf("Usage: /aaspend auto on|off|now");
            WriteChatf("       This will auto buy AA's based on ini values on next ding, or now if now specified.");
            return;
        }
        if(!_strcmpi(szArg2, "on")) {
            WriteChatf("MQ2AASpend :: Auto Mode Enabled -- Buying AA's based on ini values");
            bAutoSpend = true;
            bBruteForce = false;
			bBruteForceBonusFirst = false;
			bAutoSpendNow = false;
        }
        else if(!_strcmpi(szArg2, "off")) {
            WriteChatf("MQ2AASpend :: Auto Mode Disabled");
            bAutoSpend = false; 
			bAutoSpendNow = false;
        }
        else if(!_strcmpi(szArg2, "now")) {
            WriteChatf("MQ2AASpend :: Auto Mode Active Now");
			if (vAAList.size()) {
				doAutoSpend = true;
				bAutoSpendNow = true;
			}
        }
        return;
    }

    if(!_strcmpi(szArg1,"bank")) {
        if(!_strcmpi(szArg2, "")) {
            WriteChatf("Usage: /aaspend bank #");
            WriteChatf("       This will keep a minimum of # aa points before auto spending or brute force spending.");
            return;
        } else {
            sprintf_s(iBankPoints,"%s",szArg2);
            WriteChatf("MQ2AASpend :: Banking %s points",szArg2);
            SaveINI();
            return;
        }
    }

	if(!_strcmpi(szArg1,"order")) {
        if(!_strcmpi(szArg2, "")) {
			WriteChatf("Usage: /aaspend order \ag#####\ax where ##### is 5 numbers representing 5 tabs in the aa window.");
            WriteChatf("       General is 1 ArchType is 2 Class is 3 Special is 4 and Focus is 5.");
            WriteChatf("       Example: Lets say you want to spend in this order: Class, Focus, Archtype, General and last Special, you would then type: \ay/aaspend order 35214\ax");
            return;
        } else {
            sprintf_s(SpendOrder,"%s",szArg2);
            WriteChatf("MQ2AASpend :: New Spend Order is %s",szArg2);
            SaveINI();
            return;
        }
    }

    if(!_strcmpi(szArg1,"add")) {
        if(!_strcmpi(szArg2, "") || !_strcmpi(szArg3, "")) {
            WriteChatf("Usage: /aaspend add \"AA Name\" maxlevel");
            WriteChatf("        maxlevel can be a number or m or M for max available.");
            return;
        } else {
            for (unsigned int a = 0; a < vAAList.size(); a++) {
                string& vRef = vAAList[a];
                if (!_stricmp(szArg2,vRef.c_str())) {
                    WriteChatf("MQ2AASpend :: Ability %s already exists", szTemp);
                    return;
                }
            }

            vAAList.push_back(szArg2);
            vAALevel.push_back(szArg3);
            WriteChatf("MQ2AASpend :: Added %s levels of %s to ini", szArg3, szArg2);
            SaveINI();
        }       
    }

    if(!_strcmpi(szArg1,"del")) {
        int delIndex = -1;
        if(!_strcmpi(szArg2, "")) {
            WriteChatf("Usage: /aaspend del \"AA Name\"");
            return;
        } else {
            for (unsigned int a = 0; a < vAAList.size(); a++) {
                string& vRef = vAAList[a];
                if (!_stricmp(szArg2,vRef.c_str())) {
                    delIndex = a;
                }
            }
            if (delIndex >= 0) {         
                vAAList.erase(vAAList.begin() + delIndex);
                vAALevel.erase(vAALevel.begin() + delIndex);
                WriteChatf("MQ2AASpend :: Deleted ability %s from ini", szTemp);
                SaveINI();
            } else {
                WriteChatf("MQ2AASpend :: Abilty %s not found in ini", szTemp);
            }
        }       
    }


    if(!_strcmpi(szArg1,"buy")) {
        if(!_strcmpi(szArg2, "")) {
            WriteChatf("Usage: /aaspend buy \"AA Name\"");
            return;
        }
		int level = -1;
		if (PSPAWNINFO pMe = (PSPAWNINFO)pLocalPlayer) {
			level = pMe->Level;
		}
		int index = GetAAIndexByName((PCHAR)szArg2);
		if (PALTABILITY pAbility = GetAAByIdWrapper(index, level)) {
			BuySingleAA(pAbility);
			return;
		}
    }
}

// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID) {

    if (GetGameState() != GAMESTATE_INGAME)
		return;

    if (SkipPulse == SKIP_PULSES) {
        SkipPulse = 0;
		if (doBruteForce!=0) {
			BruteForcePurchase(doBruteForce);
            doBruteForce = 0;
        }
		if(doBruteForceBonusFirst!=0) {
			BruteForceBonusFirstPurchase(doBruteForceBonusFirst);
            doBruteForceBonusFirst = 0;
        }
        if(doAutoSpend && vAAList.size()) {
            SpendFromINI();
            doAutoSpend = false;
			bAutoSpendNow = false;
        }
    }
    SkipPulse++;
}

PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color) {
    if ( strstr(Line,"You have gained") && strstr(Line,"ability point") && strstr(Line,"You now have") ) {
        if(bBruteForce)
			doBruteForce = 1;
		if (bBruteForceBonusFirst)
			doBruteForceBonusFirst = 1;
        if(bAutoSpend && vAAList.size())
			doAutoSpend = true;
    }
    if ( strstr(Line,"Welcome to level") ) {
        if(bBruteForce)
			doBruteForce = 1;
		if (bBruteForceBonusFirst)
			doBruteForceBonusFirst = 1;
        if(bAutoSpend && vAAList.size())
			doAutoSpend = true;
    }
    return 0;
}

PLUGIN_API void InitializePlugin( void ) {
    DebugSpewAlways("Initializing MQ2AASpend");
    AddCommand("/aaspend",SpendCommand);
    ShowHelp();
}

PLUGIN_API void ShutdownPlugin( void ) {
    DebugSpewAlways("Shutting down MQ2AASpend");
    RemoveCommand("/aaspend");
}