/*
 * fontInstaller by cxziaho
 *
 * MIT License
 * 
 * Copyright (c) 2017 cxz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h> 
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/power.h>
#include <psp2/pvf.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <vita2d.h>

#include "utils.h"

#define CROSS "\xE2\x95\xB3"
#define SQUARE "\xE2\x96\xA1"
#define TRIANGLE "\xE2\x96\xB3"
#define CIRCLE "\xE2\x97\x8B"

#define FONT_DIR "ux0:data/font/"
#define FONT_TEST "The quick brown fox jumps over the lazy dog"
#define MAX_FONTS 256
#define PLUGIN_CONFIG_PATH "ux0:data/font/config.txt"

#define STATE_MAIN 0
#define STATE_HELP 1
#define STATE_CONFIRM_RESTART 2

static float FONT_SCALES[4] = {72.0f, 52.0f, 40.0f, 32.0f};
static int SPACING_SCALES[4] = {120, 100, 90, 80};
static int FONTS_PER_PAGE[4] = {3, 4, 5, 5};

static int currSelection;
static int view;

static int pluginInstalled;

static int enterButton;

static int fontScale;
static int fontThreadDone;

static int fontCount;
static vita2d_font *fonts[MAX_FONTS];
static char *names[MAX_FONTS];
static char *styles[MAX_FONTS];
static char *fileNames[MAX_FONTS];
static char installedFont[32];

static int state = STATE_MAIN;

static vita2d_pgf *menuFont;
static SceCtrlData ctrl_peek;SceCtrlData ctrl_press;
static SceCtrlData ctrl_peek2;SceCtrlData ctrl_press2;

int initCtrl() {
	memset(&ctrl_peek, 0, sizeof(ctrl_peek));
	memset(&ctrl_press, 0, sizeof(ctrl_press));
	memset(&ctrl_peek2, 0, sizeof(ctrl_peek));
	memset(&ctrl_press2, 0, sizeof(ctrl_press));
}

int updateCtrl() {
	ctrl_press = ctrl_peek;
	sceCtrlPeekBufferPositive(0, &ctrl_peek, 1);
	ctrl_press.buttons = ctrl_peek.buttons & ~ctrl_press.buttons;
	ctrl_press2 = ctrl_peek2;
	sceCtrlReadBufferPositiveExt2(0, &ctrl_peek2, 1);
	ctrl_press2.buttons = ctrl_peek2.buttons & ~ctrl_press2.buttons;
	if (ctrl_press.buttons & SCE_CTRL_START) {
		state = STATE_CONFIRM_RESTART;
	} else if (ctrl_press.buttons & SCE_CTRL_SELECT) {
		sceIoRemove(PLUGIN_CONFIG_PATH);
		strcpy(installedFont, ""); 
	}
	if (state == STATE_CONFIRM_RESTART) {
		if (ctrl_press.buttons & SCE_CTRL_CIRCLE && enterButton || ctrl_press.buttons & SCE_CTRL_CROSS && !enterButton) {
			scePowerRequestColdReset();
		} else if (ctrl_press.buttons & SCE_CTRL_CIRCLE && !enterButton || ctrl_press.buttons & SCE_CTRL_CROSS && enterButton) {
			state = STATE_MAIN;
		}
	} else if (state == STATE_MAIN) {
		if (ctrl_press.buttons & SCE_CTRL_UP) {
			currSelection = currSelection-1;
			if (currSelection < 0) {
				currSelection = 0;
			} else if (currSelection < view) {
				view--;
				if (view < 0) view = 0;
			}
		}
		if (ctrl_press.buttons & SCE_CTRL_DOWN) {
			currSelection = currSelection+1;
			if (currSelection >= fontCount) {
				currSelection = fontCount - 1;
			} else if (currSelection > view+(FONTS_PER_PAGE[fontScale] - 1) && view < fontCount - FONTS_PER_PAGE[fontScale]) {
				view++;
			}
		}
		if (ctrl_press.buttons & SCE_CTRL_SQUARE) {
			fontScale = (fontScale + 1) % 4;
		}
		if (ctrl_press.buttons & SCE_CTRL_TRIANGLE) {
			state = STATE_HELP;
		}
		if (ctrl_press.buttons & SCE_CTRL_CIRCLE && enterButton || ctrl_press.buttons & SCE_CTRL_CROSS && !enterButton) {
			setFont(fileNames[currSelection]);
			strcpy(installedFont, fileNames[currSelection]);
		}
		if (ctrl_press2.buttons & SCE_CTRL_L1) {
			currSelection -= FONTS_PER_PAGE[fontScale];
			if (currSelection < 0)
				currSelection = 0;
			view -= FONTS_PER_PAGE[fontScale];
			if (view < 0)
				view = 0;
		}
		if (ctrl_press2.buttons & SCE_CTRL_R1) {
			currSelection += FONTS_PER_PAGE[fontScale];
			view += FONTS_PER_PAGE[fontScale];
			if (currSelection > fontCount) {
				currSelection = fontCount;
				view = fontCount - FONTS_PER_PAGE[fontScale] + 1;
			} else if (view + FONTS_PER_PAGE[fontScale] > fontCount - 1) {
				view = fontCount - FONTS_PER_PAGE[fontScale] + 1;
			}
		}
	} else if (state == STATE_HELP) {
		if (ctrl_press.buttons & SCE_CTRL_CIRCLE && !enterButton || ctrl_press.buttons & SCE_CTRL_CROSS && enterButton) {
			state = STATE_MAIN;
		}
	}
	return 0;
}

int initGfx() {
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0x00));

	menuFont = vita2d_load_default_pgf();
}

int renderFont(int i, int num, int selected) {
	int offset = ((SPACING_SCALES[fontScale] + 10)*num);
	vita2d_draw_rectangle(10, 50+offset, 910, SPACING_SCALES[fontScale], RGBA8(255, 255, 255, 255));
	vita2d_draw_rectangle(11+(selected*3), 51+(selected*4)+offset, 908-(selected*8), SPACING_SCALES[fontScale]-2-(selected*8), RGBA8(0, 0, 0, 255));
	if (strstr(fileNames[i], installedFont) != NULL && strlen(fileNames[i]) == strlen(installedFont))
		vita2d_pgf_draw_text(menuFont, 25, 85 + offset, RGBA8(255,255,255,255), 1.0f, "\u2605");
	vita2d_pgf_draw_textf(menuFont, 45, 85 + offset, RGBA8(255,255,255,255), 1.0f, "%s  %s", names[i], styles[i]);
	vita2d_pgf_draw_text(menuFont, 900-vita2d_pgf_text_width(menuFont, 1.0f, fileNames[i]), 85 + offset, RGBA8(255,255,255,255), 1.0f, fileNames[i]);
	vita2d_font_draw_text(fonts[i], 45, 50 + (SPACING_SCALES[fontScale]-20) + offset, RGBA8(255,255,255,255), FONT_SCALES[fontScale], FONT_TEST);

	vita2d_draw_rectangle(920, 50+offset, 40, SPACING_SCALES[fontScale], RGBA8(0,0,0,255));
	vita2d_draw_rectangle(900, 52+offset+(selected*4), 19-(selected*4), SPACING_SCALES[fontScale]-4-(selected*8), RGBA8(0,0,0,255));
}

int renderMenu() {
	vita2d_pgf_draw_text(menuFont, 20, 20, RGBA8(255,255,255,255), 1.0f, "fontInstaller");
	vita2d_pgf_draw_text(menuFont, 920-vita2d_pgf_text_width(menuFont, 1.0f, "font path: ux0:data/font/"), 20, RGBA8(255,255,255,255), 1.0f, "font path: ux0:data/font/");
	vita2d_draw_rectangle(10, 30, 940, 1, RGBA8(255, 255, 255, 255));

	vita2d_draw_rectangle(0, 500, 960, 60, RGBA8(0, 0, 0, 255));
	vita2d_draw_rectangle(10, 520, 940, 1, RGBA8(255, 255, 255, 255));
	vita2d_pgf_draw_textf(menuFont, 10, 540, RGBA8(255,255,255,255), 1.0f, "%s > select font | %s > change font scale", enterButton ? CIRCLE : CROSS, SQUARE);
	char bottom_menu_right[1024];
	snprintf(bottom_menu_right, 1024, "L and R > change page | %s > info", TRIANGLE);
	vita2d_pgf_draw_text(menuFont, 920-vita2d_pgf_text_width(menuFont, 1.0f, bottom_menu_right), 540, RGBA8(255,255,255,255), 1.0f, bottom_menu_right);
}

int renderHelp() {
	vita2d_draw_rectangle(80, 40, 800, 450, RGBA8(255, 255, 255, 255));
	vita2d_draw_rectangle(81, 41, 798, 448, RGBA8(0, 0, 0, 255));
	vita2d_pgf_draw_text(menuFont, 100, 70, RGBA8(255,255,255,255), 1.0f, "About");
	vita2d_pgf_draw_text(menuFont, 100, 90, RGBA8(255,255,255,255), 1.0f, "\
  fontInstaller is a companion app for my fontRedirect plugin.  Unlike the plugin,\n\
  however, it is very unstable :P  Feel free to rewrite this app!");
	vita2d_pgf_draw_text(menuFont, 100, 150, RGBA8(255,255,255,255), 1.0f, "Issues");
	vita2d_pgf_draw_text(menuFont, 100, 170, RGBA8(255,255,255,255), 1.0f, "\
  To use fontInstaller, you must move .ttf or .otf fonts into ux0:data/font/, and\n\
  select one in this app.  Then, reboot your Vita  If you are having any issues\n\
  with this, please go to the GitHub page and submit an issue there.  Thanks :)");
	vita2d_pgf_draw_text(menuFont, 100, 240, RGBA8(255,255,255,255), 1.0f, "Todo");
	vita2d_pgf_draw_text(menuFont, 100, 260, RGBA8(255,255,255,255), 1.0f, "  Fix slow cursor movement");
	vita2d_pgf_draw_text(menuFont, 100, 280, RGBA8(255,255,255,255), 1.0f, "  Fix slow loading of fonts (esp. large amounts of them)");
	vita2d_pgf_draw_text(menuFont, 100, 300, RGBA8(255,255,255,255), 1.0f, "  Add better sorting and selection tools to the UI");
	vita2d_pgf_draw_text(menuFont, 100, 340, RGBA8(255,255,255,255), 1.0f, "More Keys");
	vita2d_pgf_draw_text(menuFont, 100, 360, RGBA8(255,255,255,255), 1.0f, "  Start > Restart from anywhere in this app");
	vita2d_pgf_draw_text(menuFont, 100, 380, RGBA8(255,255,255,255), 1.0f, "  Select > Remove fonts (still leaves plugin)");
	vita2d_pgf_draw_text(menuFont, 100, 420, RGBA8(255,255,255,255), 1.0f, "fontInstaller and fontRedirect");
	vita2d_pgf_draw_text(menuFont, 100, 440, RGBA8(255,255,255,255), 1.0f, "  by cxziaho");
	vita2d_pgf_draw_text(menuFont, 470, 480, RGBA8(255,255,255,255), 1.0f, CROSS);
}

int renderTextInBox(char *text) {
	int width = vita2d_pgf_text_width(menuFont, 1.0f, text);
	int height = vita2d_pgf_text_height(menuFont, 1.0f, text);
	vita2d_draw_rectangle((960/2)-((width+20)/2), (540/2)-((height+20)/2), width+20, height+20, RGBA8(255,255,255,255));
	vita2d_draw_rectangle((960/2)-((width+18)/2), (540/2)-((height+18)/2), width+18, height+18, RGBA8(0, 0, 0, 255));
	vita2d_pgf_draw_text(menuFont, (960/2)-(width/2), (540/2)+(height/2), RGBA8(255,255,255,255), 1.0f, text);
}

int render() {
	vita2d_start_drawing();
	vita2d_clear_screen();

	int i;

	for (int i = 0; i < FONTS_PER_PAGE[fontScale] + 1 && i < (fontCount-view); i++) {
		renderFont(i+view, i, currSelection == i+view);
	}

	if (fontCount > FONTS_PER_PAGE[fontScale]) {
		vita2d_draw_rectangle(930, 50, 10, 460, RGBA8(255, 255, 255, 128));
		int scrollbarSize = ((FONTS_PER_PAGE[fontScale])/(double)fontCount) * 460;
		int scrollbarStart = 50 + view * ((460-scrollbarSize)/(fontCount-FONTS_PER_PAGE[fontScale]));
		vita2d_draw_rectangle(930, scrollbarStart, 10, scrollbarSize, RGBA8(255, 255, 255, 255));
	} else if (fontCount == 0) {
		renderTextInBox("please place fonts in ux0:data/font/ and restart this app");
	}

	renderMenu();
	if (state == STATE_HELP)
		renderHelp();
	else if (state == STATE_CONFIRM_RESTART)
		renderTextInBox("press enter to restart or press cancel to return");

	vita2d_end_drawing();
	vita2d_swap_buffers();
}

int deinitGfx() {
	vita2d_fini();
	vita2d_free_pgf(menuFont);
	for (int i = 0; i < fontCount; i++)
		vita2d_free_font(fonts[i]);
}

int loadFonts() {
	vita2d_start_drawing();
	vita2d_clear_screen();

	renderTextInBox("loading fonts... please wait");

	vita2d_end_drawing();
	vita2d_swap_buffers();

	int dfd = sceIoDopen(FONT_DIR);
	if (dfd < 0) {
		fontCount = 0;
		return dfd;
	}

	SceIoDirent dirent;
	memset(&dirent, 0, sizeof(SceIoDirent));
	int i = 0;

	while (sceIoDread(dfd, &dirent) > 0) {
		if (!strcmp(&dirent.d_name[strlen(dirent.d_name)-4], ".ttf") || !strcmp(&dirent.d_name[strlen(dirent.d_name)-4], ".otf") || !strcmp(&dirent.d_name[strlen(dirent.d_name)-4], ".pvf") ||
			!strcmp(&dirent.d_name[strlen(dirent.d_name)-4], ".TTF") || !strcmp(&dirent.d_name[strlen(dirent.d_name)-4], ".OTF") || !strcmp(&dirent.d_name[strlen(dirent.d_name)-4], ".PVF")) {
			fileNames[i] = strdup(dirent.d_name);
			char path[1024];
			snprintf(path, 1024, "%s%s", FONT_DIR, dirent.d_name);
			fonts[i] = vita2d_load_font_file(path);
			vita2d_pvf *temp_pvf = vita2d_load_custom_pvf(path);
			names[i] = vita2d_pvf_get_font_name(temp_pvf);
			styles[i] = vita2d_pvf_get_font_style(temp_pvf);
			vita2d_free_pvf(temp_pvf);
			i++;
			if (i > MAX_FONTS)
				break;
		}
	}
	fontCount = i;
	sceIoDclose(dfd);
	return 0;
}

int main(int argc, char *argv[]) {
	initGfx();

	if (!isPluginInstalled()) {
		vita2d_start_drawing();
		vita2d_clear_screen();
		renderTextInBox("installing plugin to config.txt.  please wait");
		vita2d_end_drawing();
		vita2d_swap_buffers();
		installPlugin();
		sceKernelDelayThread(1000000);
	}
	enterButton = getEnterButton();
	strcpy(installedFont, getCurrentFont());
	initCtrl();
	loadFonts();

	while (1) {
		if (updateCtrl() == -1) break;
		render();
	}

	deinitGfx();

	sceKernelExitProcess(0);
	return 0;
}
