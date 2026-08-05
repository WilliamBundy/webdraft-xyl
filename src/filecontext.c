#include "filecontext.h"
#include <SDL3/SDL.h> 
#include "wb_sdlgame.h"
#include <time.h>
#include "util.h"

FileContext* globalFileContext;
void setupGlobalFileContext()
{
	globalFileContext = SDL_calloc(1, sizeof(FileContext));
	FileContext* fc = globalFileContext;
	fc->prefsPath = SDL_GetPrefPath("unlikelyto.win", "webdraft-gen6");
	fc->prefsPathLen = SDL_strlen(fc->prefsPath);
	fc->numFiles = 4;

	setupRecents(fc);
	getRecentsFilename(fc);
	fc->recents.load(&fc->recents, fc->recents.filename);
}

int checkOpenedFilesOnClose(FileContext* fc)
{
	#ifndef __EMSCRIPTEN__
	SDL_Log("checking open files...");
	for(int i = 0; i < fc->numFiles; ++i) {
		LoadedFileInfo* file = &fc->files[i];
		if(!file->wasEdited) {
			SDL_PathInfo info;
			SDL_GetPathInfo(file->filename, &info);

			if(info.modify_time >= file->modifiedTime) {
				// if this check fails, it means the file probably doesn't exist anymore
				continue;
			}
		}

		int buttonid = -1;
		char message[512];
		SDL_snprintf(message, 512, "Do you want to save changes to %s?", file->filename);
		SDL_MessageBoxButtonData buttons[3] = {
			{0, 1, "Save"}, {0, 2, "Don't Save"}, {0, 3, "Cancel"}
		};
		SDL_MessageBoxData mbd = {
			SDL_MESSAGEBOX_WARNING | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT,
			Game->window,
			"Unsaved data", message,
			SDL_arraysize(buttons), buttons, nullptr
		};
		bool ret = SDL_ShowMessageBox(&mbd, &buttonid);
		(void)ret;


		// cancel
		if(buttonid == 3) {
			return SDL_APP_CONTINUE;
		} 

		// save
		if(buttonid == 1) {
			auto saveproc = file->save;
			if(saveproc) saveproc(file, file->filename);
			file->wasEdited = false;
		}
	}


	{
		if(!fc->recents.filename) {
			getRecentsFilename(fc);
		}
		auto saveproc = fc->recents.save;
		if(saveproc) saveproc(&fc->recents, fc->recents.filename);
	}
#endif

	return SDL_APP_SUCCESS;
}

const char* getRecentsFilename(FileContext* fc)
{
	char* fn = calloc(4096, sizeof(char));
	SDL_snprintf(fn, 4096, "%slastopened.wdrecents", fc->prefsPath);
	fc->recents.filename = fn;
	return fn;
}

const char* getBackupPath(const char* filename, const char* group)
{
	size_t fnlen = SDL_strlen(filename);
	size_t minsize = i32max(512, 
		SDL_strlen(filename) + 
		SDL_strlen(group) + 
		globalFileContext->prefsPathLen + 
		sizeof("///.backup.000"));

	uint64_t x = time(0) + SDL_GetTicks();
	uint64_t r = splitmix64(&x);

	char* output = SDL_calloc(minsize, sizeof(char));
	SDL_snprintf(output, minsize, "%s%s.%s.%x.backup", 
		globalFileContext->prefsPath,
		group,
		filename + getFilenameStart(filename, fnlen),
		(uint32_t)(r & 0xFFFF));
	return output;
}

void genericFileDialogProc(void* ud, const char* const* filelist, int filter)
{
	if(!filelist) {
		SDL_Log("an error occurred selecting a save file: %s", SDL_GetError());
		return;
	}

	if(!*filelist) {
		SDL_Log("user did not choose a file");
		return;
	}

	LoadedFileInfo* file = ud;


	int mode = file->dialogMode;
	if(mode == FileDialog_Save || mode == FileDialog_Export) {
		const char* extension = mode == FileDialog_Save ? 
			file->extension :
			file->exportExtension;
		const char* filename = filelist[0];
		bool freeFilename = false;
		if(!endsWith(filename, extension, SDL_strlen(extension))) {
			size_t fnlen = SDL_strlen(filename);
			fnlen += 64;
			char* newFilename = calloc(1, fnlen);
			SDL_snprintf(newFilename, fnlen, "%s.%s", filename, extension);
			filename = newFilename;
			freeFilename = true;
		}

		if(mode == FileDialog_Save) {
			if(file->save) {
				file->save(file, filename);
			}
		} else if(mode == FileDialog_Export) {
			if(file->export) {
				file->export(file, filename);
			}
		}

		if(freeFilename) {
			free((void*)filename);
		}
	} else if(mode == FileDialog_Load || mode == FileDialog_Import) {
		const char* filename = *filelist;
		int ret = 0;

		if(mode == FileDialog_Load) {
			if(file->load) {
				ret = file->load(file, filename);
			}
		} else if(mode == FileDialog_Import) {
			if(file->import) {
				ret = file->import(file, filename);
			}
		}

		if(ret < 0) {
			SDL_Log("failed to load file");
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Unable to load point file", 
				"There was a problem with the selected file. Check the log for details", 
				Game->window);
		} 
	}
}

SDL_DialogFileFilter genericFilter[] = {
	{"Webdraft File", ""},
	{"All files", "*"}
};

void openFileDialog(int mode, LoadedFileInfo* file, int flags)
{
	file->flags = flags;
	file->dialogMode = mode;

	if(mode == FileDialog_Save || mode == FileDialog_Export) {
		const char* extension = mode == FileDialog_Save ? 
			file->extension :
			file->exportExtension;
		genericFilter[0].pattern = extension;
		SDL_ShowSaveFileDialog(
			genericFileDialogProc,
			file, 
			Game->window, 
			genericFilter, SDL_arraysize(genericFilter), 
			nullptr);
	} else if(mode == FileDialog_Load || mode == FileDialog_Import) {
		const char* extension = mode == FileDialog_Load ? 
			file->extension :
			file->exportExtension;
		genericFilter[0].pattern = extension;
		SDL_ShowOpenFileDialog(
			genericFileDialogProc,
			file, 
			Game->window, 
			genericFilter, SDL_arraysize(genericFilter), 
			nullptr,
			false);
	}
}

void recentsCopy(char* dst, const char* src, int dstSize)
{
	if(!dst || !src) return;
	int len = i32min(dstSize - 1, SDL_strlen(src));
	SDL_memcpy(dst, src, len);
}
#define RECENT_COPY(rff, src) recentsCopy(rff, src, sizeof(rff))

int recentsFileSave(LoadedFileInfo* fi, const char* path)
{
	FileContext* fc = fi->userdata;
	RecentsFile* rf = calloc(1, sizeof(RecentsFile));
	rf->magic = 0x11223300;
	rf->version = 0x00110000;
	// should be zero-terminated
	RECENT_COPY(rf->pointPath, fc->points.filename);
	RECENT_COPY(rf->playerPath, fc->players.filename);
	RECENT_COPY(rf->draftPath, fc->draft.filename);
	RECENT_COPY(rf->teamsPath, fc->teams.filename);

	SDL_SaveFile(path, rf, sizeof(RecentsFile));
	fi->filename = path;

	return 0;
}

int recentsFileLoad(LoadedFileInfo* fi, const char* path)
{
	size_t size = 0;
	RecentsFile* rf = SDL_LoadFile(path, &size);
	if(size < sizeof(RecentsFile)) {
		return 1;
	}
	if(rf->magic != 0x11223300) {
		return 1;
	}

	FileContext* fc = fi->userdata;
	fc->points.filename = rf->pointPath;
	fc->players.filename = rf->playerPath;
	fc->draft.filename = rf->draftPath;
	fc->teams.filename = rf->teamsPath;
	fi->filename = path;

	return 0;
}

void setupRecents(FileContext* fc)
{
	fc->recents.load = recentsFileLoad;
	fc->recents.save = recentsFileSave;
	fc->recents.userdata = fc;
}
