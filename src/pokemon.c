#include <stdio.h>


#include "wb_sdlgame.h"

#include "pokemon.h"
#include "util.h"

uint8_t moveTableFile[] = {
	#embed "assets/xy.wdmoves"
};
uint8_t monTableFile[] = {
	#embed "assets/xy.wdmons"
};
uint8_t abilityTableFile[] = {
	#embed "assets/xy.wdabi"
};
uint8_t crystalMonsPng[] = {
	#embed "assets/xy-sheet.png"
};

SDL_Texture* monTexture;

void monSortStat(MonDef* mons, int numMons, int stat)
{
	for(int i = 1; i < numMons; ++i) {
		int j = i - 1;
		MonDef mon = mons[i];

		while(j >= 0 && mons[j].stats[stat] < mon.stats[stat]) {
			mons[j + 1] = mons[j];
			j--;
		}
		mons[j + 1] = mon;
	}
}

void pokemon_init(MonData* data)
{
	monTexture = SDL_CreateTextureFromSurface(
		Game->renderer, 
		SDL_LoadPNG_IO(
			SDL_IOFromMem(
				crystalMonsPng, 
				SDL_arraysize(crystalMonsPng)), 
			true));

	TableHeader* monheader = (void*)monTableFile;
	data->numMons = monheader->count;
	data->mons = (void*)(monheader + 1);
	data->sortableMons = calloc(data->numMons, sizeof(MonDef));
	memcpy(data->sortableMons, data->mons, data->numMons * sizeof(MonDef));
	for(int i = 0; i < data->numMons; ++i) {
		data->sortableMons[i].preevo = i;
	}
	// 5 is speed
	monSortStat(data->sortableMons, data->numMons, 5);

	data->pointdb = createPointDB(data, 256);
	{
		if(SDL_GetPathInfo("points.default.ptbl", nullptr)) {
			pointdbLoad(data, data->pointdb, "points.default.ptbl");
		}
	}

	data->playerdb = createPlayerDB(256);
	data->draftboard = calloc(1, sizeof(DraftBoard));
	data->draftboard->startingPoints = 100;


	TableHeader* moveheader = (void*)moveTableFile;
	data->numMoves = moveheader->count;
	data->moves = (void*)(moveheader + 1);

	TableHeader* abiheader = (void*)abilityTableFile;
	data->numAbilities = abiheader->count;
	data->abilities = (void*)(abiheader + 1) ;

	//monWasLoaded = calloc(numMons, 1);

	setupMonHasMoves(data, data->numMons);
	data->evoTree = calloc(data->numMons, sizeof(int));
	for(int i = 0; i < data->numMons; ++i) {
		if(data->mons[i].preevo == -1) continue;
		data->evoTree[data->mons[i].preevo]++;
	}
}

bool checkType(int* types, int t)
{
	for(int i = 0; i < Num_Types; ++i) {
		if(types[i] == -1) break;
		if(types[i] == t) {
			return true;
		}
	}
	return false;
}


int maxStatExp[] = {
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
};

int maxDvs[] = {
	15,15,15,15,15,15
};

#include <math.h>

void calcStats(BattleMon* mon)
{
	for(int i = 0; i < 6; ++i) {

		int base = mon->def->stats[i] + mon->dvs[i];
		base *= 2;
		base += (int)(sqrtf(mon->statxp[i]) / 4.0);
		base *= mon->level;
		base /= 100;
		mon->stats[i] = base;
		mon->stats[i] += 5;
	}
	mon->stats[0] += mon->level + 5;
}

DamageResult calcDamage(BattleMon* mon1, BattleMon* mon2, MoveDef* move, int flags)
{
	if(move->number == MV_EXPLOSION || move->number == MV_SELFDESTRUCT) {
		flags |= Dmg_Explosion;
	}

	int category = TypeCategory[move->type];
	if(category == Category_Status || move->power == 0) {
		return (DamageResult){0, 0};
	}

	if(mon1->stats[0] == 0) {
		calcStats(mon1);
	}

	if(mon2->stats[0] == 0) {
		calcStats(mon2);
	}

	int atk = category == Category_Physical ? mon1->stats[Stat_Atk] : mon1->stats[Stat_SpAtk];
	int def = category == Category_Physical ? mon2->stats[Stat_Def] : mon2->stats[Stat_SpDef];

	int typeNum = 1;
	int typeDenom = 1;

	if(checkType(TypeWeaknesses[mon2->def->types[0]], move->type)) {
		typeNum *= 2;
	}
	if(checkType(TypeResists[mon2->def->types[0]], move->type)) {
		typeDenom *= 2;
	}
	if(checkType(TypeImmunities[mon2->def->types[0]], move->type)) {
		typeNum = 0;
	}

	if(mon2->def->types[0] != mon2->def->types[1]) {
		if(checkType(TypeWeaknesses[mon2->def->types[1]], move->type)) {
			typeNum *= 2;
		}
		if(checkType(TypeResists[mon2->def->types[1]], move->type)) {
			typeDenom *= 2;
		}
		if(checkType(TypeImmunities[mon2->def->types[1]], move->type)) {
			typeNum = 0;
		}
	}

	int base = (2 * mon1->level) / 5 + 2;
	base *= move->power;

	if(flags & Dmg_Explosion) {
		def /= 2;
	}

	if(flags & Dmg_LightScreen) {
		if(TypeCategory[move->type] == Category_Special) {
			def *= 2;
		}
	}
	if(flags & Dmg_Reflect) {
		if(TypeCategory[move->type] == Category_Physical) {
			def *= 2;
		}
	}

	base *= atk;
	base /= def;
	base /= 50;

	if(flags & Dmg_ItemBoost20) {
		base = (int)(base * 1.2);
	} else if(flags & Dmg_ItemBoost10) {
		base *= (int)(base * 1.1);
	}

	if(flags & Dmg_Crit) {
		base *= 2;
	}

	base += 2;

	if(flags & Dmg_Rain) {
		if(move->type == Type_Water) {
			base = (int)(base * 1.5);
		} else if(move->type == Type_Fire) {
			base = (int)(base * 0.5);
		} else if(strncmp(move->name, "Solarbeam", 16) == 0) {
			base = (int)(base * 0.5);
		}
	} else if(flags & Dmg_Sun) {
		if(move->type == Type_Water) {
			base = (int)(base * 0.5);
		} else if(move->type == Type_Fire) {
			base = (int)(base * 1.5);
		}
	}

	// stab
	if(mon1->def->types[0] == move->type) {
		base = (int)(base * 1.5);
	}

	if(mon1->def->types[0] != mon1->def->types[1]) {
		if(mon1->def->types[1] == move->type) {
			base = (int)(base * 1.5);
		}
	}

	base *= typeNum;
	base /= typeDenom;

	int low = base * 217 / 255;
	int high = base;


	DamageResult res;
	res.hprange.x = low;
	res.hprange.y = high;
	res.percrange.x = (float)low / mon2->stats[Stat_HP];
	res.percrange.y = (float)high / mon2->stats[Stat_HP];

	res.hitrange.x = res.percrange.x > 0 ? (int)(1.0 / res.percrange.x) + 1 : 999;
	res.hitrange.y = res.percrange.y > 0 ? (int)(1.0 / res.percrange.y) + 1 : 999;

	for(int i = 217; i <= 255; ++i) {
		res.allRolls[i - 217] = base * i / 255;
	}

	return res;
}


#define TABLE_4CC 0x4c424154u

PlayerDatabase* createPlayerDB(int maxTeams)
{
	size_t sz = sizeof(PlayerDatabase);
	sz += sizeof(PlayerTeam) * maxTeams;
	PlayerDatabase* playerdb = calloc(1, sz);
	playerdb->players = (void*)(playerdb + 1);
	playerdb->maxPlayers = maxTeams;
	return playerdb;
}

MonPointDatabase* createPointDB(MonData* data, int maxTiers)
{
	size_t sz = sizeof(MonPointDatabase);
	sz += sizeof(int) * maxTiers; // numMonsPerTier
	sz += sizeof(int) * data->numMons; // pointCosts;	

	//SDL_Log("creating point db with size: %zd", sz);
	//SDL_Log("num mons: %d", data->numMons);
	//SDL_Log("max tiers: %d", maxTiers);
	// TODO investigate heap corruption when calling this multiple times in
	// emscripten!
	MonPointDatabase* db = calloc(1, sz);
	db->maxTiers = maxTiers;
	db->pointCosts = (int*)(db + 1);
	db->numMonsPerTier = (int*)(db->pointCosts + data->numMons);
	db->totalSize = sz;

	// initialize to "invalid point costs"
	for(int i = 0; i < data->numMons; ++i) {
		db->pointCosts[i] = -1;
	}
	return db;
}

int pointdbSaveFile(MonData* data, MonPointDatabase* pointdb, FILE* fp, const char* filename)
{
	TableHeader header = {TABLE_4CC, 0x00010000, Table_Points, data->numMons};
	fwrite(&header, 1, sizeof(TableHeader), fp);
	fwrite(&pointdb->numTiers, sizeof(int), 1, fp);
	fwrite(pointdb->pointCosts, data->numMons, sizeof(int), fp);

	return 0;
}

int pointdbSave(MonData* data, MonPointDatabase* pointdb, const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if(!fp) {
		SDL_Log("couldn't open %s for writing (points)", filename);
		return -1;
	}
	pointdbSaveFile(data, pointdb, fp, filename);
	fclose(fp);
	return 0;
}

int pointdbLoadFile(MonData* data, MonPointDatabase* pointdb, FILE* fp, const char* filename)
{

	TableHeader header;
	fread(&header, 1, sizeof(TableHeader), fp);
	if(header.magic != TABLE_4CC) {
		SDL_Log("invalid table file: %s", filename);
		return -2;
	}
	if(header.kind != Table_Points) {
		SDL_Log("wrong table type, expected points: %s", filename);
		SDL_Log("got table type: %d", header.kind);
		return -3;
	}
	if(header.version > 0x00010000) {
		SDL_Log("unknown version: %s", filename);
		return -4;
	}
	if(header.count != data->numMons) {
		SDL_Log("invalid number of pokemon in points file: %s", filename);
		SDL_Log("should be %d, got %d", data->numMons, header.count);
		return -5;
	}
	fread(&pointdb->numTiers, sizeof(int), 1, fp);
	fread(pointdb->pointCosts, sizeof(int), data->numMons, fp);

	for(int i = 0; i < data->numMons; ++i) {
		int pt = pointdb->pointCosts[i];
		if(pt < -1 || pt > pointdb->maxTiers) {
			SDL_Log("invalid point total (%d) at %d", pt, i);
			return -6;
		}

		if(pt >= 0) {
			pointdb->numMonsPerTier[pt]++;
		}
	}
	return 0;
}
int pointdbLoad(MonData* data, MonPointDatabase* pointdb, const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if(!fp) {
		SDL_Log("couldn't open %s for reading (points)", filename);
		return -1;
	}
	int ret = pointdbLoadFile(data, pointdb, fp, filename);
	fclose(fp);
	return ret;
}

int pointdbImportCSVFromText(MonData* data, MonPointDatabase* pointdb, char* text, size_t size)
{
	if(!data) return -1;
	if(size == 0 || size > 1024 * 1024) return -2;

	char* textEnd = text + size;
	char* t0 = text, *t1;
	int columns[64];
	int numCols = 0;
	int line = 0;
	int col = 0;
	int maxCol = 0;
	while(text != textEnd) {
		bool emit = false;
		if(text[0] == '\n') {
			emit = true;
			t1 = text;
		}
		if(text[0] == ',') {
			emit = true;
			t1 = text;
		}

		if(emit) {
			if(t1[-1] == '\r') t1--;

			if(line == 0) {
				if(numCols < 63) {
					if(strIsNumber(t0, t1)) {
						columns[numCols++] = strToInt(t0, t1);
						maxCol = i32max(maxCol, columns[numCols-1]);
					} else {
						columns[numCols++] = -1;
					}
				}
			} else {
				if(col < numCols) {
					char cached = t1[0];
					t1[0] = '\0';
					int len = (int)(t1 - t0);
					if(len > 0) {
						for(int i = 0; i < data->numMons; ++i) {
							int sd = stringDistanceMatchLen(data->mons[i].name, t0);

							if(strncmp(t0, "Mega ", 5) == 0) {
								t0 += 3;
								t0[0] = 'M';
								t0[1] = '-';
							}

							if(sd == 0) {
								pointdb->pointCosts[i] = columns[col];
								//SDL_Log("%.*s (%d)", (int)(t1-t0), t0, col);
								break;
							} 
						}
					}
					t1[0] = cached;
				}
			}

			t0 = text + 1;
			col++;
		}

		if(text[0] == '\n') {
			line++;
			col = 0;
			//SDL_Log("=====");
		}
		text++;
	}
	
	pointdb->numTiers = i32min(maxCol, data->pointdb->maxTiers);


	return 0;
}

int pointdbImportCSV(MonData* data, MonPointDatabase* pointdb, const char* filename)
{
	size_t size;
	char* text = SDL_LoadFile(filename, &size);
	return pointdbImportCSVFromText(data, pointdb, text, size);
}

int pointdbExportCSV(MonData* data, MonPointDatabase* pointdb, SDL_IOStream* stream)
{
	char*** tiers = calloc(pointdb->numTiers+4, sizeof(char***));
	int* tierCounts = calloc(pointdb->numTiers+4, sizeof(int));
	int invalidTierIndex = pointdb->numTiers + 1;
	for(int i = 0; i <= pointdb->numTiers + 1; ++i) {
		tiers[i] = calloc(data->numMons, sizeof(char**));
	}

	int numRows = 0;
	for(int i = 0; i < data->numMons; ++i) {
		int pt = pointdb->pointCosts[i];
		int index = (pt < 0 || pt > pointdb->numTiers) ? invalidTierIndex : pt;
		tiers[index][tierCounts[index]++] = data->mons[i].name;
		numRows = i32max(numRows, tierCounts[index]);
	}
	//SDL_IOStream* stream = SDL_IOFromFile(filename, "wb");
	if(stream) {
		for(int i = invalidTierIndex; i >= 0; i--) {
			if(i == invalidTierIndex) {
				SDL_IOprintf(stream, "Banned,");
			} else {
				SDL_IOprintf(stream, "%d,", i);
			}
		}
		SDL_SeekIO(stream, -1, SDL_IO_SEEK_CUR);
		SDL_IOprintf(stream, "\n");
		for(int row = 0; row < numRows; ++row) {
			for(int i = invalidTierIndex; i >= 0; i--) {
				int index = i;
				char** tier = tiers[index];
				int count = tierCounts[index];
				if(row < count) {
					SDL_IOprintf(stream, "%s,", tier[row]);
				} else {
					SDL_IOprintf(stream, ",");
				}
			}

			SDL_SeekIO(stream, -1, SDL_IO_SEEK_CUR);
			SDL_IOprintf(stream, "\n");
		}
	}



	free(tierCounts);
	for(int i = 0; i < pointdb->numTiers; ++i) {
		free(tiers[i]);
	}
	free(tiers);
	return 0;
}
int playerdbSaveFile(PlayerDatabase* playerdb, FILE* fp)
{
	TableHeader header = {TABLE_4CC, 0x00010000, Table_Players, playerdb->numPlayers};

	fwrite(&header, 1, sizeof(TableHeader), fp);
	fwrite(playerdb->players, playerdb->numPlayers, sizeof(PlayerTeam), fp);
	return 0;

}

int playerdbSave(PlayerDatabase* playerdb, const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if(!fp) {
		SDL_Log("couldn't open %s for writing (players)", filename);
		return -1;
	}
	playerdbSaveFile(playerdb, fp);
	fclose(fp);
	return 0;

}

int playerdbLoadFile(PlayerDatabase* playerdb, FILE* fp, const char* filename)
{
	TableHeader header;
	fread(&header, 1, sizeof(TableHeader), fp);
	if(header.magic != TABLE_4CC) {
		SDL_Log("invalid table file: %s", filename);
		return -2;
	}
	if(header.kind != Table_Players) {
		SDL_Log("wrong table type, expected players: %s", filename);
		SDL_Log("got table type: %d", header.kind);
		return -3;
	}
	if(header.version > 0x00010000) {
		SDL_Log("unknown version: %s", filename);
		return -4;
	}

	if(header.count > playerdb->maxPlayers || header.count < 0) {
		SDL_Log("invalid number of players in team file");
		return -5;
	}

	playerdb->numPlayers = header.count;
	fread(playerdb->players, 1, playerdb->numPlayers * sizeof(PlayerTeam), fp);
	return 0;
}

int playerdbLoad(PlayerDatabase* playerdb, const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if(!fp) {
		SDL_Log("couldn't open %s for reading (players)", filename);
		return -1;
	}

	int ret = playerdbLoadFile(playerdb, fp, filename);

	fclose(fp);

	return ret;
}

int draftSave(MonData* data, DraftBoard* board, PlayerDatabase* playerdb, MonPointDatabase* pointdb, const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if(!fp) {
		SDL_Log("couldn't open %s for writing (draft)", filename);
		return -1;
	}

	TableHeader header = {TABLE_4CC, 0x00010000, Table_Draft, 0};
	fwrite(&header, 1, sizeof(TableHeader), fp);
	fwrite(board, 1, sizeof(DraftBoard), fp);
	playerdbSaveFile(playerdb, fp);
	pointdbSaveFile(data, pointdb, fp, filename);
	fclose(fp);
	return 0;
}

int draftLoad(MonData* data, DraftBoard* board, PlayerDatabase* playerdb, MonPointDatabase* pointdb, const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if(!fp) {
		SDL_Log("couldn't open %s for reading (draft)", filename);
		return -1;
	}

	TableHeader header;
	fread(&header, 1, sizeof(TableHeader), fp);
	if(header.magic != TABLE_4CC) {
		SDL_Log("invalid table file: %s", filename);
		return -2;
	}
	if(header.kind != Table_Draft) {
		SDL_Log("wrong table type, expected draft: %s", filename);
		SDL_Log("got table type: %d", header.kind);
		return -3;
	}
	if(header.version > 0x00010000) {
		SDL_Log("unknown version: %s", filename);
		return -4;
	}
	fread(board, 1, sizeof(DraftBoard), fp);

	int playerRet = playerdbLoadFile(playerdb, fp, filename);
	int pointRet = 0; //pointdbLoadFile(data, pointdb, fp, filename);
	fclose(fp);
	return playerRet != 0 ? playerRet : pointRet;
}
