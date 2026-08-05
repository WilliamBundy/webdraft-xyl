#include "wb_sdlgame.h"
#include "gui.h"

#include "drawtext.h"
#include "util.h"

#include "wb_dict32.h"

#include <string.h>
#include <stdio.h>

// text input

int gui_handle_text_input(SDL_Event event, GuiContext* gui)
{
	GuiTextbox* tb = gui->focusedTextbox;
	if(!tb) return 0;

	if(tb->flags & Textbox_BlockInput) {
		return 1;
	}

	size_t len = SDL_strlen(event.text.text);
	if(len + tb->numChars > tb->maxChars) {
		len = tb->maxChars - tb->numChars - 1;
	}

	SDL_memcpy(tb->buffer + tb->numChars, event.text.text, len);
	tb->numChars += len;
	return 1;
}

int gui_handle_key_down(SDL_Event event, GuiContext* gui)
{
	if(event.key.scancode == SDL_SCANCODE_TAB) return 0;
	if(event.key.scancode == SDL_SCANCODE_RETURN) return 0;
	if((event.key.mod & SDL_KMOD_CTRL) && 
		event.key.key != SDLK_V &&
		event.key.key != SDLK_C &&
		event.key.scancode != SDL_SCANCODE_BACKSPACE) return 0;
	if(event.type == SDL_EVENT_KEY_UP) return 0;

	GuiTextbox* tb = gui->focusedTextbox;
	if(!tb) return 0;

	if(event.key.scancode == SDL_SCANCODE_ESCAPE) {
		gui->focusedTextbox = nullptr;
		return 1;
	}

	if(event.key.scancode == SDL_SCANCODE_BACKSPACE) {
		if(event.key.mod & SDL_KMOD_CTRL) {
			while(tb->numChars > 0 && tb->buffer[tb->numChars - 1] != ' ') {
				tb->buffer[--tb->numChars] = 0;
			}
			if(tb->numChars > 0 && tb->buffer[tb->numChars - 1] == ' ') {
				tb->buffer[--tb->numChars] = 0;
			}
		} else {
			if(tb->numChars > 0)
				tb->buffer[--tb->numChars] = 0;
		}
	}

	if(event.key.key == SDLK_V && (event.key.mod & SDL_KMOD_CTRL)) {
		if(tb->flags & Textbox_BlockInput) {
			return 1;
		}

		char* clip = SDL_GetClipboardText();
		size_t len = SDL_strlen(clip);
		if(len + tb->numChars > tb->maxChars) {
			len = tb->maxChars - tb->numChars;
		}

		SDL_memcpy(tb->buffer + tb->numChars, clip, len);
		tb->numChars += len;

		SDL_free(clip);
	}

	if(event.key.key == SDLK_C && (event.key.mod & SDL_KMOD_CTRL)) {
		SDL_SetClipboardText(tb->buffer);
	}
	return 1;
}
/*

*** NEW ***
 
GuiContext-based UI (aka, a lot of my ideas for filthyui)

*/


void gui_draw_box(GuiContext* gui, float4 box, int colorEnum)
{
	SDL_FRect dst = pXformRect(box.xy, box.zw, gui->camera);
	uint32_t color = gui->style->colors[colorEnum];
	SDL_SetTextureColorMod(Game->texture, 
		(color >> 16) & 0xFF, 
		(color >>  8) & 0xFF, 
		(color >>  0) & 0xFF);
	SDL_RenderTexture(Game->renderer, Game->texture, &(SDL_FRect){1,1,1,1}, &dst);
}

void gui_draw_outline(GuiContext* gui, float4 box, float thickness, int colorEnum)
{
	SDL_FRect dst = pXformRect(box.xy, box.zw, gui->camera);
	uint32_t color = gui->style->colors[colorEnum];
	SDL_SetTextureColorMod(Game->texture, 
		(color >> 16) & 0xFF, 
		(color >>  8) & 0xFF, 
		(color >>  0) & 0xFF);

	SDL_FRect src = {1,1,1,1};

	SDL_FRect line = dst;

	line.w = thickness;
	//line.x -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line.x += dst.w;
	line.x -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line = dst;
	//line.x = thickness;
	//line.y = thickness;
	//line.w += thickness * 2;
	line.h = thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);

	line.y += dst.h;
	line.y -= thickness;
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &line);
}



void gui_draw_tex(GuiContext* gui, SDL_Texture* tex, SDL_FRect src, float4 box)
{
	SDL_FRect dst = pXformRect(box.xy, box.zw, gui->camera);
	SDL_SetTextureColorMod(Game->texture, 0xFF, 0xFF, 0xFF);
	SDL_RenderTexture(Game->renderer, tex, &src, &dst);

}

void gui_update(GuiContext* gui)
{
	gui->mbtn[SDL_BUTTON_LEFT] = Game->input->mbtn[SDL_BUTTON_LEFT];
	gui->mbtn[SDL_BUTTON_MIDDLE] = Game->input->mbtn[SDL_BUTTON_MIDDLE];
	gui->mbtn[SDL_BUTTON_RIGHT] = Game->input->mbtn[SDL_BUTTON_RIGHT];
	gui->mpos = pXformInv(Game->input->mpos, gui->camera);
	//gui->maxSize = 0;

	// TODO clear stacks and set pen?
}

bool gui_check_enabled(GuiContext* gui, int flags)
{
	return (~flags & Gui_Disabled) && (gui->flags & GuiCtx_Enabled);
}

bool gui_mbtn_down(GuiContext* gui)
{
	return gui->mbtn[SDL_BUTTON_LEFT] >= KEY_PRESSED;
}

bool gui_mbtn_up(GuiContext* gui)
{
	return gui->mbtn[SDL_BUTTON_LEFT] <= KEY_RELEASED;
}

bool gui_mbtn_justUp(GuiContext* gui)
{
	return gui->mbtn[SDL_BUTTON_LEFT] == KEY_JUST_RELEASED;
}
bool gui_mbtn_justDown(GuiContext* gui)
{
	return gui->mbtn[SDL_BUTTON_LEFT] == KEY_JUST_PRESSED;
}

void gui_column(GuiContext* gui, float width)
{
	gui->column = width;
}



void gui_increment(GuiContext* gui, float2 size)
{
	// TODO frame expand
	// TODO handle grid effect

	gui->lastPen = gui->pen;
	gui->pen += size * Gui_DirVec[gui->direction];
	gui->pen += gui->margin * Gui_DirVec[gui->direction];
	gui->lastSize = size;
	gui->maxSize = f2max(gui->maxSize, size);
}

void gui_hbox(GuiContext* gui)
{
	gui_push_frame(gui);
	gui->direction = Gui_HorizRight;
	gui->maxSize = 0;
}

void gui_vbox(GuiContext* gui)
{
	gui_push_frame(gui);
	gui->direction = Gui_VertDown;
	gui->maxSize = 0;
}

void gui_popbox(GuiContext* gui)
{
	// TODO we should probably be commuting the entire size of the 
	// frame with the gui->increment rather than just the max size
	// This causes problems where popping a frame causes the pen
	// to be in the wrong position right now
	GuiFrame frame = gui_pop_frame(gui);
	gui->direction = frame.direction;
	gui->pen = frame.pen;
	gui_increment(gui, gui->maxSize);
	gui->column = frame.column;
	gui->row = frame.row;
	gui->flags = frame.flags;
	gui->maxSize = f2max(gui->maxSize, frame.maxSize);
}

static inline
float align_in(float box, float w, int align, float pad)
{
	float x = pad;
	if(align == Align_Center) {
		return (box - w) / 2;
	} else if(align == Align_Right) {
		return box - w - pad;
	}	
	return x;
}

bool gui_button_base(GuiContext* gui, int flags, int2 align, float2 size, float2* contentPos, float4* regionOut)
{
	float4 region;
	region.xy = gui->pen;

	if(~flags & Gui_Button_NoIncrement) {
		region.z = gui->column > 0 ? gui->column : size.x + gui->padding * 2;
		region.w = gui->row > 0 ? gui->row : size.y + gui->padding * 2;
	} else {
		region.zw = size + gui->padding * 2;
	}

	float2 pos;
	pos.x = align_in(region.z, size.x, align.x, gui->padding);
	pos.y = align_in(region.w, size.y, align.y, gui->padding);

	bool hover = rect_contains(region, gui->mpos);
	bool down = hover && gui_mbtn_down(gui);
	float2 area = region.zw;
	if(~flags & Gui_Button_NoElevation) {
		area.y += gui->style->buttonElevation;
		if(down) {
			region.y += gui->style->buttonElevation;
		} else {
			float4 ev = region;
			ev.y += region.w;
			ev.w = gui->style->buttonElevation;
			gui_draw_box(gui, ev, Color_Dark);
		}
	}

	uint32_t color = Color_Dim;
	if(hover || (flags & Gui_Highlighted)) {
		color = Color_Light;
		if(down) {
			color = Color_Accent;
		}
	}

	if(flags & Gui_Button_Outline) {
		gui_draw_outline(gui, region, 1, color);
	} else {
		gui_draw_box(gui, region, color);
	}

	if(~flags & Gui_Button_NoIncrement) {
		gui_increment(gui, area);
	}

	if(contentPos) *contentPos = region.xy + pos;
	if(regionOut) *regionOut = region;

	return hover && gui_check_enabled(gui, flags) && gui_mbtn_justUp(gui);
}

bool gui_button(GuiContext* gui, int flags, const char* label)
{
	// TODO should probably manage word wrap for buttons
	float2 pos;
	float2 size;
	size_t len;
	if(flags & Gui_Button_Mono) {
		len = strlen(label) ;
		size.x = len * GLYPH_W;
		size.y = GLYPH_H;
	} else {
		size = sizeTextSimple(label);
	}
	bool ret = gui_button_base(gui, flags, Align_Center, size, &pos, nullptr);
	if(flags & Gui_Button_Mono) {
		drawText2mono(label, pos, len, -1, gui->camera);
	} else {
		drawText(label, pos, -1, gui->camera);
	}
	return ret;
}

bool gui_button_fmt(GuiContext* gui, int flags, const char* label, ...)
{
	va_list args;
	va_start(args);
	SDL_vsnprintf(gui->fmtBuffer, gui->fmtBufferSize, label, args);
	gui->fmtBuffer[gui->fmtBufferSize-1] = 0;
	return gui_button(gui, flags, gui->fmtBuffer);
}

bool gui_button_img(GuiContext* gui, int flags, SDL_Texture* texture, SDL_FRect src, float2 size)
{
	if(size.x == -1 || size.y == -1) {
		size.x = src.w;
		size.y = src.h;
	}

	float2 pos;
	bool ret = gui_button_base(gui, flags, Align_Center, size, &pos, nullptr);

	float4 dst;
	dst.xy = pos;
	dst.zw = size;
	gui_draw_tex(gui, texture, src, dst);

	return ret;
}

void gui_img(GuiContext* gui, int flags, int2 align, SDL_Texture* texture, SDL_FRect src, float2 size)
{
	if(size.x == -1 || size.y == -1) {
		size.x = src.w;
		size.y = src.h;
	}

	float pad = (flags & Gui_Highlighted) ? gui->padding : 0;

	float2 pos = pad; 
	if(gui->column > 0) {
		pos.x = align_in(gui->column, size.x, align.x, pad);
	}
	if(gui->row > 0) {
		pos.y = align_in(gui->row, size.y, align.y, pad);
	}

	float4 dst;
	dst.xy = gui->pen + pos;
	dst.zw = size;

	float2 area = dst.zw;
	if(flags & Gui_Highlighted) {
		float4 box = dst;
		box.xy = gui->pen;
		box.z = gui->column > 0 ? gui->column : box.z + pad*2;
		box.w = gui->row > 0 ? gui->row : box.w + pad*2;
		area.xy = box.zw;
		int color = Color_Light;
		if(flags & Gui_Darkened) {
			color = Color_Black;
		}
		gui_draw_box(gui, box, color);
	}
	gui_draw_tex(gui, texture, src, dst);
	gui_increment(gui, area);
}

void gui_label(GuiContext* gui, int flags, int2 align, const char* label, size_t len)
{
	float pad = (flags & Gui_Highlighted) ? gui->padding : 0;

	wbsf_Info info;
	if(flags & Gui_Button_Mono) {
		sizeTextMono(label, gui->column > 0 ? -1 : gui->column - pad * 2, &info);
	} else {
		sizeText(label, gui->column > 0 ? -1 : gui->column - pad * 2, &info);
	}


	float2 pos = pad; 
	float2 size = info.bbox.zw + pad * 2;
	if(gui->column > 0) {
		pos.x = align_in(gui->column, size.x, align.x, pad);
		size.x = gui->column;
	}
	if(gui->row > 0) {
		pos.y = align_in(gui->row, size.y, align.y, pad);
		size.y = gui->row;
	}

	if(flags & Gui_Button_Mono) {
		drawText3mono(label, gui->pen + pos, len, -1, -1, gui->camera, &info);
	} else {
		drawText3(label, gui->pen + pos, len, -1, -1, gui->camera, &info);
	}
	gui_increment(gui, size);
}

void gui_label_fmt(GuiContext* gui, int flags, int2 align, const char* fmt, ...)
{
	va_list args;
	va_start(args);
	size_t len = SDL_vsnprintf(gui->fmtBuffer, gui->fmtBufferSize, fmt, args);
	gui->fmtBuffer[gui->fmtBufferSize-1] = 0;
	gui_label(gui, flags, align, gui->fmtBuffer, len);
}

bool gui_checkbox(GuiContext* gui, int flags, bool* check, const char* label)
{
	float2 textsize = sizeTextSimple(label);
	textsize.x += 16 + gui->padding;

	float2 pos;
	float4 region;
	bool ret = gui_button_base(
		gui, 
		flags | Gui_Button_NoElevation | Gui_Button_Outline, 
		AlignCenterLeft, 
		textsize, 
		&pos, &region);


	int color = Color_Dark;
	if(rect_contains(region, gui->mpos)) {
		color = Color_Mid;
		if(gui_mbtn_down(gui)) {
			color = Color_Accent;
		}
	}
	float4 dst;
	dst.xy = pos;
	dst.zw = 16;
	gui_draw_box(gui, dst, color);
	pos.x += 16 + gui->padding;

	if(ret) {
		*check = !*check;
	}

	if(*check) {
		dst.xy += 4;
		dst.zw -= 8;
		gui_draw_box(gui, dst, Color_Accent);
	}

	drawText(label, pos, -1, gui->camera);
	return ret;
}


void gui_textbox(GuiContext* gui, int flags, float width, GuiTextbox* tb)
{
	float height = gui->row > 0 ? gui->row : GLYPH_H + gui->padding * 2;
	width = gui->column > 0 ? gui->column : width + gui->padding * 2;

	float4 region;
	region.xy = gui->pen;
	region.z = width;
	region.w = height;

	int color = Color_Dim;
	if(rect_contains(region, gui->mpos)) {
		color = Color_Mid;
		if(gui_mbtn_justUp(gui)) {
			if(!gui->focusedTextbox) {
				SDL_StartTextInput(Game->window);
			}
			gui->focusedTextbox = tb;
		}
	} else {
		if(gui_mbtn_justUp(gui) && gui->focusedTextbox == tb) {
			gui->focusedTextbox = nullptr;
			SDL_StopTextInput(Game->window);
		}
	}

	if(gui->focusedTextbox == tb) {
		color = Color_Accent;
	}

	gui_draw_outline(gui, region, 1, color);


	wbsf_Info info;
	float4 textout = drawText3(
		tb->buffer, 
		region.xy + gui->padding, 
		tb->numChars, 
		width - gui->padding * 2 - 1, 
		1, 
		gui->camera,
		&info);

	if(info.numPrinted < tb->numChars) {
		tb->numChars = info.numPrinted;
	}

	if(gui->focusedTextbox == tb) {
		textout.z = 2;
		textout.w = 16;
		textout.xy += region.xy + gui->padding;
		gui_draw_box(gui, textout, Color_White);
	}

	gui_increment(gui, region.zw);
}

bool gui_slider(GuiContext* gui, GuiSlider* slider, float width, float* value)
{
	//float height = gui->row > 0 ? gui->row : GLYPH_H + gui->padding * 2;
	if(width == -1) {
		width = 256;
	}
	width = gui->column > 0 ? gui->column : width + gui->padding * 2;

	const char* fmt = slider->fmt;
	if(!fmt) fmt = "%5.1f";
	char buf[64];
	size_t len;
	if(fmt[strlen(fmt)-1] == 'd') 
		len = snprintf(buf, 64, fmt, (int)*value);
	else 
		len = snprintf(buf, 64, fmt, *value);

	float2 textpos = gui->pen + gui->padding;
	float4 bgbox;
	bgbox.xy = textpos - gui->padding;
	bgbox.z = GLYPH_W * len + gui->padding * 2;
	bgbox.w = GLYPH_H + gui->padding * 2; 
	gui_draw_box(gui, bgbox, Color_Dark);

	float4 textRegion = drawText2mono(buf, textpos, len, -1, gui->camera);
	float4 line = 0;
	line.y = gui->padding;
	line.y += GLYPH_H / 2.0f;
	line.x += textRegion.z + gui->padding * 5;
	line.w = 1;
	line.z = width - line.x - gui->padding;
	line.xy += gui->pen;

	if(slider->flags & Slider_ShowButtons) {
		float2 backupPen = gui->pen;
		line.z -= GLYPH_W * 2 + gui->padding * 7;
		gui->pen.x = line.x + line.z + gui->padding * 3;
		int bflags = Gui_Button_NoIncrement | Gui_Button_NoElevation | Gui_Button_Mono;
		if(gui_button(gui, bflags, "-")) {
			*value -= slider->inc.x;
		}
		gui->pen.x += GLYPH_W + gui->padding * 3;
		if (gui_button(gui, bflags, "+")) {
			*value += slider->inc.y;
		}
		gui->pen = backupPen;
	}

	float2 area = line.zw;
	area.x += textRegion.z + gui->padding;
	area.y = GLYPH_H;
	area += gui->padding * 2;
	gui_increment(gui, area);


	if(slider->range.x == 0 && slider->range.y == 0) {
		slider->range.y = 100.0f;
	}
	*value = f32clamp(*value, slider->range.x, slider->range.y);

	float4 box = line;
	box.zw = gui->padding * 4;
	box.xy -= gui->padding * 2;
	box.x += (*value / (slider->range.y - slider->range.x)) * line.z;

	uint32_t color = Color_Mid;
	uint32_t lineColor = Color_Mid;
	// TODO limit to ends, use position while not grabbing, use single floats...

	if(rect_contains(box, gui->mpos)) {
		color = Color_White;
		lineColor = Color_White;
		if(gui_mbtn_down(gui)) {
			color = Color_Accent;
		}

		if(gui_mbtn_justDown(gui)) {
			slider->orig = box.xy;
			slider->grab = gui->mpos;
			slider->state = 1;
		}
	}


	if(gui_mbtn_up(gui)) {
		slider->state = 0;
	}

	bool ret = false;
	// TODO clean up by predefining the end x/y for the box pos
	if(slider->state == 1) {
		box.x = gui->mpos.x - slider->grab.x + slider->orig.x;
		if(box.x < line.x - gui->padding * 2) {
			box.x = line.x - gui->padding * 2;
		} else if(box.x >= line.x + line.z - gui->padding * 2) {
			box.x = line.x + line.z - gui->padding * 2;
		}
		color = Color_Accent;
		lineColor = Color_White;

		float newvalue = ((box.x - line.x + gui->padding * 2) / line.z) * (slider->range.y - slider->range.x);
		if(newvalue != *value) {
			*value = newvalue;
			ret = true;
		}
	}

	gui_draw_box(gui, line, lineColor);
	gui_draw_box(gui, box, color);

	return ret;
}





