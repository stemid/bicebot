/* this is like a template module and could easily be 
 * converted to a more generic quotes module */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>

#include <ircbot.h>
#include <bicebot.h>

/* try to prefix macros with a module specific name 
 * to avoid confusion */
#define JERK_QUOTESFILE "jerk_quotes.txt"
#define JERK_MAXQUOTES 512
#define JERK_MAXLEN 512

int jerk_it(int, struct event);

/* this is information that the ircbot will 
 * have about your module, you should not 
 * change the m_ld value.
 * this is also where you add events for 
 * the module to respond to */
struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 }, /* events, check README for more info */
	&jerk_it, 		/* main function name */
	"jerk", 		/* command string, (16) */
	"Jerkcity", 	/* name of module (16) */
	"Shows quotes from a dumb website", /* short description (128) */
	"nocturnal [at] swehack [dot] se", /* author (64) */
	"0.1a" 			/* version number, as a string (8) */
};

/* declare any global variables for the module */
char jerk_quotes[JERK_MAXQUOTES][JERK_MAXLEN];
int jerk_j = 0;

/* please prefix all function names and global variables 
 * in modules with a module specific name, for example 
 * it's filename or command name, just to avoid confusion */
int jerk_it(int sd, struct event events) {
	time_t t;

	t = time(&t) / random();
	srandom(getpid() + (int)((t >> 16) + t + time(&t)));

	if(jerk_j > 0) {
		irccmd(sd, "PRIVMSG %s :%s", events.to, jerk_quotes[random() % jerk_j]);
	}

	return(0);
}

/* you don't need an _init function but if you have 
 * one it will be executed once every time the module 
 * is loaded, you could for example have it store data 
 * that you need to access a lot in memory */
void _init(void) {
	int fd, n = 0, c, i = 0;
	ssize_t r;
	char buffer[512];
	char quotes_path[255+255+1];

	/* first setup the environment for ircbot to know of the mod */

	snprintf(quotes_path, 255+255+1, "%s/%s", DATA_PATH, JERK_QUOTESFILE);
	
	if((fd = open(quotes_path, O_RDONLY)) == -1) {
		return;
	}

	while((r = read(fd, buffer, 512)) > 0) {
		for(c=0;c < r && i < JERK_MAXLEN && jerk_j < JERK_MAXQUOTES;++c) {
			if((buffer[c] == '\n' || buffer[c] == '\r') || (i == 0 && buffer[c] == ' ')) {
				if(i > 0) {
					jerk_quotes[jerk_j++][i+1] = '\0';
					i=0;
				}
				++n;
				continue;
			}
			
			jerk_quotes[jerk_j][i++] = buffer[c];
		}
	}

	close(fd);

	return;
}
