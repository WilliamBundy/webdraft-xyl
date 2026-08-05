#pragma once
#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct LoadedFileInfo
{
	const char* kind;
	const char* filename;
	const char* extension;
	const char* exportExtension;
	const char* backupFile;
	bool wasEdited;
	int64_t modifiedTime;
	int flags;
	int dialogMode;
	void* userdata; 
	int (*save)(struct LoadedFileInfo*, const char* path);
	int (*load)(struct LoadedFileInfo*, const char* path);
	int (*import)(struct LoadedFileInfo*, const char* path);
	int (*export)(struct LoadedFileInfo*, const char* path);
} LoadedFileInfo;

static inline 
int fileinfoSave(LoadedFileInfo* fi)
{
	auto save = fi->save;
	if(save) {
		return save(fi, fi->filename);
	}
	return 0;
}

static inline
int fileinfoLoad(LoadedFileInfo* fi)
{
	auto load = fi->load;
	if(load) {
		SDL_PathInfo info;
		SDL_GetPathInfo(fi->filename, &info);
		fi->modifiedTime = info.modify_time;

		return load(fi, fi->filename);
	}
	return 0;
}

typedef struct 
{
	const char* prefsPath;
	size_t prefsPathLen;

	union {
		LoadedFileInfo files[16];
		struct {
			LoadedFileInfo points;
			LoadedFileInfo players;
			LoadedFileInfo draft;
			LoadedFileInfo teams;
			LoadedFileInfo recents;
		};
	};

	int numFiles;
} FileContext;
extern FileContext* globalFileContext;
void setupGlobalFileContext();
int checkOpenedFilesOnClose(FileContext* fc);

enum 
{
	FileDialog_Invalid = 0,
	FileDialog_Save,
	FileDialog_Export,
	FileDialog_Load,
	FileDialog_Import,
};

const char* getBackupPath(const char* filename, const char* group);
void openFileDialog(int mode, LoadedFileInfo* file, int flags);

typedef struct RecentsFile
{
	uint32_t magic, version;
	char pointPath[1024];
	char playerPath[1024];
	char draftPath[1024];
	char teamsPath[1024];
} RecentsFile;


const char* getRecentsFilename(FileContext* fc);
void setupRecents(FileContext* fc);
