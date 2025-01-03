/* gcurses.c - 'great' cursor optimizations */
/* Copyright (c) 2024 Ian P. Jarvis */
/* Licensed under LGPL 3 */

#include "gcurses.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned char MAX_PANELS = 255;

void screen_init(struct SCREEN* screen) {
	/* enable alt screen */
	printf("\e[?1049h");

	/* disable line wrap */
	printf("\e[=7l");

	/* initialize screen panels */
	screen->panels = (struct PANEL**)malloc(MAX_PANELS * sizeof(struct PANEL*));
	for(unsigned char g=0; g<MAX_PANELS; g++)
		screen->panels[g] = NULL;

	/* initialize screen len */
	screen->len = 0;
}

void gcurses_setfg(struct FG* fg, unsigned short red, unsigned short green, unsigned short blue) {
	if(red > 255) red = 255;
	if(green > 255) green = 255;
	if(blue > 255) blue = 255;
	fg->red = red;
	fg->green = green;
	fg->blue = blue;
}

void gcurses_setbg(struct BG* bg, unsigned short red, unsigned short green, unsigned short blue) {
	if(red > 255) red = 255;
	if(green > 255) green = 255;
	if(blue > 255) blue = 255;
	bg->red = red;
	bg->green = green;
	bg->blue = blue;
}

void gcurses_setattr(struct ATTR* attr, unsigned char n) {
	attr->attributes = n;
}

void gcurses_printchar(struct FG* fg, struct BG* bg, char ch) {
	printf("\e[38;2;%hu;%hu;%hum", fg->red, fg->green, fg->blue);
	printf("\e[48;2;%hu;%hu;%hum", bg->red, bg->green, bg->blue);
	printf("%c\e[0m", ch);
}

void gcurses_printchar_attr(struct FG* fg, struct BG* bg, char ch, struct ATTR* attr) {
	if(attr->attributes == 0) printf("\e[0m");
	else {
		unsigned char b = 1;
		for(unsigned short n=1; n<129; n<<=1) {
			if(b == 6) b++;
			if(n & attr->attributes)
				printf("\e[%hum", b);
			b++;
		}
	}
	gcurses_printchar(fg, bg, ch);
}

void gcurses_printstr(struct FG* fg, struct BG* bg, char* str) {
	for(unsigned int i=0; i<strlen(str); i++)
		gcurses_printchar(fg, bg, str[i]);
}

void gcurses_printstr_attr(struct FG* fg, struct BG* bg, char* str, struct ATTR* attr) {
	for(unsigned int i=0; i<strlen(str); i++)
		gcurses_printchar_attr(fg, bg, str[i], attr);
}

void gcurses_move(unsigned int y, unsigned int x) {
	printf("\e[%u;%uH", y, x);
}

void panel_printchar_attr(struct PANEL* panel, unsigned int y, unsigned int x, struct FG* fg, struct BG* bg, char ch, struct ATTR* attr) {
	if(y > (panel->endy - panel->starty - 1)) y = panel->endy - panel->starty - 1;
	if(x > (panel->endx - panel->startx - 1)) x = panel->endx - panel->startx - 1;
	gcurses_move(panel->starty + y, panel->startx + x);
	gcurses_printchar_attr(fg, bg, ch, attr);
	panel->contents[y][x] = ch;
	gcurses_setfg(&(panel->fg[y][x]), fg->red, fg->green, fg->blue);
	gcurses_setbg(&(panel->bg[y][x]), bg->red, bg->green, bg->blue);
	gcurses_setattr(&(panel->attr[y][x]), attr->attributes);
}

void panel_printchar(struct PANEL* panel, unsigned int y, unsigned int x, struct FG* fg, struct BG* bg, char ch) {
	struct ATTR attr;
	gcurses_setattr(&attr, 0);
	panel_printchar_attr(panel, y, x, fg, bg, ch, &attr);
}

void panel_printstr(struct PANEL* panel, unsigned int y, unsigned int x, struct FG* fg, struct BG* bg, char* str) {
	for(unsigned int i=0; i<strlen(str) && i<(panel->endx - panel->startx); i++)
		panel_printchar(panel, y, x + i, fg, bg, str[i]);
}

void panel_printstr_attr(struct PANEL* panel, unsigned int y, unsigned int x, struct FG* fg, struct BG* bg, char* str, struct ATTR* attr) {
	for(unsigned int i=0; i<strlen(str); i++)
		panel_printchar_attr(panel, y, x + i, fg, bg, str[i], attr);
}

void panel_border(struct PANEL* panel, struct FG* fg, struct BG* bg) {
	/* top side */
	for(unsigned int x=0; x<(panel->endx - panel->startx); x++)
		panel_printchar(panel, 0, x, fg, bg, ' ');
	/* left side */
	for(unsigned int y=0; y<(panel->endy - panel->starty); y++)
		panel_printchar(panel, y, 0, fg, bg, ' ');
	/* bottom side */
	for(unsigned int x=0; x<(panel->endx - panel->startx); x++)
		panel_printchar(panel, panel->endy - panel->starty - 1, x, fg, bg, ' ');
	/* right side */
	for(unsigned int y=0; y<(panel->endy - panel->starty); y++)
		panel_printchar(panel, y, panel->endx - panel->startx - 1, fg, bg, ' ');
	panel->border = true;
}

void panel_no_border(struct PANEL* panel) {
	struct FG fg;
	gcurses_setfg(&fg, 255, 255, 255);
	struct BG bg;
	gcurses_setbg(&bg, 0, 0, 0);
	panel_border(panel, &fg, &bg);
	panel->border = false;
}

void panel_new(struct SCREEN* screen, struct PANEL* panel, unsigned int starty, unsigned int startx, unsigned int lines, unsigned int cols, bool border) {
	/* sanity check */
	if(!(screen->len < MAX_PANELS)) return;

	/* set panel dimensions */
	panel->starty = starty;
	panel->startx = startx;
	panel->endy = starty + lines;
	panel->endx = startx + cols;

	/* initalize panel contents, fg, bg, attr sizes */
	panel->contents = (char**)malloc(lines * sizeof(char*));
	panel->fg = (struct FG**)malloc(lines * sizeof(struct FG*));
	panel->bg = (struct BG**)malloc(lines * sizeof(struct BG*));
	panel->attr = (struct ATTR**)malloc(lines * sizeof(struct ATTR*));
	for(unsigned int y=0; y<lines; y++) {
		panel->contents[y] = (char*)malloc(cols * sizeof(char));
		panel->fg[y] = (struct FG*)malloc(cols * sizeof(struct FG));
		panel->bg[y] = (struct BG*)malloc(cols * sizeof(struct BG));
		panel->attr[y] = (struct ATTR*)malloc(cols * sizeof(struct ATTR));
	}

	/* set default values for panel contents, fg, bg, attr */
	struct FG fg;
	struct BG bg;
	struct ATTR attr;
	gcurses_setfg(&fg, 255, 255, 255);
	gcurses_setbg(&bg, 0, 0, 0);
	gcurses_setattr(&attr, 0);
	for(unsigned int y=0; y<lines; y++) {
		for(unsigned int x=0; x<cols; x++)
			panel_printchar_attr(panel, y, x, &fg, &bg, ' ', &attr);
	}

	/* maybe set border */
	if(border) {
		gcurses_setfg(&fg, 0, 0, 0);
		gcurses_setbg(&bg, 255, 255, 255);
		panel_border(panel, &fg, &bg);
		gcurses_move(starty+1, startx+1);
	}
	else {
		panel->border = false;
		gcurses_move(starty, startx);
	}

	/* add panel to screen */
	screen->panels[screen->len] = panel;
	(screen->len)++;
}

void gcurses_refresh(struct SCREEN* screen) {
	/* enable alt screen to refresh */
	printf("\e[?1049h");

	struct FG* panel_fg;
	struct BG* panel_bg;
	struct ATTR* panel_attr;

	/* iterate through each panel */
	for(unsigned char g=0; g<screen->len; g++) {

		/* iterate through each panel y, x to print everything again */
		for(unsigned int y=0; y<(screen->panels[g]->endy - screen->panels[g]->starty); y++) {
			gcurses_move(screen->panels[g]->starty + y, screen->panels[g]->startx);
			for(unsigned int x=0; x<(screen->panels[g]->endx - screen->panels[g]->startx); x++) {
				panel_fg = &(screen->panels[g]->fg[y][x]);
				panel_bg = &(screen->panels[g]->bg[y][x]);
				panel_attr = &(screen->panels[g]->attr[y][x]);
				gcurses_printchar_attr(panel_fg, panel_bg, screen->panels[g]->contents[y][x], panel_attr);
			}
		}
	}
}

void panel_destroy(struct SCREEN* screen, struct PANEL* panel) {
	for(unsigned int y=0; y<(panel->endy - panel->starty); y++) {
		free(panel->contents[y]);
		free(panel->fg[y]);
		free(panel->bg[y]);
		free(panel->attr[y]);
	}
	free(panel->contents);
	free(panel->fg);
	free(panel->bg);
	free(panel->attr);

	for(unsigned char g=0; g<screen->len; g++) {
		if(screen->panels[g] == panel) {
			for(unsigned char h=g; h<(screen->len - 1); h++)
				screen->panels[h] = screen->panels[h+1];
			screen->panels[screen->len - 1] = NULL;
			(screen->len)--;
			break;
		}
	}
}

void panel_move(struct SCREEN* screen, struct PANEL* panel, unsigned int starty, unsigned int startx) {
	/* reassign endy, endx of panel */
	if(starty >= panel->starty)
		panel->endy += starty - panel->starty;
	else	panel->endy -= panel->starty - starty;
	if(startx >= panel->startx)
		panel->endx += startx - panel->startx;
	else	panel->endx -= panel->startx - startx;

	/* reassign starty, startx of panel */
	panel->starty = starty;
	panel->startx = startx;
}

void panel_resize(struct SCREEN* screen, struct PANEL* panel, unsigned int lines, unsigned int cols) {
	/* save border status */
	bool border = panel->border;
	if(border) panel_no_border(panel);

	/* create copy of panel */
	struct PANEL panel_copy;
	panel_new(screen, &panel_copy, panel->starty, panel->startx, panel->endy - panel->starty, panel->endx - panel->startx, false);

	/* copy panel into panel_copy */
	for(unsigned int y=0; y<(panel->endy - panel->starty); y++) {
		for(unsigned int x=0; x<(panel->endx - panel->startx); x++)
			panel_printchar_attr(&panel_copy, y, x, &(panel->fg[y][x]), &(panel->bg[y][x]), panel->contents[y][x], &(panel->attr[y][x]));
	}

	/* resize panel */
	panel_destroy(screen, panel);
	panel_new(screen, panel, panel_copy.starty, panel_copy.startx, lines, cols, border);

	/* initialize panel with panel_copy where possible */
	struct FG fg;
	struct BG bg;
	struct ATTR attr;
	gcurses_setfg(&fg, 255, 255, 255);
	gcurses_setbg(&bg, 0, 0, 0);
	gcurses_setattr(&attr, 0);
	for(unsigned int y=0; y<(panel_copy.endy - panel_copy.starty) && y<lines; y++) {
		for(unsigned int x=0; x<(panel_copy.endx - panel_copy.startx) && x<cols; x++)
			panel_printchar_attr(panel, y, x, &(panel_copy.fg[y][x]), &(panel_copy.bg[y][x]), panel_copy.contents[y][x], &(panel_copy.attr[y][x]));
		/* potential leftover spots */
		for(unsigned int c=(panel_copy.endx - panel_copy.startx); c<cols; c++)
			panel_printchar_attr(panel, y, c, &fg, &bg, ' ', &attr);
	}
	/* potential leftover lines */
	for(unsigned int l=(panel_copy.endy - panel_copy.starty); l<lines; l++) {
		for(unsigned int x=0; x<cols; x++)
			panel_printchar_attr(panel, l, x, &fg, &bg, ' ', &attr);
	}

	/* destroy panel_copy */
	panel_destroy(screen, &panel_copy);
	
	/* maybe add border */
	if(border) {
		gcurses_setfg(&fg, 0, 0, 0);
		gcurses_setbg(&bg, 255, 255, 255);
		panel_border(panel, &fg, &bg);
	}
}

void panel_bottom(struct SCREEN* screen, struct PANEL* panel) {
	for(unsigned char g=0; g<MAX_PANELS; g++) {
		if(screen->panels[g] == panel) {
			for(unsigned char h=g; h>0; h--)
				screen->panels[h] = screen->panels[h-1];
			screen->panels[0] = panel;
			break;
		}
	}
}

void panel_top(struct SCREEN* screen, struct PANEL* panel) {
	for(unsigned char g=0; g<MAX_PANELS; g++) {
		if(screen->panels[g] == panel) {
			for(unsigned char h=g; h<(screen->len - 1); h++)
				screen->panels[h] = screen->panels[h+1];
			screen->panels[screen->len - 1] = panel;
			break;
		}
	}
}

void screen_end(struct SCREEN* screen) {
	/* destroy panels on screen */
	for(unsigned char g=0; g<screen->len; g++)
		panel_destroy(screen, screen->panels[g]);
	free(screen->panels);

	/* disable alt screen */
	printf("\e[?1049l");

	/* enable line wrap */
	printf("\e[=7h");
}
