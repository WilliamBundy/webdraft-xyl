#pragma once

#include <SDL3/SDL.h>
#include "wb_gamemath.h"
#include "sprite.h"

#include <stdarg.h>

enum {
	Gui_VertDown,
	Gui_HorizRight,
	Gui_VertUp,
	Gui_HorizLeft,
};

static const float2 Gui_DirVec[] = {
	{0, 1},
	{1, 0},
	{0, -1},
	{-1, 0},
};

enum {
	GuiCtx_Enabled = 1<<0,
	GuiCtx_Column = 1<<1
};

enum {
	Color_Dark,
	Color_Dim,
	Color_Mid,
	Color_Light,
	Color_White,
	Color_Black,
	Color_Accent,
	Gui_Num_Colors
};

enum {
	Slider_ShowButtons = 1<<0
};

typedef struct GuiSlider
{
	float2 range;
	float2 grab;
	float2 orig;
	float2 inc;
	// formats a float
	const char* fmt;
	int state, flags;
} GuiSlider;


enum
{
	Textbox_BlockInput = 1<<0
};

typedef struct GuiTextbox
{
	char* buffer;
	int numChars, maxChars;
	int flags, reserved;
} GuiTextbox;

void gui_init_textbox(GuiTextbox* tb, char* buffer, int maxChars)
{
	tb->buffer = buffer;
	tb->maxChars = maxChars;
	tb->numChars = 0;
	tb->flags = 0;
	tb->reserved = 0;
}

typedef struct GuiStyle
{
	uint32_t textColor;
	uint32_t textColorDim;
	uint32_t colors[Gui_Num_Colors];

	float buttonElevation;
	int buttonAlign;
} GuiStyle;

typedef struct GuiFrame
{
	float2 pen;
	float2 maxSize;
	float column, row;
	int direction, flags;
} GuiFrame;

typedef struct GuiContext
{
	int direction, flags;
	float2 pen, lastPen;
	float2 lastSize;
	float2 maxSize;
	GuiFrame* frameStack;
	int numFrames, maxFrames;
	Xform camera;

	int mbtn[4];
	float2 mpos;

	float margin, padding, column, row;

	char* fmtBuffer;
	size_t fmtBufferSize;
	GuiStyle* style, defaultStyle;

	GuiTextbox* focusedTextbox;
} GuiContext;
extern GuiContext* Gui;

enum {
	Gui_Disabled = 1<<0,
	Gui_Highlighted = 1<<1,
	Gui_Darkened = 1<<2,
	Gui_Button_Mono = 1<<28,
	Gui_Button_NoIncrement = 1<<29,
	Gui_Button_Outline = 1<<30,
	Gui_Button_NoElevation = 1<<31
};

enum {
	Align_Left,
	Align_Right,
	Align_Center
};


static const int2 AlignTopLeft = Align_Left;
static const int2 AlignTopRight = {Align_Right, Align_Left};
static const int2 AlignBottomRight = Align_Right;
static const int2 AlignBottomLeft = {Align_Left, Align_Right};
static const int2 AlignCenterLeft = {Align_Left, Align_Center};
static const int2 AlignCenterRight = {Align_Right, Align_Center};
static const int2 AlignTopCenter = {Align_Center, Align_Left};
static const int2 AlignBottomCenter = {Align_Center, Align_Right};

static inline
void gui_init(GuiContext* gui) 
{
	gui->camera = Xzero;
	gui->margin = 4;
	gui->padding = 4;
	gui->style = &gui->defaultStyle;
	gui->flags = GuiCtx_Enabled;
	gui->defaultStyle = (GuiStyle){
		.textColor = 0xFFFFFF,
		.textColorDim = 0x888888,
		.colors = {
			[Color_Dark] = 0x222222,
			[Color_Dim] = 0x444444,
			[Color_Mid] = 0x777777,
			[Color_Light] = 0xAAAAAA,
			[Color_White] = 0xFFFFFF,
			[Color_Black] = 0x000000,
			[Color_Accent] = 0x33bbff
		},
		.buttonElevation = 4,
		.buttonAlign = Align_Center
	};

	gui->column = -1;
	gui->row = -1;

	gui->fmtBufferSize = 1<<16;
	gui->fmtBuffer = calloc(gui->fmtBufferSize, sizeof(char));

	gui->maxFrames = 64;
	gui->frameStack = calloc(gui->maxFrames, sizeof(GuiFrame));
}

static inline
void gui_push_frame(GuiContext* gui)
{
	if(gui->numFrames >= gui->maxFrames) {
		return;
	}

	GuiFrame fr = {
		gui->pen, gui->maxSize, gui->column, gui->row, gui->direction, gui->flags
	};
	gui->frameStack[gui->numFrames++] = fr;
}

static inline
GuiFrame gui_pop_frame(GuiContext* gui)
{
	if(gui->numFrames == 0) {
		GuiFrame fr = {
			gui->pen, gui->maxSize, gui->column, gui->row, gui->direction, gui->flags
		};
		return fr;
	}
	return gui->frameStack[--gui->numFrames];
}

static inline
void gui_restore_frame(GuiContext* gui)
{
	GuiFrame frame = gui_pop_frame(gui);
	gui->direction = frame.direction;
	gui->pen = frame.pen;
	gui->column = frame.column;
	gui->row = frame.row;
	gui->flags = frame.flags;
	gui->maxSize = frame.maxSize;
}




int gui_handle_text_input(SDL_Event event, GuiContext* gui);
int gui_handle_key_down(SDL_Event event, GuiContext* gui);

void gui_update(GuiContext* gui);
void gui_increment(GuiContext* gui, float2 size);
void gui_draw_box(GuiContext* gui, float4 box, int colorEnum);

void gui_vbox(GuiContext* gui);
void gui_hbox(GuiContext* gui);
void gui_popbox(GuiContext* gui);

bool gui_button(GuiContext* gui, int flags, const char* label);
bool gui_button_fmt(GuiContext* gui, int flags, const char* fmt, ...);
bool gui_button_img(GuiContext* gui, int flags, SDL_Texture* texture, SDL_FRect src, float2 size);

void gui_img(GuiContext* gui, int flags, int2 align, SDL_Texture* texture, SDL_FRect src, float2 size);

void gui_label(GuiContext* gui, int flags, int2 align, const char* label, size_t len);
void gui_label_fmt(GuiContext* gui, int flags, int2 align, const char* fmt, ...);

bool gui_checkbox(GuiContext* gui, int flags, bool* check, const char* label);
bool gui_slider(GuiContext* gui, GuiSlider* slider, float width, float* value);
void gui_textbox(GuiContext* gui, int flags, float width, GuiTextbox* tb);

static inline void uiUpdate() { gui_update(Gui); }
static inline void uiVbox() { gui_vbox(Gui); }
static inline void uiHbox() { gui_hbox(Gui); }
static inline void uiPop() { gui_popbox(Gui); }
void uiIncrement(float2 size) { gui_increment(Gui, size); }
void uiSpacer(float2 size) { gui_increment(Gui, size * Gui_DirVec[Gui->direction]); }

static inline bool uiButton(int flags, const char* label) { return gui_button(Gui, flags, label); }
static inline bool uiButtonFmt(int flags, const char* fmt, ...)
{
	va_list args;
	va_start(args);
	SDL_vsnprintf(Gui->fmtBuffer, Gui->fmtBufferSize, fmt, args);
	Gui->fmtBuffer[Gui->fmtBufferSize-1] = 0;
	return gui_button(Gui, flags, Gui->fmtBuffer);
}

static inline bool uiButtonImg(int flags, SDL_Texture* texture, SDL_FRect src, float2 size) { return gui_button_img(Gui, flags, texture, src, size); }
static inline void uiImg(int flags, int2 align, SDL_Texture* texture, SDL_FRect src, float2 size) { gui_img(Gui, flags, align, texture, src, size); }
static inline void uiLabel(int flags, int2 align, const char* label, size_t len) { gui_label(Gui, flags, align, label, len); }
static inline void uiLabelFmt(int flags, int2 align, const char* fmt, ...) 
{
	va_list args;
	va_start(args);
	size_t len = SDL_vsnprintf(Gui->fmtBuffer, Gui->fmtBufferSize, fmt, args);
	Gui->fmtBuffer[Gui->fmtBufferSize-1] = 0;
	gui_label(Gui, flags, align, Gui->fmtBuffer, len);
}


static inline bool uiCheckbox(int flags, bool* check, const char* label) { return gui_checkbox(Gui, flags, check, label); }
static inline bool uiSlider(GuiSlider* slider, float width, float* value) { return gui_slider(Gui, slider, width, value); }

static inline void uiTextbox(int flags, float width, GuiTextbox* tb) { return gui_textbox(Gui, flags, width, tb); }


