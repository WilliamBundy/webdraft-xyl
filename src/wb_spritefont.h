/*
Copyright 2026 William Bundy, all rights reserved.

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

This is a quick and dirty implementation of some standard text layout for 
ASCII spritefonts and english text. It accepts a bitmap with glyphs in ascii
order (starting with 33, '!'), breaks them out individually, trims transparency
along the edges, and stores that in the spritefont struct. When drawing, you 
have the option of using the original width or the trimmed width, which
for me corresponds to mono or proportional layout. 

I usually wrap wbsf_drawText in a few different ways to make it less awkward
to use, usually for each different kind of text/font and using a global 
renderer/texture/camera to cut down on args

Status:
Mostly tested, probably works.
*/


#ifndef _WB_SPRITEFONT_H_ 
#define _WB_SPRITEFONT_H_

#include <stdint.h>
#include <SDL3/SDL.h>
#include "wb_gamemath.h"

extern float wbsf_Scale;

typedef struct Spritefont
{
	union {
		int4 region;
		struct {
			int x, y, w, h;
		};
	};
	int chrWidth[256];
	int chrLSB[256];
	int row, line;

	void* texture;
} wbsf_Spritefont;

enum
{
	wbsf_Monospace = 1<<0,
	wbsf_SkipDrawing = 1<<1
};

typedef struct wbsf_Packet
{
	const char* text;
	float2 pos;
	size_t len;
	size_t numCharsToShow;
	float maxWidth;
	int maxLines;

	float2 cameraOffset;
	float zoom;
	int flags;

	float scale;
	int reserved;

} wbsf_Packet;


typedef struct wbsf_Info
{
	float4 bbox;
	float2 lastPen;
	size_t numPrinted;
} wbsf_Info;


// region: x/y: pos of top left corner of char pixels
//		   z/w: size of an individula glyph 
// row: glyphs per row, must be a power of 2 (it's usually 16)
// line: actual line height for spacing
// pxw/pxh: width/height of bitmap
void wbsf_spritefontInit(wbsf_Spritefont* sfm, int4 region, int row, int line, uint32_t* pixels, int pxw, int pxh);
float4 wbsf_drawText(SDL_Renderer* renderer, wbsf_Spritefont* sfm, wbsf_Packet* packet, wbsf_Info* info);
/*
float4 wbsf_drawText(
	SDL_Renderer* renderer, 
	wbsf_Spritefont* sfm, 
	const char* text, 
	float2 p, 
	size_t len, 
	size_t numCharsToShow, 
	float maxWidth, 
	int maxLines,
	SDL_Texture* texture, 
	float2 cameraOffset, 
	float zoom, 
	bool mono, 
	bool doDraw);
*/

static inline
SDL_FRect wbsf_glyphSrc(wbsf_Spritefont* sfm, int g)
{
	g -= 33;
	return (SDL_FRect) {
		(int)(g & (sfm->row-1)) * sfm->w + sfm->x,
		(int)(g / (sfm->row)) * sfm->h + sfm->y,
		sfm->w,
		sfm->h
	};
}

float wbsf_stringWidth(const char* text, size_t len, wbsf_Spritefont* sfm, float scale);


//float4 wbsf_drawText(SDL_Renderer* renderer, wbsf_Spritefont* sfm, const char* text, float2 p, size_t len, size_t numCharsToShow, float maxWidth, int maxLines, SDL_Texture* texture, float2 cameraOffset, float zoom, bool mono, bool doDraw)

#endif


#ifdef WB_SPRITEFONT_IMPLEMENTATION
#ifndef _WB_SPRITEFONT_C_
#define _WB_SPRITEFONT_C_

static inline
bool wbsf_charIsUpper(int c)
{
	return c >= 'A' && c <= 'Z';
}

static inline 
bool wbsf_charIsLower(int c)
{
	return c >= 'a' && c <= 'z';
}


static inline
bool wbsf_charIsNumber(int c)
{
	return c >= '0' && c <= '9';
}

static inline
bool wbsf_charIsLetter(int c)
{
	return wbsf_charIsUpper(c) || wbsf_charIsLower(c);
}

static inline
bool wbsf_charIsAlphanumeric(int c)
{
	return wbsf_charIsLetter(c) || wbsf_charIsNumber(c);
}


static inline
SDL_FRect wbsf_pXformRect(float2 p, float2 size, float2 offset, float scale, float2 origin)
{
	float2 pos = (p - offset) * scale + origin;
	return (SDL_FRect){pos.x, pos.y, size.x * scale, size.y * scale};
}

void wbsf_spritefontInit(wbsf_Spritefont* sfm, int4 region, int row, int line, uint32_t* pixels, int pxw, int pxh)
{
	sfm->region = region;
	sfm->row = row;
	sfm->line = line;

	for(int i = 0; i < 256; ++i) {
		sfm->chrWidth[i] = sfm->w;
		sfm->chrLSB[i] = 0;
	}

	for(int i = 33; i <= 127; ++i) {
		int w = -1;
		int lsb = -1;
		int srcx = (int)((i-33) & (sfm->row-1)) * sfm->w + sfm->x;
		int srcy = (int)((i-33) / (sfm->row)) * sfm->h + sfm->y;
		for(int x = 0; x < sfm->w; ++x) {
			for(int y = 0; y < sfm->h; ++y) {
				uint32_t px = pixels[(srcy + y) * pxw + (srcx + x)];
				if(px != 0) {
					lsb = x;
					break;
				}
			}
			if(lsb != -1) {
				break;
			}
		}
		for(int x = sfm->w-1; x >= 0; x--) {
			for(int y = 0; y < sfm->h; ++y) {
				uint32_t px = pixels[(srcy + y) * pxw + (srcx + x)];
				if(px != 0) {
					w = x;
					break;
				}
			}
			if(w != -1) {
				break;
			}
		}
		w -= lsb;
		sfm->chrWidth[i] = w + 1;
		sfm->chrLSB[i] = lsb;
	}
}


float wbsf_Scale = 1.0f;

// TODO implement proper word wrap by adding up the widths of processed characters
// and subtracting them when they're written to the screen. There's no way to 

float wbsf_stringWidth(const char* text, size_t len, wbsf_Spritefont* sfm, float scale)
{
	float w = 0; 
	while(len && --len >= 0) {
		char c = text[len];
		if(c == ' ') {
			w += sfm->w / 2;
		} else {
			w += (sfm->chrWidth[(int)c]) * scale + 1;
		}
	}
	return w;
}

//float4 wbsf_drawText(SDL_Renderer* renderer, wbsf_Spritefont* sfm, const char* text, float2 p, size_t len, size_t numCharsToShow, float maxWidth, int maxLines, SDL_Texture* texture, float2 cameraOffset, float zoom, bool mono, bool doDraw)
float4 wbsf_drawText(SDL_Renderer* renderer, wbsf_Spritefont* sfm, wbsf_Packet* packet, wbsf_Info* info)
{
	float2 p = packet->pos;
	size_t len = packet->len;
	size_t numCharsToShow = packet->numCharsToShow;
	float maxWidth = packet->maxWidth;
	int maxLines = packet->maxLines;
	float2 cameraOffset = packet->cameraOffset;
	float zoom = packet->zoom;
	bool mono = packet->flags & wbsf_Monospace;
	bool doDraw = ~packet->flags & wbsf_SkipDrawing;
	const char* text = packet->text;

	if(len == -1) {
		len = SDL_strlen(text);
	}

	if(numCharsToShow == -1 || numCharsToShow > len) {
		numCharsToShow = len - 1;
	}

	int line = 0;
	bool bail = false;
	size_t lasti = 0;
	float2 pen = 0;
	float2 ext = {sfm->w, sfm->h};
	float lastX = 0;
	//bool usedLine = false;
	size_t numPrinted = 0;
	for(size_t i = 0; i < len; ++i) {
		int c = text[i];
		if(c < 0) c += 128 + 127;
		if(c != '\n' && (c < ' ' || c > '~')) continue;

		if((wbsf_charIsAlphanumeric(c) || c == '\'') && i < len - 1) continue;
		size_t llen = i - lasti;
		// this llen * sfm->w needs to be replaced with stringwidth(lasti, i)
		if(maxWidth > 0 && wbsf_stringWidth(text + lasti, llen, sfm, zoom) + pen.x > maxWidth) {
			line++;
			ext = f2max(pen, ext);
			lastX = pen.x;
			pen.x = 0;
			pen.y += sfm->line;
			//usedLine = false;
		}

		// this returns the last word to the current line if we're wrapping
		if(pen.x == 0) {
			if(maxLines > 0 && line >= maxLines) {
				pen.x = lastX;
				pen.y -= sfm->line;
				len = i;
				numCharsToShow = len - 1;
			}
		}

		//bool nonSpace = false;
		for(size_t j = lasti; j <= i; ++j) {
			if(j > numCharsToShow) break;
			numPrinted++;
			// this prevents anything from being shown on invalid lines
			if(maxLines > 0 && pen.y >= maxLines * sfm->line - 1) {
				numPrinted--;
				break;
			}
			if(text[j] == ' ') {
				if(mono || pen.x != 0) {
					pen.x += mono ? sfm->w : sfm->w / 2;
				}
				continue;
			} else if(text[j] == '\n') {
				line++;
				ext = f2max(pen, ext);
				pen.x = 0;
				pen.y += sfm->line;

				continue;
			}
			//nonSpace = true;
			int g = text[j];

			SDL_FRect src = wbsf_glyphSrc(sfm, g);
			if(!mono) {
				src.x += sfm->chrLSB[g];
				src.w = sfm->chrWidth[g];
			}

			// this prevents anything from being shown too far to the right
			if(maxWidth > 0 && pen.x + src.w >= maxWidth) {
				numPrinted--;
				break;
			}
			
			SDL_FRect dst = wbsf_pXformRect(pen + p, (float2){src.w, sfm->h} * wbsf_Scale, cameraOffset, zoom, 0);
			if(doDraw) SDL_RenderTexture(renderer, sfm->texture, &src, &dst);
			if(!mono) {
				pen.x += (src.w + 1) * wbsf_Scale;
			} else {
				pen.x += sfm->w * wbsf_Scale;
			}

			ext = f2max(pen, ext);
		}
		if(bail) {
			break;
		}
		lasti = i + 1;
		//if(nonSpace) {
			//usedLine = true;
		//}

		if(lasti > numCharsToShow) {
			break;
		}	
	}

	if(info) {
		info->lastPen = pen;
		info->bbox = (float4){p.x, p.y, ext.x, ext.y};
		info->numPrinted = numPrinted;
	}

	return (float4){pen.x, pen.y, ext.x, ext.y};
}

#endif
#endif