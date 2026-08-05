#pragma once

#include <stdint.h>

#include "gui.h"
#include "wb_dict32.h"

typedef struct PlayerTeam
{
	const char* name;
	UiTray* tray;
	int points;
	int icon;
} PlayerTeam;

int teamPointTotal = 77;

void saveTrays(FILE* fp, UiTray** trays, int numTrays, const char* key);
void saveTeamTrays(FILE* fp, PlayerTeam* teams, int numTeams, const char* key);
void saveTeams(FILE* fp, PlayerTeam* teams, int numTeams);
int loadTeams(char* text, size_t len, PlayerTeam* teams, int maxTeams);
int loadTrays(char* text, size_t len, dict32* trayData, int** arrays, int** etrAarrays, int** etrBarrays, int* sizes);
