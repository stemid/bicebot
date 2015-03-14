/*
 * The MIT License
 *
 * Copyright (c) 2008 Stefan Midjich
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* libircbot - some simple library functions for an ircbot
 * by nocturnal [at] swehack [dot] se */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <stdarg.h>
#include <errno.h>

#include <ircbot.h>

void parse_line(struct event *events, char *data, int dsize) {
	/* :noctest!~noctest@status.sa.blarf.se JOIN :#test
	 * :noctest!~noctest@status.sa.blarf.se QUIT : */
	int mi, i;

	events->type = EVENT_NULL;
	data++;

	/* first get the nickname and check for type of message */
	for(i=0,mi=0;*(data+mi) != ' ';i++,mi++) {
		if(*(data+mi) == '!') {
			events->type = EVENT_MSG;
			break;
		}
		if(i < NICKLEN-1) {
			events->sender.nickname[i] = *(data+mi);
		}
	}
	events->sender.nickname[i] = '\0';

	/* return if we have a server message */
	if(events->type == EVENT_NULL) {
		return;
	}

	/* TODO: strsep(3) LOLWUT?! more like strnsep AMIRITE?!?? */
	/* then get the username */
	for(i=0,++mi;*(data+mi) != '@' && i < NICKLEN-1;i++,mi++) {
		events->sender.username[i] = *(data+mi);
	}
	events->sender.username[i] = '\0';

	/* then get the hostname */
	for(i=0,++mi;*(data+mi) != ' ' && i < HOST_NAME_MAX;i++,mi++) {
		events->sender.hostname[i] = *(data+mi);
	}
	events->sender.hostname[i] = '\0';

	/* then get the servercommand */
	for(i=0,++mi;*(data+mi) != ' ' && i < SERVER_CMD_MAX-1;i++,mi++) {
		events->srvcmd[i] = *(data+mi);
	}
	events->srvcmd[i] = '\0';
	if(!strncmp(events->srvcmd, "JOIN", i)) {
		events->type = EVENT_JOIN;
	} else if(!strncmp(events->srvcmd, "PART", i)) {
		events->type = EVENT_PART;
	} else if(!strncmp(events->srvcmd, "QUIT", i)) {
		events->type = EVENT_QUIT;
	}

	/* then get the rcpt */
	if(*(data+(mi+1)) == ':') {
		++mi;
	}
	for(i=0,++mi;*(data+mi) != ' ' && i < CHANLEN-1;i++,mi++) {
		if(*(data+mi) == '\r') {
			break;
		}
		events->to[i] = *(data+mi);
	}
	events->to[++i] = '\0';

	/* AND THEN get the actual message */
	if(*(data+(mi+1)) == ':') {
		++mi;
	}
	/* TODO: make this function get rid of the leading ! in 
	 * bot commands, but make sure it's safe */
	if(*(data+(mi+1)) == '!') {
		events->type = EVENT_BOTCMD;
		/* ++mi; possibly enough */
	}
	for(i=0,++mi;*(data+mi) != '\r' && i < MSG_MAX-3;i++,mi++) {
		if(*(data+mi) == '\r' || *(data+mi) == '\n') {
			break;
		}
		events->msg[i] = *(data+mi);
	}
	events->msg[i] = '\0';

	return;
}

int irccmd(int sd, char *fmt, ...) {
	va_list ap;
	char *cp, *sval;
	char buf[MSG_MAX];
	int i = 0, ival;
	double dval;

	va_start(ap, fmt);

	for(cp = fmt; *cp; cp++) {
		if(i == MSG_MAX-2) {
			return(-1);
		}
		
		if(*cp != '%') {
			buf[i++] = *cp;
			continue;
		}
		
		switch(*++cp) {
			case 'c':
				ival = va_arg(ap, int);
				if(i == MSG_MAX-2) {
					return(-1);
				}
				buf[i++] = ival;
				break;
			case 's':
				for(sval = va_arg(ap, char *); *sval; sval++) {
					if(i == MSG_MAX-2) {
						return(-1);
					}
					buf[i++] = *sval;
				}
				break;
			case 'd':
				ival = va_arg(ap, int);
				if(i == MSG_MAX-2) {
					return(-1);
				}
				buf[i++] = ival;
				break;
			case 'f':
				dval = va_arg(ap, double);
				/* TODO: make it handle doubles(floats) */
				break;
			default:
				if(i == MSG_MAX-2) {
					return(-1);
				}
				buf[i++] = *cp;
				break;
		}
	}

	va_end(ap);

	if(i < MSG_MAX-2) {
		buf[i] = '\r';
		buf[++i] = '\n';
	} else {
		return(-1);
	}

	if(write(sd, buf, i) == -1) {
		return(-1);
	}

	return(i);
}

/* a debugging function */
void dump_events(struct event events) {
	printf("struct event {\n");
	printf("\tint type = %d;\n"
			"\tstruct from {\n"
			"\t\tchar nickname[%d] = \"%s\";\n"
			"\t\tchar username[%d] = \"%s\";\n"
			"\t\tchar hostname[%d] = \"%s\";\n"
			"\t} sender;\n"
			"\tchar srvcmd[%d] = \"%s\";\n"
			"\tchar to[%d] = \"%s\";\n"
			"\tchar msg[%d] = \"%s\";\n"
			"\tchar channel[%d] = \"%s\";\n"
			"\tchar servername[%d] = \"%s\";\n"
			"} events;\n", 
			events.type, NICKLEN, events.sender.nickname, NICKLEN, events.sender.username, 
			HOST_NAME_MAX+1, events.sender.hostname, SERVER_CMD_MAX, events.srvcmd, CHANLEN, 
			events.to, MSG_MAX-2, events.msg, CHANLEN, events.channel, HOST_NAME_MAX+1, events.servername);

	return;
}

int is_numeric(int ch) {
	int i;

	for(i=0;i<9;++i) {
		if(ch == i) {
			return(1);
		}
	}

	return(0);
}
