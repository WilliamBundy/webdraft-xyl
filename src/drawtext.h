#pragma once
#include "wb_sdlgame.h"
#include "wb_spritefont.h"
#include "sprite.h"

extern wbsf_Spritefont globalDefaultFont;

#define GLYPH_W 8
#define GLYPH_H 16

static inline
void setupDefaultFont()
{
	wbsf_spritefontInit(
		&globalDefaultFont, 
		(int4){128, 0, 8, 16}, 
		16, 16, 
		Game->surface->pixels, 
		Game->surface->w, Game->surface->h);
	globalDefaultFont.texture = Game->texture;
}

static inline
float4 drawText(const char* text, float2 p, float maxw, Xform camera)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);

	wbsf_Packet pkt = {
		.text = text,
		.pos = p,
		.len = -1,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = -1,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
	};

	return wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, nullptr);
}

static inline 
float4 drawTextIn(const char* text, float2 p, float maxWidth, float scale, Xform camera)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);

	float oldscale = wbsf_Scale;
	wbsf_Scale = scale;
	wbsf_Packet pkt = {
		.text = text,
		.pos = pXformInv(p, camera),
		.len = -1,
		.numCharsToShow = -1,
		.maxWidth = maxWidth,
		.maxLines = -1,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
	};

	float4 ret =  wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, nullptr);
	wbsf_Scale = oldscale;
	return ret;
}

static inline 
float4 drawTextIn3(const char* text, float2 p, float maxWidth, int maxLines, float scale, Xform camera)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);

	float oldscale = wbsf_Scale;
	wbsf_Scale = scale;
	wbsf_Packet pkt = {
		.text = text,
		.pos = pXformInv(p, camera),
		.len = -1,
		.numCharsToShow = -1,
		.maxWidth = maxWidth,
		.maxLines = maxLines,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
	};

	float4 ret =  wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, nullptr);
	wbsf_Scale = oldscale;
	return ret;
}



static inline
float4 drawText2(const char* text, float2 p, size_t len, float maxw, Xform camera)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);
	wbsf_Packet pkt = {
		.text = text,
		.pos = p,
		.len = len,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = -1,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
	};
	return wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, nullptr);
}

static inline
float4 drawText2mono(const char* text, float2 p, size_t len, float maxw, Xform camera)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);
	wbsf_Packet pkt = {
		.text = text,
		.pos = p,
		.len = len,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = -1,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
		.flags = wbsf_Monospace
	};
	return wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, nullptr);
}

static inline
float4 drawText3(const char* text, float2 p, size_t len, float maxw, int maxLines, Xform camera, wbsf_Info* info)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);
	wbsf_Packet pkt = {
		.text = text,
		.pos = p,
		.len = len,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = maxLines,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
	};
	return wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, info);
}

static inline
float4 drawText3mono(const char* text, float2 p, size_t len, float maxw, int maxLines, Xform camera, wbsf_Info* info)
{
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);
	wbsf_Packet pkt = {
		.text = text,
		.pos = p,
		.len = len,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = maxLines,
		.cameraOffset = camera.pos,
		.zoom = camera.scale,
		.flags = wbsf_Monospace
	};
	return wbsf_drawText(Game->renderer, &globalDefaultFont, &pkt, info);
}




static inline float2 sizeText(const char* text, float maxw, wbsf_Info* info)
{
	wbsf_Packet pkt = {
		.text = text,
		.pos = 0,
		.len = -1,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = -1,
		.cameraOffset = 0,
		.zoom = 1,
		.flags = wbsf_SkipDrawing
	};
	return wbsf_drawText(nullptr, &globalDefaultFont, &pkt, info).zw;
}


static inline float2 sizeTextMono(const char* text, float maxw, wbsf_Info* info)
{
	wbsf_Packet pkt = {
		.text = text,
		.pos = 0,
		.len = -1,
		.numCharsToShow = -1,
		.maxWidth = maxw,
		.maxLines = -1,
		.cameraOffset = 0,
		.zoom = 1,
		.flags = wbsf_SkipDrawing | wbsf_Monospace 
	};
	return wbsf_drawText(nullptr, &globalDefaultFont, &pkt, info).zw;
}

static inline float2 sizeTextSimple(const char* text)
{
	float w = wbsf_stringWidth(text, SDL_strlen(text), &globalDefaultFont, 1.0f);
	return (float2){w, globalDefaultFont.line};
}

