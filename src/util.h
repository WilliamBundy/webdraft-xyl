#pragma once
#include "wb_gamemath.h"
#include <SDL3/SDL.h>
#include "wb_sdlgame.h"
#include "sprite.h"

static inline
SDL_FRect f4tofr(float4 f4)
{
	return (SDL_FRect){f4[0], f4[1], f4[2], f4[3]};
}

static inline
bool frect_contains(SDL_FRect fr, float2 p)
{
	return p.x > fr.x && p.y > fr.y && p.x < (fr.x + fr.w) && p.y < (fr.y + fr.h);
}

static inline
char toLower(char a)
{
	return a >= 'A' && a <= 'Z' ? (a - 'A' + 'a') : a;
}

static inline
bool isNumber(char a)
{
	return a >= '0' && a <= '9';
}

static inline
bool strIsNumber(const char* start, const char* end)
{
	while(start != end)  {
		if(!isNumber(*start)) return false;
		start++;
	}
	return true;
}

static inline 
int strToInt(const char* start, const char* end) 
{
	int x = 0;
	while(start != end)  {
		x *= 10;
		x += (int)(*start - '0');
		start++;
	}
	return x;
}


static inline
int stringDistance2(const char* a, const char* b)
{
	int blen = SDL_strlen(b);
	int alen = SDL_strlen(a);


	if(SDL_strncmp(a, b, 78) == 0) return 0;
	int len = i32min(alen, blen);

	int count = 0;
	for(int i = 0; i < len; ++i) {
		if(toLower(a[i]) != toLower(b[i])) {
			count++;
		}
	}

	return count;
}

static inline
int stringDistanceMatchLen(const char* a, const char* b)
{
	int blen = SDL_strlen(b);
	int alen = SDL_strlen(a);

	if(alen != blen) return 1000;

	if(SDL_strncmp(a, b, 78) == 0) return 0;
	int len = i32min(alen, blen);

	int count = 0;
	for(int i = 0; i < len; ++i) {
		if(toLower(a[i]) != toLower(b[i])) {
			count++;
		}
	}

	return count;
}



static inline
bool keyDown(int key)
{
	return Game->input->keys[key] >= KEY_PRESSED;
}

static inline
bool keyJustDown(int key)
{
	return Game->input->keys[key] == KEY_JUST_PRESSED;
}

static inline
bool keyUp(int key)
{
	return Game->input->keys[key] == KEY_RELEASED;
}

static inline
bool keyJustUp(int key)
{
	return Game->input->keys[key] == KEY_JUST_RELEASED;
}


static inline
bool mbtnDown(int btn)
{
	return Game->input->mbtn[btn] >= KEY_PRESSED;
}

static inline
bool mbtnUp(int btn)
{
	return Game->input->mbtn[btn] <= KEY_RELEASED;
}

static inline
bool mbtnJustDown(int btn)
{
	return Game->input->mbtn[btn] == KEY_JUST_PRESSED;
}


static inline
bool mbtnJustUp(int btn)
{
	return Game->input->mbtn[btn] == KEY_JUST_RELEASED;
}


static inline
void setRgb(uint32_t rgb, float scale)
{
	SDL_SetTextureColorMod(Game->texture, 
		(uint8_t)(((float)((rgb >> 16) & 0xFF) / 255.0 * scale) * 255), 
		(uint8_t)(((float)((rgb >> 8) & 0xFF) / 255.0 * scale) * 255), 
		(uint8_t)(((float)(rgb & 0xFF) / 255.0 * scale) * 255));
}

static inline 
void drawRect(uint32_t rgb, SDL_FRect fr, Xform camera)
{
	setRgb(rgb, 1.0);
	fr = rXformRect(fr, camera);
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
}

static inline
void drawRectAlpha(uint32_t rgb, float alpha, SDL_FRect fr, Xform camera)
{
	setRgb(rgb, 1.0);
	SDL_SetTextureAlphaModFloat(Game->texture, alpha);
	fr = rXformRect(fr, camera);
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
	SDL_SetTextureAlphaMod(Game->texture, 255);
}

static inline
void drawFloat4(uint32_t rgb, float4 rect)
{
	setRgb(rgb, 1.0);
	SDL_FRect fr = {rect.x, rect.y, rect.z, rect.w};
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
}


static inline
void drawFloat4Alpha(uint32_t rgb, float alpha, float4 rect)
{
	setRgb(rgb, 1.0);
	SDL_SetTextureAlphaModFloat(Game->texture, alpha);
	SDL_FRect fr = {rect.x, rect.y, rect.z, rect.w};
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
	SDL_SetTextureAlphaMod(Game->texture, 255);
}
static inline
void drawFloat4Camera(uint32_t rgb, float4 rect, Xform camera)
{
	setRgb(rgb, 1.0);
	SDL_FRect fr = pXformRect(rect.xy, rect.zw, camera);
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
	SDL_SetTextureAlphaMod(Game->texture, 255);
}


static inline
void drawFloat4AlphaCamera(uint32_t rgb, float alpha, float4 rect, Xform camera)
{
	setRgb(rgb, 1.0);
	SDL_SetTextureAlphaModFloat(Game->texture, alpha);
	SDL_FRect fr = pXformRect(rect.xy, rect.zw, camera);
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	SDL_SetTextureAlphaMod(Game->texture, 255);
	setRgb(0xFFFFFF, 1.0f);
}

static inline
void drawOutlineCamera(uint32_t rgb, float4 box, float thickness, Xform camera)
{
	SDL_FRect dst = pXformRect(box.xy, box.zw, camera);
	uint32_t color = rgb;
	SDL_SetTextureColorMod(Game->texture, 
		(color >> 16) & 0xFF, 
		(color >>  8) & 0xFF, 
		(color >>  0) & 0xFF);

	SDL_FRect src = {1,1,1,1};

	SDL_FRect line = dst;

	line.w = thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line.x += dst.w;
	line.x -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line = dst;
	line.h = thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line.y += dst.h;
	line.y -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);
}


static inline
void drawOutline(uint32_t rgb, float4 box, float thickness)
{
	SDL_FRect dst = {box.x, box.y, box.z, box.w};
	uint32_t color = rgb;
	SDL_SetTextureColorMod(Game->texture, 
		(color >> 16) & 0xFF, 
		(color >>  8) & 0xFF, 
		(color >>  0) & 0xFF);

	SDL_FRect src = {1,1,1,1};

	SDL_FRect line = dst;

	line.w = thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line.x += dst.w;
	line.x -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line = dst;
	line.h = thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line.y += dst.h;
	line.y -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);
}

static inline
int endsWith(const char* a, const char* s, size_t slen)
{
	size_t alen = strlen(a);
	for(size_t i = 0; i < slen; ++i)
	{
		if(a[i+alen-slen] != s[i])
		{
			return 0;
		}
	}
	return 1;
}


static inline
size_t getFilenameStart(const char* filename, size_t len)
{
	while(--len >= 0) {
		if(filename[len] == '/' || filename[len] == '\\') {
			return len + 1;
		}
	}
	return 0;
}

static inline
size_t getExtensionStart(const char* filename, size_t len)
{
	while(--len >= 0) {
		if(filename[len] == '.') {
			return len;
		}
	}
	return 0;
}
