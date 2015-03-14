/*
 * Copyright (c) 1980, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Modified and indented for ircbot, by nocturnal [at] swehack [dot] se
 */

#include <err.h>
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <ircbot.h>
#include <bicebot.h>

int leave(int, struct event);
void doalarm(int, struct event, u_int, char *);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 },
	&leave,
	"leave",
	"BSD Leave(1)",
	"Reminds you when to leave",
	"nocturnal [at] swehack [dot] se",
	"0.5b"
};

/*
 * leave [[+]hhmm]
 *
 * Reminds you when you have to leave.
 * Leave prompts for input and goes away if you hit return.
 * It nags you like a mother hen.
 */
int leave(int sd, struct event events) {
	u_int secs;
	int hours, minutes, i, mi;
	char c, *cp = NULL, arg[NICKLEN], *msg = events.msg;
	struct tm *t;
	time_t now;
	int plusnow, t_12_hour;
	char reason[64];

	if (setlocale(LC_TIME, "") == NULL) {
		warn("setlocale");
	}

	/* get the argument */
	for(i=0,mi=strlen(_module_info.m_cmd)+2;*(msg+mi) != '\0' && i < NICKLEN-1;++i,++mi) {
		if(*(msg+mi) == ' ') {
			break;
		}
		arg[i] = *(msg+mi);
	}
	arg[i] = '\0';
	cp = arg;

	if(*cp == '\0') {
		return(-1);
	}

	/* get the reason */
	for(i=0,++mi;i<64 || *(msg+mi) == '\r';++i,++mi) {
		reason[i] = *(msg+mi);
	}
	reason[i] = '\0';

	if(reason[0] == '\0') {
		strncpy(reason, "no reason given", 15);
	}

	/* TODO: cp is the argument to the command */
	if (*cp == '+') {
		plusnow = 1;
		++cp;
	} else {
		plusnow = 0;
	}

	for (hours = 0; (c = *cp) && c != '\n'; ++cp) {
		if (!isdigit(c)) {
			return(-1);
		}
		hours = hours * 10 + (c - '0');
	}
	minutes = hours % 100;
	hours /= 100;

	if (minutes < 0 || minutes > 59) {
		return(-1);
	}
	if (plusnow) {
		secs = hours * 60 * 60 + minutes * 60;
	} else {
		(void)time(&now);
		t = localtime(&now);

		if (hours > 23) {
			return(-1);
		}

		/* Convert tol to 12 hr time (0:00...11:59) */
		if (hours > 11) {
			hours -= 12;
		}

		/* Convert tm to 12 hr time (0:00...11:59) */
		if (t->tm_hour > 11) {
			t_12_hour = t->tm_hour - 12;
		} else {
			t_12_hour = t->tm_hour;
		}

		if (hours < t_12_hour || (hours == t_12_hour && minutes <= t->tm_min)) {
			/* Leave time is in the past so we add 12 hrs */
			hours += 12;
		}

		secs = (hours - t_12_hour) * 60 * 60;
		secs += (minutes - t->tm_min) * 60;
		secs -= now % 60;	/* truncate (now + secs) to min */
	}
	doalarm(sd, events, secs, reason);

	return(0);
}

void doalarm(int sd, struct event events, u_int secs, char *reason) {
	int bother;
	time_t daytime;
	char tb[80];
	int pid;

	if ((pid = fork())) {
		(void)time(&daytime);
		daytime += secs;
		strftime(tb, sizeof(tb), "%+", localtime(&daytime));
		if(events.to[0] == '#') {
			irccmd(sd, "PRIVMSG %s :%s: Alarm set for %s. (%s)", events.to, events.sender.nickname, tb, reason);
		} else {
			irccmd(sd, "PRIVMSG %s :Alarm set for %s. (%s)", events.sender.nickname, tb, reason);
		}
		return;
	}
#ifdef SETPROCTITLE
	setproctitle("leave: %s at %s", events.sender.nickname, events.servername);
#endif
	sleep((u_int) 2);	/* let parent print set message */
	if (secs >= 2) {
		secs -= 2;
	}

	/* if write fails, we've lost the socket descriptor */
#define	FIVEMIN	(5 * 60)
#define	MSG2	"You have to leave in 5 minutes."
	if (secs >= FIVEMIN) {
		sleep(secs - FIVEMIN);
		if (irccmd(sd, "PRIVMSG %s :%s (%s)", events.sender.nickname, MSG2, reason) < 0) {
			exit(0);
		}
		secs = FIVEMIN;
	}
#define	ONEMIN	(60)
#define	MSG3	"Just one more minute!"
	if (secs >= ONEMIN) {
		sleep(secs - ONEMIN);
		if (irccmd(sd, "PRIVMSG %s :%s (%s)", events.sender.nickname, MSG3, reason) < 0) {
			exit(0);
		}
	}
#define	MSG4	"Time to leave!"
	for (bother = 5; bother--;) {
		sleep((u_int) ONEMIN);
		if (irccmd(sd, "PRIVMSG %s :%s (%s)", events.sender.nickname, MSG4, reason) < 0) {
			exit(0);
		}
	}

#define	MSG5	"That was the last time I'll tell you.  Bye."
	(void)irccmd(sd, "PRIVMSG %s :%s (%s)", events.sender.nickname, MSG5, reason);
	exit(0);
}

