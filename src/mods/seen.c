#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ndbm.h>
#include <time.h>

#include <ircbot.h>
#include <bicebot.h>

int seen(int, struct event);
void itoa(int, char []);
void reverse(char []);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, EVENT_QUIT, EVENT_PART, EVENT_JOIN, 0 },
	&seen,
	"seen",
	"Seen DB",
	"Stores information about users",
	"nocturnal [at] swehack [dot] se",
	"0.2a"
};

int seen(int sd, struct event events) {
	int i, mi, r;
	char *msg = events.msg;
	char arg[NICKLEN];

	time_t timestamp;
	struct tm *loctime;
	char timebuf[32];
	
	DBM *dbp = NULL;
	datum key;
	datum data;
	char seen_db[1024];

	if((events.to[0] != '#' || events.to == NULL) && events.type != EVENT_QUIT) {
		return(-4); /* -4 - we can call it malformed input data */
	}

	if(!strlen(events.channel)) {
		printf("returned here: %d\n", __LINE__);
		return(-4);
	}

	snprintf(seen_db, 1024, "%s/seen_%s-%s", DATA_PATH, events.servername, events.channel);

	/* someone calls the !seen command */
	if(*msg == '!' && events.type == EVENT_BOTCMD) {
		for(i=0,mi=strlen(_module_info.m_cmd)+2;*(msg+mi) != '\0' && i < NICKLEN-1;i++,mi++) {
			if(*(msg+mi) == ' ') {
				break;
			}
			arg[i] = *(msg+mi);
		}
		arg[i] = '\0';

		if((dbp = dbm_open(seen_db, (O_RDWR | O_CREAT), 0660)) == NULL) {
			return(-1);
		}

		key.dsize = i;
		key.dptr = arg;

		data = dbm_fetch(dbp, key);
		
		if(data.dptr != NULL) {
			timestamp = (int)atoi(data.dptr);
			loctime = localtime(&timestamp);
			strftime(timebuf, 32, "%c", loctime);

			irccmd(sd, "PRIVMSG %s :%s: %s was last seen on %s", events.to, events.sender.nickname, arg, timebuf);
		}

		dbm_close(dbp);
	} else if(events.type == EVENT_JOIN || events.type == EVENT_PART || events.type == EVENT_QUIT) {
		timestamp = time(NULL);

		if((dbp = dbm_open(seen_db, (O_RDWR | O_CREAT), 0660)) == NULL) {
			return(-1);
		}
		
		if((key.dptr = malloc((sizeof(char)*16)+1)) == NULL) {
			dbm_close(dbp);
			return(-2);
		}

		if((data.dptr = malloc(sizeof(char)*strlen(events.sender.nickname)+1)) == NULL) {
			dbm_close(dbp);
			return(-2);
		}

		key.dptr = events.sender.nickname;
		key.dsize = strlen(events.sender.nickname);

		itoa((int)timestamp, data.dptr);
		data.dsize = strlen(data.dptr)+2;
		*(data.dptr+data.dsize-1) = '\0';

		/* finally store it */
		r = dbm_store(dbp, key, data, DBM_REPLACE); 

		dbm_close(dbp);
	}

	return(0);
}

void itoa(int n, char s[]) {
	int i, sign; 

	if((sign = n) < 0) {
		n = -n;
	}   
	i = 0;
	do {
		s[i++] = n%10+'0';
	} while((n /= 10) > 0);
	if(sign < 0) {
		s[i++] = '-';
	}   
	s[i] = '\0';
	reverse(s);
}

void reverse(char s[]) {
	int c, i, j;

	for(i=0,j=strlen(s)-1;i<j;i++,j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}
