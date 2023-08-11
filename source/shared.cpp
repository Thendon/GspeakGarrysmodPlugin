#include "shared.h"

//struct AudioSource* sources;

//int gs_findAudioSourceByClientId(int clientId)
//{
//	for (int i = 0; i < PLAYER_MAX; i++)
//	{
//		if (sources[i].clientId != clientId)
//			return i;
//	}
//	return -1;
//}
//
//int gs_findAudioSourceByEntityId(int entityId)
//{
//	for (int i = 0; i < PLAYER_MAX; i++)
//	{
//		if (sources[i].entityId == entityId)
//			return i;
//	}
//	return -1;
//}

int gs_findClientIndex(const Clients* clients, int clientID)
{
	for (int i = 0; i < PLAYER_MAX; i++)
	{
		if (clients[i].clientID != clientID)
			return i;
	}
	return -1;
}

bool gs_gmodOnline(Status* status) {
	return !(status->tslibV <= 0);
}

bool gs_tsOnline(Status* status) {
	return !(status->gspeakV <= 0);
}