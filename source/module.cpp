/*
* Gspeak 3.0
* by Thendon.exe
* Sneaky Rocks
*/

#define TSLIB_VERSION 3000

#include "GarrysMod/Lua/Interface.h"
#include <list>
#include <Windows.h>
#include "shared/shared.h"
#include <ctime>
#include <iostream>
#include <string>

using namespace GarrysMod::Lua;
using namespace Gspeak;
using namespace std::literals::string_literals;

namespace Gspeak
{
	//move this to lua?
	/*struct Client_local
	{
		int clientID;
		int entID;
		int radioID;
	};*/

	struct CommandCallback
	{
		Command key;
		std::string args;
		int callback;

		CommandCallback(Command key, int callback) : key(key), callback(callback), args("") {}
	};
	
	enum class LogType
	{
		Log,
		Warning,
		Error,
		Success
	};
}

//Client_local* clients_local;
std::list<CommandCallback> commandList;
std::list<short> avaliableSpots;

//*************************************
// GSPEAK FUNCTIONS
//*************************************

void gs_printError(GarrysMod::Lua::ILuaBase* LUA, char* error_string, int error_code = -1)
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "print");
	LUA->PushString(error_string);
	
	if (error_code == -1)
	{
		LUA->Call(1, 0);
	}
	else
	{
		LUA->PushNumber(error_code);
		LUA->Call(2, 0);
	}
	LUA->Pop();
}

void gs_log(GarrysMod::Lua::ILuaBase* LUA, std::string msg, LogType type = LogType::Log)
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "gspeak");
	switch (type)
	{
	case LogType::Warning:
		LUA->GetField(-1, "ConsoleWarning");
		break;
	case LogType::Error:
		LUA->GetField(-1, "ConsoleError");
		break;
	case LogType::Success:
		LUA->GetField(-1, "ConsoleSuccess");
		break;
	case LogType::Log:
	default:
		LUA->GetField(-1, "ConsoleLog");
		break;
	}

	msg = "[tslib] " + msg;

	LUA->PushString(msg.c_str());
	LUA->Call(1, 0);
	LUA->Pop();
}

//TODO: concurrency should be an issue with this whole idea actually
// here might be a solution:
// https://learn.microsoft.com/en-us/windows/win32/sync/using-named-objects
bool gs_openMapFile(GarrysMod::Lua::ILuaBase* LUA, HANDLE* hMapFile, TCHAR* name, unsigned int buf_size)
{
	*hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (*hMapFile == NULL)
	{
		int code = GetLastError();
		gs_printError(LUA, "[Gspeak] open map state ", code);
		if (code == 5)
		{
			gs_printError(LUA, "[Gspeak] access denied - restart Teamspeak3 with Administrator!");
			return false;
		}
		else if (code == 2)
		{
			*hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, buf_size, name);
			if (*hMapFile == NULL)
			{
				gs_printError(LUA, "[Gspeak] critical error ", GetLastError());
				return false;
			}

			//init map file here
			//init concurrency stuff here
		}
		else
		{
			gs_printError(LUA, "[Gspeak] critical error ", GetLastError());
			return false;
		}
	}
	return true;
}

LUA_FUNCTION(gs_connectTS)
{
	if (Shared::openClients() != HMAP_RESULT::SUCCESS)
	{
		gs_printError(LUA, "[Gspeak] mapview error ", GetLastError());
		LUA->PushBool(false);
		return 1;
	}

	if (Shared::openStatus() != HMAP_RESULT::SUCCESS)
	{
		gs_printError(LUA, "[Gspeak] mapview error ", GetLastError());
		LUA->PushBool(false);
		return 1;
	}

	//clients_local = new Client_local[PLAYER_MAX];
	for (short i = 0; i < PLAYER_MAX; i++)
		avaliableSpots.push_back(i);

	Shared::status()->tslibV = TSLIB_VERSION;

	LUA->PushBool(true);
	return 1;
}

void gs_discoTS()
{
	Shared::status()->tslibV = 0;

	Shared::closeClients();
	Shared::closeStatus();

	/*if (clients_local != NULL)
	{
		delete[] clients_local;
		clients_local = NULL;
	}*/
}

LUA_FUNCTION(gs_discoTS)
{
	gs_discoTS();
	return 0;
}

int gs_takeAvaliableSpot()
{
	//no avaliable spots => try override most irrelevant spot
	if (avaliableSpots.size() == 0)
	{
		float lowestVolume = FLT_MAX;
		int lowestVolumeIndex = -1;

		for (int i = 0; i < PLAYER_MAX; i++)
		{
			if (!Shared::clients()[i].talking)
				return i;

			if (Shared::clients()[i].volume_gm < lowestVolume)
			{
				lowestVolume = Shared::clients()[i].volume_gm;
				lowestVolumeIndex = i;
			}
		}

		/*if (lowestVolumeIndex == 1)
		{
			srand((unsigned int)time(nullptr));
			return rand() % PLAYER_MAX;
		}*/

		return lowestVolumeIndex;
	}

	short index = avaliableSpots.front();
	avaliableSpots.pop_front();

	return (int)index;
}

void gs_addAvaliableSpot(int index)
{
	avaliableSpots.push_back((short)index);
}

//int gs_searchPlayerObsolete(int clientID, bool* exist)
//{
//	int free_spot = -1;
//	int del_spot = -1;
//	int spot = 0;
//
//	for (spot; Shared::clients()[spot].clientID != 0 && spot < PLAYER_MAX; spot++)
//	{
//		//return already claimed spot
//		if (Shared::clients()[spot].clientID == clientID)
//		{
//			*exist = true;
//			return spot;
//		}
//
//		//Check if spot free
//		if (Shared::clients()[spot].clientID == -1)
//		{
//			//set first free spot
//			if (free_spot == -1)
//				free_spot = spot;
//			//set first deletable spot
//			if (del_spot == -1)
//				del_spot = spot;
//		}
//
//		//Check if spot not free
//		if (Shared::clients()[spot].clientID > -1)
//			//reset deletable spot
//			if (del_spot != -1)
//				del_spot = -1;
//	}
//
//	//clear spots from first deletable spot to next clientID = 0
//	if (del_spot != -1)
//		for (int i = del_spot; del_spot <= spot && i < PLAYER_MAX; i++)
//		{
//			Shared::clients()[i].clientID = 0;
//			clients_local[i].clientID = 0;
//		}
//
//	//return free spot
//	if (free_spot > -1)
//		return free_spot;
//	//return first avaliable spot if nothing was deleted
//	if (del_spot == -1)
//		return spot;
//	//return first deleted spot
//	return del_spot;
//}

LUA_FUNCTION(gs_sendSettings)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); short radio_downsampler = (short)LUA->GetNumber(1);
	LUA->CheckType(2, GarrysMod::Lua::Type::NUMBER); short radio_distortion = (short)LUA->GetNumber(2);
	LUA->CheckType(3, GarrysMod::Lua::Type::NUMBER); float radio_volume = (float)LUA->GetNumber(3);
	LUA->CheckType(4, GarrysMod::Lua::Type::NUMBER); float radio_volume_noise = (float)LUA->GetNumber(4);

	Shared::status()->radioEffect.downsampler = radio_downsampler > 0 ? radio_downsampler : 1;
	Shared::status()->radioEffect.distortion = radio_distortion;
	Shared::status()->radioEffect.volume = radio_volume;
	Shared::status()->radioEffect.noise = radio_volume_noise;

	return 0;
}

bool gs_pushCMD(GarrysMod::Lua::ILuaBase* LUA, Command key, int callback, const std::string* args = NULL)
{
	if (commandList.size() >= CMD_BUF)
	{
		gs_printError(LUA, "CommandList full - check for sendName / forceMove spam");
		return false;
	}

	CommandCallback cmd(key, callback);

	if (args != NULL)
	{
		if (args->length() >= CMD_ARGS_BUF)
			return false;

		cmd.args = *args;
	}

	commandList.push_back(cmd);

	return true;
}

LUA_FUNCTION(gs_sendName)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::STRING); char* name = (char*)LUA->GetString(1);

	int callback = -1;
	if (LUA->GetType(2) == GarrysMod::Lua::Type::FUNCTION)
	{
		LUA->Push(2);
		callback = LUA->ReferenceCreate();
	}

	if (strlen(name) >= NAME_BUF - 1)
		name[NAME_BUF - 1] = '\0';

	//strcpy_s(Shared::status()->name, NAME_BUF * sizeof(char), name);
	std::string args(name);
	if (!gs_pushCMD(LUA, Command::Rename, callback, &args))
	{
		LUA->PushBool(false);
		return 1;
	}
	LUA->PushBool(true);
	return 1;
}

LUA_FUNCTION(gs_compareName)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::STRING); char* name = (char*)LUA->GetString(1);
	if (strlen(name) >= NAME_BUF - 1)
		name[NAME_BUF - 1] = '\0';

	if (strcmp(Shared::status()->name, name) == 0)
		LUA->PushBool(true);
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(gs_forceMove)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::FUNCTION); LUA->Push(1); int callback = LUA->ReferenceCreate();
	LUA->CheckType(2, GarrysMod::Lua::Type::STRING); char* password = (char*)LUA->GetString(2);
	LUA->CheckType(3, GarrysMod::Lua::Type::STRING); char* channelName = (char*)LUA->GetString(3);

	std::string commandArgs(password);
	commandArgs.append(";");
	commandArgs.append(channelName);

	if (!gs_pushCMD(LUA, Command::ForceMove, callback, &commandArgs))
	{
		LUA->PushBool(false);
		return 1;
	}

	LUA->PushBool(true);
	return 1;
}

LUA_FUNCTION(gs_forceKick)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::FUNCTION); LUA->Push(1); int callback = LUA->ReferenceCreate();

	if (!gs_pushCMD(LUA, Command::ForceKick, callback))
	{
		LUA->PushBool(false);
		return 1;
	}

	LUA->PushBool(true);
	return 1;
}

LUA_FUNCTION(gs_update)
{
	if (Shared::status()->command < Command::Clear)
	{
		CommandCallback result = commandList.front();
		//gs_log(LUA, "handle command result "s + std::to_string((int)Shared::status()->command), LogType::Log);

		if (result.callback >= 0)
		{
			LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
			LUA->ReferencePush(result.callback);
			LUA->PushBool((bool)(Shared::status()->command == Command::Success));
			LUA->Call(1, 0);
			LUA->Pop();

			LUA->ReferenceFree(result.callback);
		}

		//Set ready for next cmd
		commandList.pop_front();

		Shared::status()->command = Command::Clear;
		strcpy_s(Shared::status()->commandArgs, CMD_ARGS_BUF * sizeof(char), "");
	}

	//Ready for next in list
	if (Shared::status()->command == Command::Clear && commandList.size() > 0)
	{
		Shared::status()->command = commandList.front().key;
		strcpy_s(Shared::status()->commandArgs, CMD_ARGS_BUF * sizeof(char), commandList.front().args.c_str());
		//gs_log(LUA, "handle command "s + std::to_string((int)Shared::status()->command) + " "s + std::string(Shared::status()->commandArgs), LogType::Log);
	}

	return 0;
}

LUA_FUNCTION(gs_sendClientPos)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); float forX = (float)LUA->GetNumber(1);
	LUA->CheckType(2, GarrysMod::Lua::Type::NUMBER); float forY = (float)LUA->GetNumber(2);
	LUA->CheckType(3, GarrysMod::Lua::Type::NUMBER); float forZ = (float)LUA->GetNumber(3);
	LUA->CheckType(4, GarrysMod::Lua::Type::NUMBER); float upX = (float)LUA->GetNumber(4);
	LUA->CheckType(5, GarrysMod::Lua::Type::NUMBER); float upY = (float)LUA->GetNumber(5);
	LUA->CheckType(6, GarrysMod::Lua::Type::NUMBER); float upZ = (float)LUA->GetNumber(6);

	Shared::status()->forward[0] = forX;
	Shared::status()->forward[1] = forZ;
	Shared::status()->forward[2] = forY;
	Shared::status()->upward[0] = upX;
	Shared::status()->upward[1] = upZ;
	Shared::status()->upward[2] = upY;
	return 0;
}

LUA_FUNCTION(gs_sendPlayer)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); int clientID = (int)LUA->GetNumber(1);
	LUA->CheckType(2, GarrysMod::Lua::Type::NUMBER); float volume = (float)LUA->GetNumber(2);
	LUA->CheckType(3, GarrysMod::Lua::Type::NUMBER); float x = (float)LUA->GetNumber(3);
	LUA->CheckType(4, GarrysMod::Lua::Type::NUMBER); float y = (float)LUA->GetNumber(4);
	LUA->CheckType(5, GarrysMod::Lua::Type::NUMBER); float z = (float)LUA->GetNumber(5);
	LUA->CheckType(6, GarrysMod::Lua::Type::NUMBER); int effect = (float)LUA->GetNumber(6);

	int index = Shared::findClientIndex(clientID);
	if (index == -1)
	{
		index = gs_takeAvaliableSpot();
		if (index == -1)
		{
			LUA->PushBool(false);
			return 1;
		}

		Shared::clients()[index].clientID = clientID;
	}

	Shared::clients()[index].volume_gm = volume;
	Shared::clients()[index].pos[0] = x;
	Shared::clients()[index].pos[1] = z;
	Shared::clients()[index].pos[2] = y;
	Shared::clients()[index].effect = static_cast<VoiceEffect>(effect);

	//todo init new player here (all members)
	Shared::clients()[index].talking = false;

	LUA->PushBool(true);
	return 1;
}

LUA_FUNCTION(gs_removePlayer)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); int clientID = (int)LUA->GetNumber(1);

	int index = Shared::findClientIndex(clientID);
	LUA->PushBool(index != -1);

	if (index != -1)
	{
		Shared::clients()[index].clientID = 0;
		gs_addAvaliableSpot(index);
	}

	return 1;
}

LUA_FUNCTION(gs_getPlayerTeamspeakData)
{
	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); int clientID = (int)LUA->GetNumber(1);

	float volume = 0.0f;
	bool talking = false;

	int index = Shared::findClientIndex(clientID);
	LUA->PushBool(index != -1);

	if (index != -1)
	{
		volume = Shared::clients()[index].volume_ts;
		talking = Shared::clients()[index].talking;
	}

	LUA->PushNumber(volume);
	LUA->PushNumber(talking);

	return 3;
}

//todo: 
// * list of ents instead of players
// * players are attached to ent
// * mix player & ent signal effects
//LUA_FUNCTION(gs_sendPos)
//{
//	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); int clientID = (int)LUA->GetNumber(1);
//	LUA->CheckType(2, GarrysMod::Lua::Type::NUMBER); float volume = (float)LUA->GetNumber(2);
//	LUA->CheckType(3, GarrysMod::Lua::Type::NUMBER); int entID = (int)LUA->GetNumber(3);
//	LUA->CheckType(4, GarrysMod::Lua::Type::NUMBER); float x = (float)LUA->GetNumber(4);
//	LUA->CheckType(5, GarrysMod::Lua::Type::NUMBER); float y = (float)LUA->GetNumber(5);
//	LUA->CheckType(6, GarrysMod::Lua::Type::NUMBER); float z = (float)LUA->GetNumber(6);
//	LUA->CheckType(7, GarrysMod::Lua::Type::BOOL); bool isRadio = (bool)LUA->GetBool(7);
//	//LUA->CheckType(7, GarrysMod::Lua::Type::BOOL); int effect = (int)LUA->GetNumber(7);
//
//	int radioID = -1;
//	if (isRadio == true)
//	{
//		LUA->CheckType(8, GarrysMod::Lua::Type::NUMBER); radioID = (int)LUA->GetNumber(8);
//	}
//
//	int index = Shared::findClientIndex(clientID);
//
//	if (index != -1)
//	{
//		//Update data
//		if (clients_local[index].radioID == radioID && clients_local[index].entID == entID)
//		{
//			Shared::clients()[index].volume_gm = volume;
//			Shared::clients()[index].pos[0] = x;
//			Shared::clients()[index].pos[1] = z;
//			Shared::clients()[index].pos[2] = y;
//			return 0;
//		}
//		//Ignore further away data
//		/*else*/ if (volume < Shared::clients()[index].volume_gm)
//		{
//			return 0;
//		}
//		//Override with closer data
//	}
//	else
//	{
//		//Add new data
//		index = gs_takeAvaliableSpot();
//	}
//
//	Shared::clients()[index].clientID = clientID;
//	Shared::clients()[index].volume_gm = volume;
//	Shared::clients()[index].pos[0] = x;
//	Shared::clients()[index].pos[1] = z;
//	Shared::clients()[index].pos[2] = y;
//	Shared::clients()[index].effect = isRadio ? VoiceEffect::Radio : VoiceEffect::None;
//	Shared::clients()[index].talking = false;
//
//	//gs_log(LUA, "add "s + std::to_string(index) + " "s + std::to_string(Shared::clients()[index].clientID));
//
//	clients_local[index].clientID = clientID;
//	clients_local[index].entID = entID;
//	clients_local[index].radioID = radioID;
//
//	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
//	LUA->GetField(-1, "gspeak");
//	LUA->GetField(-1, "setHearable");
//	LUA->PushNumber(entID);
//	LUA->PushBool(true);
//	LUA->Call(2, 0);
//	LUA->Pop();
//
//	return 0;
//}
//
////TODO: remove entid or radioid everywhere
//LUA_FUNCTION(gs_delPos)
//{
//	LUA->CheckType(1, GarrysMod::Lua::Type::NUMBER); int entID = (int)LUA->GetNumber(1);
//	LUA->CheckType(2, GarrysMod::Lua::Type::BOOL); bool isRadio = (bool)LUA->GetBool(2);
//
//	int radioID = -1;
//	if (isRadio)
//	{
//		LUA->CheckType(3, GarrysMod::Lua::Type::NUMBER); radioID = (int)LUA->GetNumber(3);
//	}
//
//	for (int i = 0; i < PLAYER_MAX; i++)
//	{
//		if (clients_local[i].entID == entID && clients_local[i].radioID == radioID)
//		{
//			int prevId = Shared::clients()[i].clientID;
//			Shared::clients()[i].clientID = 0;
//			//clients_local[i] = Client_local();
//			clients_local[i].clientID = 0;
//			clients_local[i].entID = 0;
//			clients_local[i].radioID = 0;
//			gs_addAvaliableSpot(i);
//			//gs_log(LUA, "del "s + std::to_string(i) + " "s + std::to_string(Shared::clients()[i].clientID) + " "s + std::to_string(prevId));
//			break;
//		}
//	}
//
//	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
//	LUA->GetField(-1, "gspeak");
//	LUA->GetField(-1, "setHearable");
//	LUA->PushNumber(entID);
//	LUA->PushBool(false);
//	LUA->Call(2, 0);
//	LUA->Pop();
//
//	return 0;
//}

LUA_FUNCTION(gs_delAll)
{
	for (int i = 0; i < PLAYER_MAX; i++)
	{
		Shared::clients()[i].clientID = 0;
		//clients_local[i].clientID = 0;
	}
	return 0;
}

LUA_FUNCTION(gs_getTsID)
{
	if (Shared::status()->gspeakV == 0)
		LUA->PushNumber(0);
	else
		LUA->PushNumber(Shared::status()->clientID);
	return 1;
}

LUA_FUNCTION(gs_getInChannel)
{
	if (!Shared::status()->inChannel || Shared::status()->gspeakV == 0)
		LUA->PushBool(false);
	else
		LUA->PushBool(true);
	return 1;
}

LUA_FUNCTION(gs_getArray)
{
	char it[3];
	LUA->PushSpecial(GarrysMod::Lua::Type::TABLE);
	LUA->CreateTable();
	for (int i = 0; i < PLAYER_MAX; i++)
	{
		if (Shared::clients()[i].clientID == 0)
			continue;

		LUA->PushNumber(Shared::clients()[i].clientID);
		sprintf_s(it, sizeof(it), "%d", i);
		LUA->SetField(-2, it);
	}
	return 1;
}

LUA_FUNCTION(gs_talkCheck)
{
	if (Shared::status()->talking == true)
	{
		LUA->PushBool(true);
	}
	else
	{
		LUA->PushBool(false);
	}
	return 1;
}

LUA_FUNCTION(gs_getGspeakVersion)
{
	LUA->PushNumber(Shared::status()->gspeakV);
	return 1;
}

LUA_FUNCTION(gs_getName)
{
	LUA->PushString(Shared::status()->name);
	return 1;
}

LUA_FUNCTION(gs_getTslibVersion)
{
	LUA->PushNumber(TSLIB_VERSION);
	return 1;
}

//LUA_FUNCTION(gs_tick)
//{
//	for (int i = 0; i < PLAYER_MAX; i++)
//	{
//		if (Shared::clients()[i].clientID == 0)
//			continue;
//
//		LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
//		LUA->GetField(-1, "gspeak");
//		LUA->GetField(-1, "SetPlayerTeamspeakData");
//		//LUA->PushNumber(clients_local[i].entID);
//		LUA->PushNumber(Shared::clients()[i].volume_ts);
//		LUA->PushBool(Shared::clients()[i].talking);
//		LUA->Call(3, 0);
//		LUA->Pop();
//	}
//	return 0;
//}

//obsolete
//LUA_FUNCTION(gs_getAllID)
//{
//	//char it[3];
//	LUA->PushSpecial(GarrysMod::Lua::Type::TABLE);
//	LUA->CreateTable();
//	for (int i = 0; i < PLAYER_MAX; i++)
//	{
//		if (Shared::clients()[i].clientID == 0)
//			continue;
//		
//		LUA->CreateTable();
//		LUA->PushNumber(clients_local[i].entID); LUA->SetField(-2, "ent_id");
//		LUA->PushBool(Shared::clients()[i].effect == VoiceEffect::Radio); LUA->SetField(-2, "radio");
//		LUA->PushNumber(clients_local[i].radioID); LUA->SetField(-2, "radio_id");
//		LUA->PushNumber(Shared::clients()[i].volume_ts); LUA->SetField(-2, "volume");
//		LUA->PushBool(Shared::clients()[i].talking); LUA->SetField(-2, "talking");
//		std::string id = std::to_string(i);
//		LUA->SetField(-2, id.c_str());
//	}
//
//	return 1;
//}

//*************************************
// REQUIRED GMOD FUNCTIONS
//*************************************

GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CreateTable();
	LUA->PushCFunction(gs_connectTS); LUA->SetField(-2, "connectTS");
	LUA->PushCFunction(gs_discoTS); LUA->SetField(-2, "discoTS");
	LUA->PushCFunction(gs_sendSettings); LUA->SetField(-2, "sendSettings");
	LUA->PushCFunction(gs_sendName); LUA->SetField(-2, "sendName");
	LUA->PushCFunction(gs_getName); LUA->SetField(-2, "getName");
	LUA->PushCFunction(gs_compareName); LUA->SetField(-2, "compareName");
	LUA->PushCFunction(gs_forceMove); LUA->SetField(-2, "forceMove");
	LUA->PushCFunction(gs_forceKick); LUA->SetField(-2, "forceKick");
	LUA->PushCFunction(gs_update); LUA->SetField(-2, "update");
	LUA->PushCFunction(gs_sendClientPos); LUA->SetField(-2, "sendClientPos");
	/*LUA->PushCFunction(gs_sendPos); LUA->SetField(-2, "sendPos");
	LUA->PushCFunction(gs_delPos); LUA->SetField(-2, "delPos");*/
	LUA->PushCFunction(gs_sendPlayer); LUA->SetField(-2, "sendPlayer");
	LUA->PushCFunction(gs_removePlayer); LUA->SetField(-2, "removePlayer");
	LUA->PushCFunction(gs_delAll); LUA->SetField(-2, "delAll");
	LUA->PushCFunction(gs_getTsID); LUA->SetField(-2, "getTsID");
	LUA->PushCFunction(gs_getInChannel); LUA->SetField(-2, "getInChannel");
	LUA->PushCFunction(gs_getArray); LUA->SetField(-2, "getArray");
	LUA->PushCFunction(gs_talkCheck); LUA->SetField(-2, "talkCheck");
	LUA->PushCFunction(gs_getGspeakVersion); LUA->SetField(-2, "getGspeakVersion");
	LUA->PushCFunction(gs_getTslibVersion); LUA->SetField(-2, "getVersion");
	//LUA->PushCFunction(gs_getVolumeOf); LUA->SetField(-2, "getVolumeOf");
	//LUA->PushCFunction(gs_getAllID); LUA->SetField(-2, "getAllID");
	LUA->PushCFunction(gs_getPlayerTeamspeakData); LUA->SetField(-2, "getPlayerData");
	//LUA->PushCFunction(gs_tick); LUA->SetField(-2, "tick");
	LUA->SetField(-2, "tslib");
	LUA->Pop();

	gs_printError(LUA, "Module successfully loaded!");

	return 0;
}

GMOD_MODULE_CLOSE()
{
	gs_discoTS();
	return 0;
}