//Author: Adam Harms
//Date: 3.13.2017
//CSC 4100 - Operating Systems
//Mike Rogers
//scan_map.h is the header for the SCAN_MAP

#ifndef SCAN_MAP_H
#define SCAN_MAP_H

char* keys = "??1234567890-=\b\tqwertyuiop[]\n?asdfghjkl;'`?\\zxcvbnm,./??? ?";
char* skeys = "??!@#$%^&*()_+\b\tQWERTYUIOP{}\n?ASDFGHJKL:\"~?|ZXCVBNM<>???? ?";

enum scanmap
{
	NONE,
	ESC, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
	ZERO, DASH, EQUAL, BACKSPACE, TAB, Q, W, E, R, T,
	Y, U, I, O, P, OPEN_BRACKET, CLOSED_BRACKET, ENTER, CTRL, A,
	S, D, F, G, H, J, K, L, SEMI, QUOTE,
	TILDE, LSHIFT, BACK_SLASH, Z, X, C, V, B, N, M,
	COMMA, PERIOD, SLASH, RSHIFT, PRT_SCR, ALT, SPACE, CAPS_LOCK
};

#endif
