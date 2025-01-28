/* gcurses.h - 'great' cursor optimizations */
/* Copyright (c) 2024 Ian P. Jarvis */
/* Licensed under LGPL 2.1 */

#ifndef GCURSES_H
#define GCURSES_H /* GCURSES_H */

#include <stdbool.h>

struct FG {
	unsigned short red;
	unsigned short green;
	unsigned short blue;
};

struct BG {
	unsigned short red;
	unsigned short green;
	unsigned short blue;
};

struct ATTR {
	unsigned char attributes;
	/*
	0 = reset (0)
	1 = bold (1)
	2 = faint (2)
	4 = italic (3)
	8 = unserline (4)
	16 = blinking (5)
	32 = reverse (7)
	64 = invisible (8)
	128 = strikethrough (9)
	*/
};

struct PANEL {
	unsigned int starty;
	unsigned int startx;
	unsigned int endy;
	unsigned int endx;
	bool border;
	char** contents;
	struct FG** fg;
	struct BG** bg;
	struct ATTR** attr;
};

struct SCREEN {
	struct PANEL** panels;
	unsigned short len;
};

void gcurses_start();

void screen_init(struct SCREEN*);

void gcurses_setfg(struct FG*, unsigned short, unsigned short, unsigned short);

void gcurses_setbg(struct BG*, unsigned short, unsigned short, unsigned short);

void gcurses_setattr(struct ATTR*, unsigned char);

void gcurses_printchar(struct FG*, struct BG*, char);

void gcurses_printchar_attr(struct FG*, struct BG*, char, struct ATTR*);

void gcurses_printstr(struct FG*, struct BG*, char*);

void gcurses_printstr_attr(struct FG*, struct BG*, char*, struct ATTR*);

void gcurses_move(unsigned int, unsigned int);

void panel_printchar_attr(struct PANEL*, unsigned int, unsigned int, struct FG*, struct BG*, char, struct ATTR*);

void panel_printchar(struct PANEL*, unsigned int, unsigned int, struct FG*, struct BG*, char);

void panel_printstr(struct PANEL*, unsigned int, unsigned int, struct FG*, struct BG*, char*);

void panel_printstr_attr(struct PANEL*, unsigned int, unsigned int, struct FG*, struct BG*, char*, struct ATTR*);

void panel_border(struct PANEL*, struct FG*, struct BG*);

void panel_no_border(struct PANEL*);

void panel_new(struct SCREEN*, struct PANEL*, unsigned int, unsigned int, unsigned int, unsigned int, bool);

void gcurses_refresh(struct SCREEN*);

void panel_destroy(struct SCREEN*, struct PANEL*);

void panel_move(struct SCREEN*, struct PANEL*, unsigned int, unsigned int);

void panel_resize(struct SCREEN*, struct PANEL*, unsigned int, unsigned int);

void panel_bottom(struct SCREEN*, struct PANEL*);

void panel_top(struct SCREEN*, struct PANEL*);

void screen_end(struct SCREEN*);

void gcurses_end();

#endif /* GCURSES_H */
