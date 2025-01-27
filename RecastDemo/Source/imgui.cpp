//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "imgui.h"
#include "NavProfiles.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const unsigned TEXT_POOL_SIZE = 50000;
static char g_textPool[TEXT_POOL_SIZE];
static unsigned g_textPoolSize = 0;
static const char* allocText(const char* text)
{
	unsigned len = static_cast<unsigned>(strlen(text)+1);
	if (g_textPoolSize + len >= TEXT_POOL_SIZE)
		return 0;
	char* dst = &g_textPool[g_textPoolSize]; 
	memcpy(dst, text, len);
	g_textPoolSize += len;
	return dst;
}

static const unsigned GFXCMD_QUEUE_SIZE = 5000;
static imguiGfxCmd g_gfxCmdQueue[GFXCMD_QUEUE_SIZE];
static unsigned g_gfxCmdQueueSize = 0;

static void resetGfxCmdQueue()
{
	g_gfxCmdQueueSize = 0;
	g_textPoolSize = 0;
}

static void addGfxCmdScissor(int x, int y, int w, int h)
{
	if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
		return;
	imguiGfxCmd& cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
	cmd.type = IMGUI_GFXCMD_SCISSOR;
	cmd.flags = x < 0 ? 0 : 1;	// on/off flag.
	cmd.col = 0;
	cmd.rect.x = (short)x;
	cmd.rect.y = (short)y;
	cmd.rect.w = (short)w;
	cmd.rect.h = (short)h;
}

static void addGfxCmdRect(float x, float y, float w, float h, unsigned int color)
{
	if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
		return;
	imguiGfxCmd& cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
	cmd.type = IMGUI_GFXCMD_RECT;
	cmd.flags = 0;
	cmd.col = color;
	cmd.rect.x = (short)(x*8.0f);
	cmd.rect.y = (short)(y*8.0f);
	cmd.rect.w = (short)(w*8.0f);
	cmd.rect.h = (short)(h*8.0f);
	cmd.rect.r = 0;
}

static void addGfxCmdLine(float x0, float y0, float x1, float y1, float r, unsigned int color)
{
	if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
		return;
	imguiGfxCmd& cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
	cmd.type = IMGUI_GFXCMD_LINE;
	cmd.flags = 0;
	cmd.col = color;
	cmd.line.x0 = (short)(x0*8.0f);
	cmd.line.y0 = (short)(y0*8.0f);
	cmd.line.x1 = (short)(x1*8.0f);
	cmd.line.y1 = (short)(y1*8.0f);
	cmd.line.r = (short)(r*8.0f);
}

static void addGfxCmdRoundedRect(float x, float y, float w, float h, float r, unsigned int color)
{
	if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
		return;
	imguiGfxCmd& cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
	cmd.type = IMGUI_GFXCMD_RECT;
	cmd.flags = 0;
	cmd.col = color;
	cmd.rect.x = (short)(x*8.0f);
	cmd.rect.y = (short)(y*8.0f);
	cmd.rect.w = (short)(w*8.0f);
	cmd.rect.h = (short)(h*8.0f);
	cmd.rect.r = (short)(r*8.0f);
}

static void addGfxCmdTriangle(int x, int y, int w, int h, int flags, unsigned int color)
{
	if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
		return;
	imguiGfxCmd& cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
	cmd.type = IMGUI_GFXCMD_TRIANGLE;
	cmd.flags = (char)flags;
	cmd.col = color;
	cmd.rect.x = (short)(x*8.0f);
	cmd.rect.y = (short)(y*8.0f);
	cmd.rect.w = (short)(w*8.0f);
	cmd.rect.h = (short)(h*8.0f);
}

static void addGfxCmdText(int x, int y, int align, const char* text, unsigned int color)
{
	if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
		return;
	imguiGfxCmd& cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
	cmd.type = IMGUI_GFXCMD_TEXT;
	cmd.flags = 0;
	cmd.col = color;
	cmd.text.x = (short)x;
	cmd.text.y = (short)y;
	cmd.text.align = (short)align;
	cmd.text.text = allocText(text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GuiState
{
	GuiState() :
		left(false), leftPressed(false), leftReleased(false),
		mx(-1), my(-1), scroll(0),
		active(0), hot(0), hotToBe(0), isHot(false), isActive(false), wentActive(false),
		dragX(0), dragY(0), dragOrig(0), widgetX(0), widgetY(0), widgetW(100),
		insideCurrentScroll(false),  areaId(0), widgetId(0)
	{
	}

	bool left;
	bool leftPressed, leftReleased;
	int mx,my;
	int scroll;
	unsigned int active;
	unsigned int hot;
	unsigned int hotToBe;
	bool isHot;
	bool isActive;
	bool wentActive;
	int dragX, dragY;
	float dragOrig;
	int widgetX, widgetY, widgetW;
	bool insideCurrentScroll;
	
	unsigned int areaId;
	unsigned int widgetId;
};

static GuiState g_state;

inline bool anyActive()
{
	return g_state.active != 0;
}

inline bool isActive(unsigned int id)
{
	return g_state.active == id;
}

inline bool isHot(unsigned int id)
{
	return g_state.hot == id;
}

inline bool inRect(int x, int y, int w, int h, bool checkScroll = true)
{
   return (!checkScroll || g_state.insideCurrentScroll) && g_state.mx >= x && g_state.mx <= x+w && g_state.my >= y && g_state.my <= y+h;
}

inline void clearInput()
{
	g_state.leftPressed = false;
	g_state.leftReleased = false;
	g_state.scroll = 0;
}

inline void clearActive()
{
	g_state.active = 0;
	// mark all UI for this frame as processed
	clearInput();
}

inline void setActive(unsigned int id)
{
	g_state.active = id;
	g_state.wentActive = true;
}

inline void setHot(unsigned int id)
{
   g_state.hotToBe = id;
}


static bool buttonLogic(unsigned int id, bool over)
{
	bool res = false;
	// process down
	if (!anyActive())
	{
		if (over)
			setHot(id);
		if (isHot(id) && g_state.leftPressed)
			setActive(id);
	}

	// if button is active, then react on left up
	if (isActive(id))
	{
		g_state.isActive = true;
		if (over)
			setHot(id);
		if (g_state.leftReleased)
		{
			if (isHot(id))
				res = true;
			clearActive();
		}
	}

	if (isHot(id))
		g_state.isHot = true;

	return res;
}

static void updateInput(int mx, int my, unsigned char mbut, int scroll)
{
	bool left = (mbut & IMGUI_MBUT_LEFT) != 0;

	g_state.mx = mx;
	g_state.my = my;
	g_state.leftPressed = !g_state.left && left;
	g_state.leftReleased = g_state.left && !left;
	g_state.left = left;

	g_state.scroll = scroll;
}

void imguiBeginFrame(int mx, int my, unsigned char mbut, int scroll)
{
	updateInput(mx,my,mbut,scroll);

	g_state.hot = g_state.hotToBe;
	g_state.hotToBe = 0;

	g_state.wentActive = false;
	g_state.isActive = false;
	g_state.isHot = false;

	g_state.widgetX = 0;
	g_state.widgetY = 0;
	g_state.widgetW = 0;

	g_state.areaId = 1;
	g_state.widgetId = 1;

	resetGfxCmdQueue();
}

void imguiEndFrame()
{
	clearInput();
}

const imguiGfxCmd* imguiGetRenderQueue()
{
	return g_gfxCmdQueue;
}

int imguiGetRenderQueueSize()
{
	return g_gfxCmdQueueSize;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const int BUTTON_HEIGHT = 20;
static const int SLIDER_HEIGHT = 20;
static const int SLIDER_MARKER_WIDTH = 10;
static const int CHECK_SIZE = 8;
static const int DEFAULT_SPACING = 4;
static const int TEXT_HEIGHT = 8;
static const int SCROLL_AREA_PADDING = 6;
static const int INDENT_SIZE = 16;
static const int AREA_HEADER = 28;
static const int ROW_HEIGHT = 60;

static int g_scrollTop = 0;
static int g_scrollBottom = 0;
static int g_scrollRight = 0;
static int g_scrollAreaTop = 0;
static int* g_scrollVal = 0;
static int g_focusTop = 0;
static int g_focusBottom = 0;
static unsigned int g_scrollId = 0;
static bool g_insideScrollArea = false;

bool imguiBeginScrollArea(const char* name, int x, int y, int w, int h, int* scroll, bool bRounded)
{
	g_state.areaId++;
	g_state.widgetId = 0;
	g_scrollId = (g_state.areaId<<16) | g_state.widgetId;

	g_state.widgetX = x + SCROLL_AREA_PADDING;
	g_state.widgetY = y+h-AREA_HEADER + (*scroll);
	g_state.widgetW = w - SCROLL_AREA_PADDING*4;
	g_scrollTop = y-AREA_HEADER+h;
	g_scrollBottom = y+SCROLL_AREA_PADDING;
	g_scrollRight = x+w - SCROLL_AREA_PADDING*3;
	g_scrollVal = scroll;

	g_scrollAreaTop = g_state.widgetY;

	g_focusTop = y-AREA_HEADER;
	g_focusBottom = y-AREA_HEADER+h;

	g_insideScrollArea = inRect(x, y, w, h, false);
	g_state.insideCurrentScroll = g_insideScrollArea;

	if (bRounded)
	{
		addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, 6, imguiRGBA(0, 0, 0, 192));
	}
	else
	{
		addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(0, 0, 0, 192));
	}

	addGfxCmdText(x+AREA_HEADER/2, y+h-AREA_HEADER/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, name, imguiRGBA(255,255,255,128));

	addGfxCmdScissor(x+SCROLL_AREA_PADDING, y+SCROLL_AREA_PADDING, w-SCROLL_AREA_PADDING*4, h-AREA_HEADER-SCROLL_AREA_PADDING);

	return g_insideScrollArea;
}

void imguiEndScrollArea()
{
	// Disable scissoring.
	addGfxCmdScissor(-1,-1,-1,-1);

	// Draw scroll bar
	int x = g_scrollRight+SCROLL_AREA_PADDING/2;
	int y = g_scrollBottom;
	int w = SCROLL_AREA_PADDING*2;
	int h = g_scrollTop - g_scrollBottom;

	int stop = g_scrollAreaTop;
	int sbot = g_state.widgetY;
	int sh = stop - sbot; // The scrollable area height.

	float barHeight = (float)h/(float)sh;
	
	if (barHeight < 1)
	{
		float barY = (float)(y - sbot)/(float)sh;
		if (barY < 0) barY = 0;
		if (barY > 1) barY = 1;
		
		// Handle scroll bar logic.
		unsigned int hid = g_scrollId;
		int hx = x;
		int hy = y + (int)(barY*h);
		int hw = w;
		int hh = (int)(barHeight*h);
		
		const int range = h - (hh-1);
		bool over = inRect(hx, hy, hw, hh);
		buttonLogic(hid, over);
		if (isActive(hid))
		{
			float u = (float)(hy-y) / (float)range;
			if (g_state.wentActive)
			{
				g_state.dragY = g_state.my;
				g_state.dragOrig = u;
			}
			if (g_state.dragY != g_state.my)
			{
				u = g_state.dragOrig + (g_state.my - g_state.dragY) / (float)range;
				if (u < 0) u = 0;
				if (u > 1) u = 1;
				*g_scrollVal = (int)((1-u) * (sh - h));
			}
		}
		
		// BG
		addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, (float)w/2-1, imguiRGBA(0,0,0,196));
		// Bar
		if (isActive(hid))
			addGfxCmdRoundedRect((float)hx, (float)hy, (float)hw, (float)hh, (float)w/2-1, imguiRGBA(255,196,0,196));
		else
			addGfxCmdRoundedRect((float)hx, (float)hy, (float)hw, (float)hh, (float)w/2-1, isHot(hid) ? imguiRGBA(255,196,0,96) : imguiRGBA(255,255,255,64));

		// Handle mouse scrolling.
		if (g_insideScrollArea) // && !anyActive())
		{
			if (g_state.scroll)
			{
				*g_scrollVal += 20*g_state.scroll;
				if (*g_scrollVal < 0) *g_scrollVal = 0;
				if (*g_scrollVal > (sh - h)) *g_scrollVal = (sh - h);
			}
		}
	}
	g_state.insideCurrentScroll = false;
}

bool imguiButton(const char* text, bool enabled, bool invisible)
{
	g_state.widgetId++;
	unsigned int id = (g_state.areaId<<16) | g_state.widgetId;
	
	int x = g_state.widgetX;
	int y = g_state.widgetY - BUTTON_HEIGHT;
	int w = g_state.widgetW;
	int h = BUTTON_HEIGHT;
	g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

	bool over = enabled && inRect(x, y, w, h);
	bool res = buttonLogic(id, over);

	if (!invisible)
	{
		addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, (float)BUTTON_HEIGHT / 2 - 1, imguiRGBA(128, 128, 128, isActive(id) ? 196 : 96));
		if (enabled)
			addGfxCmdText(x + BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT, text, isHot(id) ? imguiRGBA(255, 196, 0, 255) : imguiRGBA(255, 255, 255, 200));
		else
			addGfxCmdText(x + BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT, text, imguiRGBA(128, 128, 128, 200));
	}
	else
	{
		addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, (float)BUTTON_HEIGHT / 2 - 1, imguiRGBA(0,0,0,0));
	}

	return res;
}

bool imguiItem(const char* text, bool enabled)
{
	g_state.widgetId++;
	unsigned int id = (g_state.areaId<<16) | g_state.widgetId;
	
	int x = g_state.widgetX;
	int y = g_state.widgetY - BUTTON_HEIGHT;
	int w = g_state.widgetW;
	int h = BUTTON_HEIGHT;
	g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
	
	bool over = enabled && inRect(x, y, w, h);
	bool res = buttonLogic(id, over);
	
	if (isHot(id))
		addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, 2.0f, imguiRGBA(255,196,0,isActive(id)?196:96));

	if (enabled)
		addGfxCmdText(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(255,255,255,200));
	else
		addGfxCmdText(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(128,128,128,200));
	
	return res;
}

bool imguiCheck(const char* text, bool checked, bool enabled)
{
	g_state.widgetId++;
	unsigned int id = (g_state.areaId<<16) | g_state.widgetId;
	
	int x = g_state.widgetX;
	int y = g_state.widgetY - BUTTON_HEIGHT;
	int w = g_state.widgetW;
	int h = BUTTON_HEIGHT;
	g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

	bool over = enabled && inRect(x, y, w, h);
	bool res = buttonLogic(id, over);
	
	const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
	const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;
	addGfxCmdRoundedRect((float)cx-3, (float)cy-3, (float)CHECK_SIZE+6, (float)CHECK_SIZE+6, 4, imguiRGBA(128,128,128, isActive(id)?196:96));
	if (checked)
	{
		if (enabled)
			addGfxCmdRoundedRect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, imguiRGBA(255,255,255,isActive(id)?255:200));
		else
			addGfxCmdRoundedRect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, imguiRGBA(128,128,128,200));
	}

	if (enabled)
		addGfxCmdText(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, isHot(id) ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200));
	else
		addGfxCmdText(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(128,128,128,200));

	return res;
}

bool imguiCollapse(const char* text, const char* subtext, bool checked, bool enabled)
{
	g_state.widgetId++;
	unsigned int id = (g_state.areaId<<16) | g_state.widgetId;
	
	int x = g_state.widgetX;
	int y = g_state.widgetY - BUTTON_HEIGHT;
	int w = g_state.widgetW;
	int h = BUTTON_HEIGHT;
	g_state.widgetY -= BUTTON_HEIGHT; // + DEFAULT_SPACING;

	const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
	const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;

	bool over = enabled && inRect(x, y, w, h);
	bool res = buttonLogic(id, over);
	
	if (checked)
		addGfxCmdTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 2, imguiRGBA(255,255,255,isActive(id)?255:200));
	else
		addGfxCmdTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 1, imguiRGBA(255,255,255,isActive(id)?255:200));

	if (enabled)
		addGfxCmdText(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, isHot(id) ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200));
	else
		addGfxCmdText(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(128,128,128,200));

	if (subtext)
		addGfxCmdText(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, subtext, imguiRGBA(255,255,255,128));
	
	return res;
}

void imguiLabel(const char* text)
{
	int x = g_state.widgetX;
	int y = g_state.widgetY - BUTTON_HEIGHT;
	g_state.widgetY -= BUTTON_HEIGHT;
	addGfxCmdText(x, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(255,255,255,255));
}

void imguiValue(const char* text)
{
	const int x = g_state.widgetX;
	const int y = g_state.widgetY - BUTTON_HEIGHT;
	const int w = g_state.widgetW;
	g_state.widgetY -= BUTTON_HEIGHT;
	
	addGfxCmdText(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, text, imguiRGBA(255,255,255,200));
}

bool imguiSlider(const char* text, float* val, float vmin, float vmax, float vinc, bool enabled)
{
	g_state.widgetId++;
	unsigned int id = (g_state.areaId<<16) | g_state.widgetId;
	
	int x = g_state.widgetX;
	int y = g_state.widgetY - BUTTON_HEIGHT;
	int w = g_state.widgetW;
	int h = SLIDER_HEIGHT;
	g_state.widgetY -= SLIDER_HEIGHT + DEFAULT_SPACING;

	addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, 4.0f, imguiRGBA(0,0,0,128));

	const int range = w - SLIDER_MARKER_WIDTH;

	float u = (*val - vmin) / (vmax-vmin);
	if (u < 0) u = 0;
	if (u > 1) u = 1;
	int m = (int)(u * range);

	bool over = enabled && inRect(x+m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT);
	bool res = buttonLogic(id, over);
	bool valChanged = false;

	if (isActive(id))
	{
		if (g_state.wentActive)
		{
			g_state.dragX = g_state.mx;
			g_state.dragOrig = u;
		}
		if (g_state.dragX != g_state.mx)
		{
			u = g_state.dragOrig + (float)(g_state.mx - g_state.dragX) / (float)range;
			if (u < 0) u = 0;
			if (u > 1) u = 1;
			*val = vmin + u*(vmax-vmin);
			*val = floorf(*val/vinc+0.5f)*vinc; // Snap to vinc
			m = (int)(u * range);
			valChanged = true;
		}
	}

	if (isActive(id))
		addGfxCmdRoundedRect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, imguiRGBA(255,255,255,255));
	else
		addGfxCmdRoundedRect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, isHot(id) ? imguiRGBA(255,196,0,128) : imguiRGBA(255,255,255,64));

	// TODO: fix this, take a look at 'nicenum'.
	int digits = (int)(ceilf(log10f(vinc)));
	char fmt[16];
	snprintf(fmt, 16, "%%.%df", digits >= 0 ? 0 : -digits);
	char msg[128];
	snprintf(msg, 128, fmt, *val);
	
	if (enabled)
	{
		addGfxCmdText(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, isHot(id) ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200));
		addGfxCmdText(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, msg, isHot(id) ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200));
	}
	else
	{
		addGfxCmdText(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(128,128,128,200));
		addGfxCmdText(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, msg, imguiRGBA(128,128,128,200));
	}

	return res || valChanged;
}


void imguiIndent()
{
	g_state.widgetX += INDENT_SIZE;
	g_state.widgetW -= INDENT_SIZE;
}

void imguiUnindent()
{
	g_state.widgetX -= INDENT_SIZE;
	g_state.widgetW += INDENT_SIZE;
}

void imguiSeparator()
{
	g_state.widgetY -= DEFAULT_SPACING*3;
}

void imguiSeparatorLine()
{
	int x = g_state.widgetX;
	int y = g_state.widgetY - DEFAULT_SPACING*2;
	int w = g_state.widgetW;
	int h = 1;
	g_state.widgetY -= DEFAULT_SPACING*4;

	addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(255,255,255,32));
}

void imguiDrawText(int x, int y, int align, const char* text, unsigned int color)
{
	addGfxCmdText(x, y, align, text, color);
}

void imguiDrawLine(float x0, float y0, float x1, float y1, float r, unsigned int color)
{
	addGfxCmdLine(x0, y0, x1, y1, r, color);
}

void imguiDrawRect(float x, float y, float w, float h, unsigned int color)
{
	addGfxCmdRect(x, y, w, h, color);
}

void imguiDrawRoundedRect(float x, float y, float w, float h, float r, unsigned int color)
{
	addGfxCmdRoundedRect(x, y, w, h, r, color);
}


inline int bit2(int a, int b)
{
	return (a & (1 << b)) >> b;
}

inline unsigned int duRGBA2(int r, int g, int b, int a)
{
	return ((unsigned int)r) | ((unsigned int)g << 8) | ((unsigned int)b << 16) | ((unsigned int)a << 24);
}

unsigned int duIntToCol2(int i, int a)
{
	int	r = bit2(i, 1) + bit2(i, 3) * 2 + 1;
	int	g = bit2(i, 2) + bit2(i, 4) * 2 + 1;
	int	b = bit2(i, 0) + bit2(i, 5) * 2 + 1;
	return duRGBA2(r * 63, g * 63, b * 63, a);
}

bool addNavProfileEntryRow(const int ProfileIndex)
{
	NavAgentProfile* ProfileDef = GetAgentProfileAtIndex(ProfileIndex);

	if (!ProfileDef) { return false; }

	int CurrentProfileSelected = GetProfileModifier();

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;

	int endWidgetW = g_state.widgetW;
	int startWidgetY = g_state.widgetY;
	int endWidgetX = g_state.widgetX;

	int TotalWidth = w - 10;

	int NameWidth = (int)(ceilf((float)TotalWidth * 0.9f));
	int DeleteWidth = (int)(ceilf((float)TotalWidth * 0.1f));

	g_state.widgetW = NameWidth;

	if (imguiCheck(ProfileDef->ProfileName.c_str(), CurrentProfileSelected == ProfileIndex, true))
	{
		ResetProfileMenus();
		SetProfileModifier(ProfileIndex);
	}

	int endWidgetY = g_state.widgetY;

	g_state.widgetY = startWidgetY;
	g_state.widgetX += NameWidth;

	g_state.widgetW = 25;


	if (imguiButton("-", true))
	{
		RemoveAgentProfile(ProfileIndex);
		return true;
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;

	return false;

}

bool showNavProfileDetails(const int ProfileIndex)
{
	NavAgentProfile* ProfileDef = GetAgentProfileAtIndex(ProfileIndex);

	if (!ProfileDef) { return false; }

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;

	imguiLabel("Profile Name");

	char RowEntry[128];

	sprintf(RowEntry, "%s", ProfileDef->ProfileName.c_str());

	if (imguiButton(RowEntry, true))
	{
		ResetProfileMenus();
		SetCurrentlyModifiedString(&ProfileDef->ProfileName);

		return true;
	}

	imguiLabel("Profile Technical Name");

	sprintf(RowEntry, "%s", ProfileDef->TechnicalName.c_str());

	if (imguiButton(RowEntry, true))
	{
		ResetProfileMenus();
		SetCurrentlyModifiedString(&ProfileDef->TechnicalName);

		return true;
	}

	imguiSeparator();

	imguiLabel("Nav Mesh Used");

	NavMeshDefinition* AssociatedNavMesh = GetMeshAtIndex(ProfileDef->NavMeshIndex);

	if (imguiButton(AssociatedNavMesh->NavMeshName.c_str(), true))
	{
		if (GetProfileMeshModifier() > -1)
		{
			SetProfileMeshModifier(-1);
		}
		else
		{
			SetProfileMeshModifier(ProfileIndex);
		}

		return true;
	}

	int endWidgetX = g_state.widgetX;
	int endWidgetY = g_state.widgetY;
	int endWidgetW = g_state.widgetW;

	if (GetProfileMeshModifier() == ProfileIndex)
	{
		g_state.widgetY += 20;

		static int areaScroll = 0;

		imguiBeginScrollArea("Choose Nav Mesh", endWidgetX, endWidgetY + 20, 200, 200, &areaScroll);

		vector<NavMeshDefinition> AllMeshes = GetAllMeshDefinitions();

		int MeshIndex = 0;

		for (auto it = AllMeshes.begin(); it != AllMeshes.end(); it++)
		{
			if (imguiItem(it->NavMeshName.c_str()))
			{
				ProfileDef->NavMeshIndex = MeshIndex;
				SetProfileMeshModifier(-1);
			}

			MeshIndex++;
		}

		imguiEndScrollArea();
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;

	imguiSeparator();

	int CapabilityStartX = g_state.widgetX;
	int CapabilityStartY = g_state.widgetY;

	imguiLabel("Movement Capabilities");

	g_state.widgetW = (g_state.widgetW / 2) - 5;

	vector<NavFlagDefinition> AllFlags = GetAllNavFlagDefinitions();

	for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
	{
		if (it->NavFlagIndex == 0) { continue; }

		sprintf(RowEntry, "%s", it->FlagName.c_str());

		if (imguiCheck(RowEntry, ProfileDef->MovementFlags & it->FlagId, true))
		{
			ProfileDef->MovementFlags ^= it->FlagId;
		}
	}

	g_state.widgetX = CapabilityStartX + g_state.widgetW + 5;
	g_state.widgetY = CapabilityStartY;

	imguiLabel("Area Costs");

	vector<NavAreaDefinition> AllAreas = GetAllNavAreaDefinitions();

	for (auto it = AllAreas.begin(); it != AllAreas.end(); it++)
	{
		if (it->NavAreaIndex == 0) { continue; }

		sprintf(RowEntry, "%s", it->AreaName.c_str());

		imguiSlider(RowEntry, &ProfileDef->AreaCosts[it->NavAreaIndex], 0.1f, 20.0f, 0.1f, true);
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;

	return false;

}

bool addNavHintRow(const int HintIndex)
{
	NavHintDefinition* HintDef = GetNavHintAtIndex(HintIndex);

	if (!HintDef) { return false; }

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;
	const int h = ROW_HEIGHT;

	addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(0, 0, 0, 192));

	int TotalWidth = w - 10;

	int IDWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int NameWidth = (int)(ceilf((float)TotalWidth * 0.4f));
	int TechNameWidth = (int)(ceilf((float)TotalWidth * 0.4f));
	int DeleteWidth = (int)(ceilf((float)TotalWidth * 0.1f));

	int IDStartX = x + 5;
	int NameStartX = IDStartX + IDWidth;
	int TechNameStartX = NameStartX + NameWidth;
	int DeleteStartX = TechNameStartX + TechNameWidth;

	int HeaderStartY = y + (h - (TEXT_HEIGHT + 10));
	int RowStartY = y + 20;

	addGfxCmdText(IDStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "ID", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(NameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Descriptive Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(TechNameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Technical Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(DeleteStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Delete", imguiRGBA(255, 255, 255, 200));

	char RowEntry[128];

	sprintf(RowEntry, "%d", HintIndex);
	addGfxCmdText(IDStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));

	int endWidgetX = g_state.widgetX;
	int endWidgetY = g_state.widgetY;
	int endWidgetW = g_state.widgetW;

	g_state.widgetX = NameStartX;
	g_state.widgetW = TechNameStartX - NameStartX - 20;
	g_state.widgetY = RowStartY + 10;

	sprintf(RowEntry, "%s", HintDef->HintName.c_str());

	if (imguiButton(RowEntry, true))
	{
		ResetProfileMenus();
		SetCurrentlyModifiedString(&HintDef->HintName);

		return true;
	}

	g_state.widgetX = TechNameStartX;
	g_state.widgetW = DeleteStartX - TechNameStartX - 20;
	g_state.widgetY = RowStartY + 10;

	sprintf(RowEntry, "%s", HintDef->TechnicalName.c_str());

	if (imguiButton(RowEntry, true))
	{
		ResetProfileMenus();
		SetCurrentlyModifiedString(&HintDef->TechnicalName);

		return true;
	}

	g_state.widgetX = DeleteStartX;
	g_state.widgetW = 100;
	g_state.widgetY = RowStartY + 10;

	if (imguiButton("Delete", true))
	{
		ResetProfileMenus();
		RemoveHintType(HintIndex);
		return true;
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;
	g_state.widgetY -= ROW_HEIGHT;

	return false;
}

bool addNavMeshRow(const int MeshIndex)
{
	NavMeshDefinition* MeshDef = GetMeshAtIndex(MeshIndex);

	if (!MeshDef) { return false; }

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;
	const int h = ROW_HEIGHT;

	addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(0, 0, 0, 192));

	int TotalWidth = w - 10;

	int IDWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int NameWidth = (int)(ceilf((float)TotalWidth * 0.2f));
	int TechnicalNameWidth = (int)(ceilf((float)TotalWidth * 0.15f));
	int RadiusWidth = (int)(ceilf((float)TotalWidth * 0.15f));
	int StandHeightWidth = (int)(ceilf((float)TotalWidth * 0.15f));
	int CrouchHeightWidth = (int)(ceilf((float)TotalWidth * 0.15f));
	int DeleteWidth = (int)(ceilf((float)TotalWidth * 0.1f));

	int IDStartX = x + 5;
	int NameStartX = IDStartX + IDWidth;
	int TechNameStartX = NameStartX + TechnicalNameWidth;
	int RadiusStartX = TechNameStartX + NameWidth;
	int StandHeightStartX = RadiusStartX + RadiusWidth;
	int CrouchHeightStartX = StandHeightStartX + StandHeightWidth;
	int DeleteStartX = CrouchHeightStartX + CrouchHeightWidth;

	int HeaderStartY = y + (h - (TEXT_HEIGHT + 10));
	int RowStartY = y + 20;

	addGfxCmdText(IDStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "ID", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(NameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Descriptive Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(TechNameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Technical Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(RadiusStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Default Agent Radius", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(StandHeightStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Default Agent Height (Standing)", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(CrouchHeightStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Default Agent Height (Crouched)", imguiRGBA(255, 255, 255, 200));

	char RowEntry[128];

	sprintf(RowEntry, "%d", MeshIndex);
	addGfxCmdText(IDStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));

	int endWidgetX = g_state.widgetX;
	int endWidgetY = g_state.widgetY;
	int endWidgetW = g_state.widgetW;

	g_state.widgetX = NameStartX;
	g_state.widgetW = TechNameStartX - NameStartX - 20;
	g_state.widgetY = RowStartY + 10;

	sprintf(RowEntry, "%s", MeshDef->NavMeshName.c_str());

	if (imguiButton(RowEntry, true))
	{
		ResetProfileMenus();
		SetCurrentlyModifiedString(&MeshDef->NavMeshName);

		return true;
	}

	g_state.widgetX = TechNameStartX;
	g_state.widgetW = RadiusStartX - TechNameStartX - 20;
	g_state.widgetY = RowStartY + 10;

	sprintf(RowEntry, "%s", MeshDef->TechnicalName.c_str());

	if (imguiButton(RowEntry, true))
	{
		ResetProfileMenus();
		SetCurrentlyModifiedString(&MeshDef->TechnicalName);

		return true;
	}

	g_state.widgetX = RadiusStartX;
	g_state.widgetW = StandHeightStartX - RadiusStartX - 20;
	g_state.widgetY = RowStartY + 10;

	imguiSlider("", &MeshDef->AgentRadius, 1.0f, 200.0f, 1.0f, true);

	g_state.widgetX = StandHeightStartX;
	g_state.widgetW = CrouchHeightStartX - StandHeightStartX - 20;
	g_state.widgetY = RowStartY + 10;

	imguiSlider("", &MeshDef->AgentStandingHeight, 1.0f, 200.0f, 1.0f, true);

	g_state.widgetX = CrouchHeightStartX;
	g_state.widgetW = DeleteStartX - CrouchHeightStartX - 20;
	g_state.widgetY = RowStartY + 10;

	imguiSlider("", &MeshDef->AgentCrouchingHeight, 1.0f, 200.0f, 1.0f, true);


	if (GetNumNavMeshes() <= 1)
	{
		addGfxCmdText(DeleteStartX, RowStartY, IMGUI_ALIGN_LEFT, "Min 1 Mesh Required", imguiRGBA(255, 255, 255, 200));
	}
	else
	{
		g_state.widgetX = DeleteStartX;
		g_state.widgetW = 100;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton("Delete", true))
		{
			ResetProfileMenus();
			RemoveMeshDefinition(MeshIndex);
			return true;
		}
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;
	g_state.widgetY -= ROW_HEIGHT;

	return false;
}

bool addConnectionRow(const int ConnectionIndex)
{
	NavOffMeshConnectionDefinition* ConnDef = GetConnectionAtIndex(ConnectionIndex);

	if (!ConnDef) { return false; }

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;
	const int h = ROW_HEIGHT;

	addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(0, 0, 0, 192));

	int TotalWidth = w - 10;

	int IDWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int NameWidth = (int)(ceilf((float)TotalWidth * 0.225f));
	int AreaWidth = (int)(ceilf((float)TotalWidth * 0.225f));
	int FlagWidth = (int)(ceilf((float)TotalWidth * 0.225f));
	int ColourWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int DeleteWidth = (int)(ceilf((float)TotalWidth * 0.1f));

	int IDStartX = x + 5;
	int NameStartX = IDStartX + IDWidth;
	int AreaStartX = NameStartX + NameWidth;
	int FlagStartX = AreaStartX + AreaWidth;
	int ColourStartX = FlagStartX + FlagWidth;
	int DeleteStartX = ColourStartX + ColourWidth;

	int HeaderStartY = y + (h - (TEXT_HEIGHT + 10));
	int RowStartY = y + 20;

	addGfxCmdText(IDStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "ID", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(NameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Descriptive Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(AreaStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Area", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(FlagStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Flag", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(ColourStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Flag Color", imguiRGBA(255, 255, 255, 200));

	char RowEntry[128];

	sprintf(RowEntry, "%d", ConnDef->ConnIndex);
	addGfxCmdText(IDStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));

	sprintf(RowEntry, "%s", ConnDef->ConnName.c_str());

	if (ConnDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = NameStartX;
		g_state.widgetW = AreaStartX - NameStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			ResetProfileMenus();
			SetCurrentlyModifiedString(&ConnDef->ConnName);

			return true;
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(NameStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	NavAreaDefinition* AreaRef = GetAreaAtIndex(ConnDef->AreaIndex);

	if (AreaRef)
	{
		sprintf(RowEntry, "%s", AreaRef->AreaName.c_str());
	}
	else
	{
		sprintf(RowEntry, "Invalid");
	}

	if (ConnDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = AreaStartX;
		g_state.widgetW = FlagStartX - AreaStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			if (GetConnAreaModifier() > -1)
			{
				SetConnAreaModifier(-1);
			}
			else
			{
				SetConnAreaModifier(ConnDef->ConnIndex);
			}

			return true;
		}

		if (GetConnAreaModifier() == ConnDef->ConnIndex)
		{
			g_state.widgetY += 20;

			static int areaScroll = 0;

			imguiBeginScrollArea("Choose Area", AreaStartX, RowStartY + 20, FlagStartX - AreaStartX - 20, 200, &areaScroll);

			vector<NavAreaDefinition> AllFlags = GetAllNavAreaDefinitions();

			for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
			{
				if (imguiItem(it->AreaName.c_str()))
				{
					ConnDef->AreaIndex = it->NavAreaIndex;
					SetConnAreaModifier(-1);
				}
			}

			imguiEndScrollArea();
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(AreaStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	NavFlagDefinition* FlagRef = GetFlagAtIndex(ConnDef->FlagIndex);

	if (FlagRef)
	{
		sprintf(RowEntry, "%s", FlagRef->FlagName.c_str());
	}
	else
	{
		sprintf(RowEntry, "Invalid");
	}

	if (ConnDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = FlagStartX;
		g_state.widgetW = FlagStartX - AreaStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			if (GetConnFlagModifier() > -1)
			{
				SetConnFlagModifier(-1);
			}
			else
			{
				SetConnFlagModifier(ConnDef->ConnIndex);
			}

			return true;
		}

		if (GetConnFlagModifier() == ConnDef->ConnIndex)
		{
			g_state.widgetY += 20;

			static int flagScroll = 0;

			imguiBeginScrollArea("Choose Area", FlagStartX, RowStartY + 20, ColourStartX - FlagStartX - 20, 200, &flagScroll);

			vector<NavFlagDefinition> AllFlags = GetAllNavFlagDefinitions();

			for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
			{
				if (imguiItem(it->FlagName.c_str()))
				{
					ConnDef->FlagIndex = it->NavFlagIndex;
					SetConnFlagModifier(-1);
				}
			}

			imguiEndScrollArea();
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(FlagStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	imguiDrawRect((float)ColourStartX, (float)RowStartY - 10, (float)25, (float)25, (FlagRef) ? FlagRef->DebugColor : duRGBA2(8, 8, 8, 255));

	int endWidgetX = g_state.widgetX;
	int endWidgetY = g_state.widgetY;
	int endWidgetW = g_state.widgetW;

	g_state.widgetX = DeleteStartX;
	g_state.widgetW = 100;
	g_state.widgetY -= 20;

	if (ConnDef->bCustom)
	{
		if (imguiButton("Delete", true))
		{
			ResetProfileMenus();
			RemoveConnectionType(ConnectionIndex);
			return true;
		}
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;

	g_state.widgetY -= ROW_HEIGHT;

	return false;
}

bool addFlagRow(const int FlagIndex)
{
	NavFlagDefinition* FlagDef = GetFlagAtIndex(FlagIndex);

	if (!FlagDef) { return false; }

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;
	const int h = ROW_HEIGHT;

	addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(0, 0, 0, 192));

	int TotalWidth = w - 10;

	int IDWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int NameWidth = (int)(ceilf((float)TotalWidth * 0.3f));
	int TechNameWidth = (int)(ceilf((float)TotalWidth * 0.3f));
	int TeleportWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int ColourWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int DeleteWidth = (int)(ceilf((float)TotalWidth * 0.1f));

	int IDStartX = x + 5;
	int NameStartX = IDStartX + IDWidth;
	int TechNameStartX = NameStartX + NameWidth;
	int TeleportStartX = TechNameStartX + TechNameWidth;
	int ColourStartX = TeleportStartX + TeleportWidth;
	int DeleteStartX = ColourStartX + ColourWidth;

	int HeaderStartY = y + (h - (TEXT_HEIGHT + 10));
	int RowStartY = y + 20;

	addGfxCmdText(IDStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "ID", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(NameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Descriptive Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(TechNameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Technical Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(TeleportStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Teleport Type", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(ColourStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Flag Color", imguiRGBA(255, 255, 255, 200));

	char RowEntry[128];

	sprintf(RowEntry, "%d", FlagDef->NavFlagIndex);
	addGfxCmdText(IDStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));

	sprintf(RowEntry, "%s", FlagDef->FlagName.c_str());

	if (FlagDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = NameStartX;
		g_state.widgetW = TechNameStartX - NameStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			ResetProfileMenus();
			SetCurrentlyModifiedString(&FlagDef->FlagName);

			return true;
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(NameStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	sprintf(RowEntry, "%s", FlagDef->TechnicalName.c_str());

	if (FlagDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = TechNameStartX;
		g_state.widgetW = TeleportStartX - TechNameStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			ResetProfileMenus();
			SetCurrentlyModifiedString(&FlagDef->TechnicalName);

			return true;
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(TechNameStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	int beforeTeleportX = g_state.widgetX;
	int beforeTeleportY = g_state.widgetY;
	int beforeTeleportW = g_state.widgetW;

	g_state.widgetX = TeleportStartX;
	g_state.widgetW = ColourStartX - TeleportStartX - 20;
	g_state.widgetY = RowStartY + 10;

	if (imguiCheck("", FlagDef->bTeleport, FlagDef->bCustom))
	{
		FlagDef->bTeleport = !FlagDef->bTeleport;
	}

	g_state.widgetX = beforeTeleportX;
	g_state.widgetY = beforeTeleportY;
	g_state.widgetW = beforeTeleportW;


	int beforeColourX = g_state.widgetX;
	int beforeColourY = g_state.widgetY;
	int beforeColourW = g_state.widgetW;

	imguiDrawRect((float)ColourStartX, (float)RowStartY - 10, (float)25, (float)25, FlagDef->DebugColor);

	int afterColourX = g_state.widgetX;
	int afterColourY = g_state.widgetY;
	int afterColourW = g_state.widgetW;

	g_state.widgetX = ColourStartX;
	g_state.widgetY = RowStartY + 10;
	g_state.widgetW = DeleteStartX - ColourStartX - 10;

	if (imguiButton("", true, true))
	{
		unsigned int* CurrentFlagColorModifier = GetFlagColorModifier();

		ResetProfileMenus();

		if (CurrentFlagColorModifier != &FlagDef->DebugColor)
		{
			SetFlagColorModifier(&FlagDef->DebugColor);
		}
	}

	g_state.widgetX = afterColourX;
	g_state.widgetY = afterColourY;
	g_state.widgetW = afterColourW;

	if (GetFlagColorModifier() == &FlagDef->DebugColor)
	{
		g_state.widgetY += 20;

		static int areaScroll = 0;

		imguiBeginScrollArea("Choose Color", ColourStartX, RowStartY + 20, DeleteStartX - ColourStartX - 20, 150, &areaScroll);

		if (imguiSlider("R", &FlagDef->R, 0, 255, 1, true))
		{
			FlagDef->DebugColor = duRGBA2((int)FlagDef->R, (int)FlagDef->G, (int)FlagDef->B, 255);
		}

		if (imguiSlider("G", &FlagDef->G, 0, 255, 1, true))
		{
			FlagDef->DebugColor = duRGBA2((int)FlagDef->R, (int)FlagDef->G, (int)FlagDef->B, 255);
		}

		if (imguiSlider("B", &FlagDef->B, 0, 255, 1, true))
		{
			FlagDef->DebugColor = duRGBA2((int)FlagDef->R, (int)FlagDef->G, (int)FlagDef->B, 255);
		}

		if (imguiButton("Save"))
		{
			ResetProfileMenus();
		}

		imguiEndScrollArea();

		g_state.widgetX = afterColourX;
		g_state.widgetY = afterColourY;
		g_state.widgetW = afterColourW;
	}


	int endWidgetX = g_state.widgetX;
	int endWidgetY = g_state.widgetY;
	int endWidgetW = g_state.widgetW;

	g_state.widgetX = DeleteStartX;
	g_state.widgetW = 100;
	g_state.widgetY -= 20;

	if (FlagDef->bCustom)
	{
		if (imguiButton("Delete", true))
		{
			ResetProfileMenus();
			RemoveMovementFlag(FlagIndex);
			return true;
		}
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;

	g_state.widgetY -= ROW_HEIGHT;

	return false;
}

bool addAreaRow(const int AreaIndex)
{
	NavAreaDefinition* AreaDef = GetAreaAtIndex(AreaIndex);

	if (!AreaDef) { return false; }

	const int x = g_state.widgetX;
	const int y = g_state.widgetY - ROW_HEIGHT;
	const int w = g_state.widgetW;
	const int h = ROW_HEIGHT;

	addGfxCmdRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(0, 0, 0, 192));

	int TotalWidth = w - 10;

	int IDWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int NameWidth = (int)(ceilf((float)TotalWidth * 0.225f));
	int TechNameWidth = (int)(ceilf((float)TotalWidth * 0.225f));
	int FlagWidth = (int)(ceilf((float)TotalWidth * 0.225f));
	int ColourWidth = (int)(ceilf((float)TotalWidth * 0.1f));
	int DeleteWidth = (int)(ceilf((float)TotalWidth * 0.1f));

	int IDStartX = x + 5;
	int NameStartX = IDStartX + IDWidth;
	int TechNameStartX = NameStartX + NameWidth;
	int FlagStartX = TechNameStartX + TechNameWidth;
	int ColourStartX = FlagStartX + FlagWidth;
	int DeleteStartX = ColourStartX + ColourWidth;

	int HeaderStartY = y + (h - (TEXT_HEIGHT + 10));
	int RowStartY = y + 20;

	addGfxCmdText(IDStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "ID", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(NameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Descriptive Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(TechNameStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Technical Name", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(FlagStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Flag Used When Traversing", imguiRGBA(255, 255, 255, 200));
	addGfxCmdText(ColourStartX, HeaderStartY, IMGUI_ALIGN_LEFT, "Area Color", imguiRGBA(255, 255, 255, 200));

	char RowEntry[128];

	sprintf(RowEntry, "%d", AreaDef->AreaId);
	addGfxCmdText(IDStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));

	sprintf(RowEntry, "%s", AreaDef->AreaName.c_str());

	if (AreaDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = NameStartX;
		g_state.widgetW = TechNameStartX - NameStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			if (GetCurrentlyModifiedString() != &AreaDef->AreaName)
			{
				ResetProfileMenus();
				SetCurrentlyModifiedString(&AreaDef->AreaName);
			}

			return true;
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(NameStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	sprintf(RowEntry, "%s", AreaDef->TechnicalName.c_str());

	if (AreaDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = TechNameStartX;
		g_state.widgetW = FlagStartX - TechNameStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			ResetProfileMenus();
			SetCurrentlyModifiedString(&AreaDef->TechnicalName);

			return true;
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;

	}
	else
	{
		addGfxCmdText(TechNameStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));

	}

	NavFlagDefinition* FlagRef = GetFlagAtIndex(AreaDef->FlagIndex);

	if (FlagRef)
	{
		sprintf(RowEntry, "%s", FlagRef->FlagName.c_str());
	}
	else
	{
		sprintf(RowEntry, "Invalid");
	}

	if (AreaDef->bCustom)
	{
		int endWidgetX = g_state.widgetX;
		int endWidgetY = g_state.widgetY;
		int endWidgetW = g_state.widgetW;

		g_state.widgetX = FlagStartX;
		g_state.widgetW = ColourStartX - FlagStartX - 20;
		g_state.widgetY = RowStartY + 10;

		if (imguiButton(RowEntry, true))
		{
			if (GetAreaFlagModifier() > -1)
			{
				SetAreaFlagModifier(-1);
			}
			else
			{
				SetAreaFlagModifier(AreaDef->NavAreaIndex);
			}

			return true;
		}

		if (GetAreaFlagModifier() == AreaDef->NavAreaIndex)
		{
			g_state.widgetY += 20;

			static int levelScroll = 0;

			imguiBeginScrollArea("Choose Flag", FlagStartX, RowStartY + 20, ColourStartX - FlagStartX - 20, 200, &levelScroll);

			vector<NavFlagDefinition> AllFlags = GetAllNavFlagDefinitions();

			for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
			{
				if (imguiItem(it->FlagName.c_str()))
				{
					AreaDef->FlagIndex = it->NavFlagIndex;
					SetAreaFlagModifier(-1);
				}
			}

			imguiEndScrollArea();
		}

		g_state.widgetX = endWidgetX;
		g_state.widgetY = endWidgetY;
		g_state.widgetW = endWidgetW;
	}
	else
	{

		addGfxCmdText(FlagStartX, RowStartY, IMGUI_ALIGN_LEFT, RowEntry, imguiRGBA(255, 255, 255, 200));
	}

	int beforeColourX = g_state.widgetX;
	int beforeColourY = g_state.widgetY;
	int beforeColourW = g_state.widgetW;

	imguiDrawRect((float)ColourStartX, (float)RowStartY - 10, (float)25, (float)25, AreaDef->DebugColor);

	int afterColourX = g_state.widgetX;
	int afterColourY = g_state.widgetY;
	int afterColourW = g_state.widgetW;

	g_state.widgetX = ColourStartX;
	g_state.widgetY = RowStartY + 10;
	g_state.widgetW = DeleteStartX - ColourStartX - 10;

	if (imguiButton("", true, true))
	{
		unsigned int* CurrentFlagColorModifier = GetFlagColorModifier();

		ResetProfileMenus();

		if (CurrentFlagColorModifier != &AreaDef->DebugColor)
		{
			SetFlagColorModifier(&AreaDef->DebugColor);
		}
	}

	g_state.widgetX = afterColourX;
	g_state.widgetY = afterColourY;
	g_state.widgetW = afterColourW;

	if (GetFlagColorModifier() == &AreaDef->DebugColor)
	{
		g_state.widgetY += 20;

		static int areaScroll = 0;

		imguiBeginScrollArea("Choose Color", ColourStartX, RowStartY + 20, DeleteStartX - ColourStartX - 20, 150, &areaScroll);

		if (imguiSlider("R", &AreaDef->R, 0, 255, 1, true))
		{
			AreaDef->DebugColor = duRGBA2((int)AreaDef->R, (int)AreaDef->G, (int)AreaDef->B, 255);
		}

		if (imguiSlider("G", &AreaDef->G, 0, 255, 1, true))
		{
			AreaDef->DebugColor = duRGBA2((int)AreaDef->R, (int)AreaDef->G, (int)AreaDef->B, 255);
		}

		if (imguiSlider("B", &AreaDef->B, 0, 255, 1, true))
		{
			AreaDef->DebugColor = duRGBA2((int)AreaDef->R, (int)AreaDef->G, (int)AreaDef->B, 255);
		}

		if (imguiButton("Save"))
		{
			ResetProfileMenus();
		}

		imguiEndScrollArea();

		g_state.widgetX = afterColourX;
		g_state.widgetY = afterColourY;
		g_state.widgetW = afterColourW;
	}

	int endWidgetX = g_state.widgetX;
	int endWidgetY = g_state.widgetY;
	int endWidgetW = g_state.widgetW;

	g_state.widgetX = DeleteStartX;
	g_state.widgetW = 100;
	g_state.widgetY -= 20;

	if (AreaDef->bCustom)
	{
		if (imguiButton("Delete", true))
		{
			ResetProfileMenus();
			RemoveMovementArea(AreaIndex);
			return true;
		}
	}

	g_state.widgetX = endWidgetX;
	g_state.widgetY = endWidgetY;
	g_state.widgetW = endWidgetW;

	g_state.widgetY -= ROW_HEIGHT;

	return false;
}