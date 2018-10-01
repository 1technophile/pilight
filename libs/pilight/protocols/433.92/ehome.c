/*
	Copyright (C) 2014 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "ehome.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	277
#define MAX_PULSE_LENGTH	287
#define AVG_PULSE_LENGTH	282
#define RAW_LENGTH				50

static int validate(void) {
	if(ehome->rawlen == RAW_LENGTH) {
		if(ehome->raw[ehome->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   ehome->raw[ehome->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(char **message, int id, int state) {
	int x = snprintf((*message), 255, "{\"id\":%d,", id);

	if(state == 1) {
		x += snprintf(&(*message)[x], 255-x, "\"state\":\"on\"");
	} else {
		x += snprintf(&(*message)[x], 255-x, "\"state\":\"off\"");
	}
	x += snprintf(&(*message)[x], 255-x, "}");
}

static void parseCode(char **message) {
	int i = 0, binary[RAW_LENGTH/4];

	if(ehome->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "ehome: parsecode - invalid parameter passed %d", ehome->rawlen);
		return;
	}

	for(i=0;i<ehome->rawlen-2;i+=4) {
		if(ehome->raw[i+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i/4]=1;
		} else {
			binary[i/4]=0;
		}
	}

	int id = binToDec(binary, 1, 3);
	int state = binary[0];

	createMessage(message, id, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		ehome->raw[i]=AVG_PULSE_LENGTH;
		ehome->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		ehome->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		ehome->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createMed(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		ehome->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		ehome->raw[i+1]=AVG_PULSE_LENGTH;
		ehome->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		ehome->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		ehome->raw[i]=AVG_PULSE_LENGTH;
		ehome->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		ehome->raw[i+2]=AVG_PULSE_LENGTH;
		ehome->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createLow(0,47);
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createHigh(4+x, 4+(x+3));
		}
	}
}

static void createState(int state) {
	if(state == 0) {
		createMed(0, 3);
	} else {
		createHigh(0, 3);
	}
}

static void createFooter(void) {
	ehome->raw[48]=(AVG_PULSE_LENGTH);
	ehome->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code, char **message) {
	int id = -1;
	int state = -1;
	double itmp = 0;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(id == -1 || state == -1) {
		logprintf(LOG_ERR, "ehome: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 7 || id < 0) {
		logprintf(LOG_ERR, "ehome: invalid id range");
		return EXIT_FAILURE;
	} else {
		createMessage(message, id, state);
		clearCode();
		createId(id);
		createState(state);
		createFooter();
		ehome->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void ehomeInit(void) {

	protocol_register(&ehome);
	protocol_set_id(ehome, "ehome");
	protocol_device_add(ehome, "ehome", "eHome Switches");
	ehome->devtype = SWITCH;
	ehome->hwtype = RF433;
	ehome->minrawlen = RAW_LENGTH;
	ehome->maxrawlen = RAW_LENGTH;
	ehome->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	ehome->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&ehome->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-4])$");
	options_add(&ehome->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&ehome->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&ehome->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&ehome->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	ehome->parseCode=&parseCode;
	ehome->createCode=&createCode;
	ehome->printHelp=&printHelp;
	ehome->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "ehome";
	module->version = "2.0";
	module->reqversion = "7.0";
	module->reqcommit = "94";
}

void init(void) {
	ehomeInit();
}
#endif
