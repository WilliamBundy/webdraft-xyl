#include "serialization.h"
void saveTrays(FILE* fp, UiTray** trays, int numTrays, const char* key)
{
	for(int i = 0; i < numTrays; ++i) {
		UiTray* tray = trays[i];
		for(int j = 0; j < tray->numSlots; ++j) {
			UiTrayEntry* entry = tray->slots[j];
			char buf[512];
			size_t len = SDL_snprintf(buf, 512, "'%s' %d %d %d '%s'\n", tray->title, entry->id+1, entry->a, entry->b, mons[entry->id].name);
			fwrite(buf, 1, len, fp);
		}
	}
}

void saveTeamTrays(FILE* fp, PlayerTeam* teams, int numTeams, const char* key)
{
	for(int i = 0; i < numTeams; ++i) {
		UiTray* tray = teams[i].tray;
		if(!tray) continue;
		for(int j = 0; j < tray->numSlots; ++j) {
			UiTrayEntry* entry = tray->slots[j];
			char buf[512];
			size_t len = SDL_snprintf(buf, 512, "'%s' %d %d %d '%s'\n", tray->title, entry->id+1, entry->a, entry->b, mons[entry->id].name);
			fwrite(buf, 1, len, fp);
		}
	}
}

void saveTeams(FILE* fp, PlayerTeam* teams, int numTeams)
{
	for(int i = 0; i < numTeams; ++i) {
		PlayerTeam* team = &teams[i];
		char buf[512];
		size_t len = SDL_snprintf(buf, 512, "'%s' %d %d\n", team->name, team->points, team->icon);
		fwrite(buf, 1, len, fp);
	}
}

int loadTeams(char* text, size_t len, PlayerTeam* teams, int maxTeams)
{
	// world's worst deserialization code :) 
	char* head = text;
	char* tstart, *tend;

	char name[64];
	char points[64];
	char icon[64];
	int mode = 0;
	int numLoaded = 0;
	while(head[0]) {
		if(head[0] == ' ') head++;

		if(mode == 0) {
			if(head[0] == '\'') {
				tstart = head;
				mode = 1;
			}
		} else if (mode == 1) {
			if(head[0] == '\'') {
				tend = head;
				memset(name, 0, SDL_arraysize(name));
				strncpy(name, tstart + 1, i32min(tend - (tstart + 1), 63));
				mode = 2;
			}
		} else if(mode == 2)  {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(points, 0, SDL_arraysize(points));
			strncpy(points, tstart, i32min(tend + 1 - (tstart), 63));


			mode = 3;
		} else if(mode == 3) {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(icon, 0, SDL_arraysize(icon));
			strncpy(icon, tstart, i32min(tend + 1 - (tstart), 63));
			if(numLoaded < maxTeams) {
				teams[numLoaded].name = SDL_strdup(name);
				teams[numLoaded].points = atoi(points);
				teams[numLoaded].icon = atoi(icon);
				numLoaded++;
			} else {
				break;
			}

			mode = 4;
		} 

		if(mode == 4) {
			if(head[0] == '\n') {
				mode = 0;
			}
		}

		head++;
	}
	return numLoaded;
}

int loadTrays(char* text, size_t len, dict32* trayData, int** arrays, int** etrAarrays, int** etrBarrays, int* sizes)
{
	char* head = text;
	char* tstart, *tend;

	char key[64];
	char num[64];
	char etr_a[64];
	char etr_b[64];
	int mode = 0;
	int numLoaded = 0;
	bool gotKey = false, gotNum = false, gotA = false, gotB = false;
	while(head[0]) {
		if(head[0] == ' ') head++;

		if(mode == 0) {
			if(head[0] == '\'') {
				tstart = head;
				mode = 1;
			}
		} else if (mode == 1) {
			if(head[0] == '\'') {
				tend = head;
				memset(key, 0, SDL_arraysize(key));
				strncpy(key, tstart + 1, i32min(tend - (tstart + 1), 63));
				gotKey = true;
				mode = 2;
			}
		} else if(mode == 2)  {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(num, 0, SDL_arraysize(num));
			strncpy(num, tstart, i32min(tend + 1 - (tstart), 63));
			gotNum = true;

			mode = 3;
		}  else if (mode == 3) {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(etr_a, 0, SDL_arraysize(etr_a));
			strncpy(etr_a, tstart, i32min(tend + 1 - (tstart), 63));
			gotA = true;
			mode = 4;
		}  else if (mode == 4) {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(etr_b, 0, SDL_arraysize(etr_b));
			strncpy(etr_b, tstart, i32min(tend + 1 - (tstart), 63));
			gotB = true;

		}

		if(head[0] == '\n') {
			mode = 0;

			if(!gotKey || !gotNum)  {
				head++;
				gotKey = false;
				gotNum = false;
				gotA = false;
				gotB = false;
				continue;
			}

			int n = atoi(num);
			if(n <= numMons && n > 0) {
				if(monWasLoaded[n-1]) {
					head++;
					continue;
				} else {
					if(countWasLoaded)
						monWasLoaded[n-1] = 1;
				}
			}

			uint32_t index = 0;
			int ret = dict32Get(trayData, key, &index);

			int a = 0;
			int b = 0;

			if(etrAarrays) {
				if(gotA) {
					a = atoi(etr_a);
				}
			}

			if(etrBarrays) {
				if(gotB) {
					b = atoi(etr_b);
				}
			}

			if(ret == DICT32_OK) {
				if(sizes[index] >= 1024) {
					head++;
					continue;
				}
				arrays[index][sizes[index]] = atoi(num);
				if(etrAarrays) etrAarrays[index][sizes[index]] = a;
				if(etrBarrays) etrBarrays[index][sizes[index]] = b;
				sizes[index]++;
				numLoaded++;
			} else {
				// tray not present in pre-allocated set? insert
				int ret2 = dict32Add(&trayData, key, loadedTrays);
				(void)ret2;
				index = loadedTrays++;
				arrays[index][sizes[index]] = atoi(num);
				if(etrAarrays) etrAarrays[index][sizes[index]] = a;
				if(etrBarrays) etrBarrays[index][sizes[index]] = b;
				sizes[index]++;
				numLoaded++;
			}


			gotKey = false;
			gotNum = false;
			gotA = false;
			gotB = false;
		}

		head++;
	}
	return numLoaded;
}

void saveDraft(const char* draftname, const char* teamsname)
{
	FILE* fp = fopen(draftname, "wb");
	saveTrays(fp, trays, numTrays, "tray");
	saveTrays(fp, sourceTrays, numSourceTrays, "source");
	saveTeamTrays(fp, pteams, numTeams, "team");
	fclose(fp);

	fp = fopen(teamsname, "wb");
	saveTeams(fp, pteams, numTeams);
	fclose(fp);
}



void serialize_on_exit()
{
	int teamTotals[16] = {0};
	StatTotals teamStats[16] = {0};

	for(int i = 0; i < numTeams; ++i) {
		for(int j = 0; j < 10; ++j)	 {
			UiTrayEntry* entry = pteams[i].tray->slots[j];
			if(!entry) continue;

			for(int k = 0; k < 6; ++k) {
				teamTotals[i] += mons[entry->id].stats[k];
				teamStats[i].stats[k] += mons[entry->id].stats[k];
			}
		}
	}

	const char* statnames[] = {"HP  :", "ATK  :", "DEF  :", "SpAtk:", "SpDef:", "Speed:"};

	for(int i = 0; i < numTeams; ++i) {
		printf("%s: %d", pteams[i].name, teamTotals[i]);
		for(int k = 0; k < 6; ++k) {
			printf("\t%s: %d\n", statnames[k], teamStats[i].stats[k]);
		}
	}

	if(editedPoints) {
		SDL_Log("writing new point totals");
		FILE* fp = fopen("points.txt", "wb");
		saveTrays(fp, trays, numTrays, "tray");
		saveTrays(fp, sourceTrays, numSourceTrays, "source");
		fclose(fp);
	} else {
		SDL_Log("point assignments unchanged");
		SDL_Log("writing draft and teams");
		saveDraft("draft.txt", "teams.txt");
	}
	
}

