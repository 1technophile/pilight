/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

    pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

    pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#ifndef _PROTOCOL_ARCTECH_SCREEN_H_
#define _PROTOCOL_ARCTECH_SCREEN_H_

struct protocol_t *arctech_screen;

void arctechSrInit(void);
void arctechSrCreateMessage(int id, int unit, int state, int all);
void arctechSrParseBinary(void);
int arctechSrCreateCode(JsonNode *code);
void arctechSrCreateLow(int s, int e);
void arctechSrCreateHigh(int s, int e);
void arctechSrClearCode(void);
void arctechSrCreateStart(void);
void arctechSrCreateId(int id);
void arctechSrCreateAll(int all);
void arctechSrCreateState(int state);
void arctechSrCreateUnit(int unit);
void arctechSrCreateFooter(void);
void arctechSrPrintHelp(void);

#endif
