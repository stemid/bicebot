/* just a mod of jerkcity, made for lulz and lilz */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>

#include <ircbot.h>
#include <bicebot.h>

#define ISLAM_QUOTESFILE "islam_quotes.txt"
#define ISLAM_MAXQUOTES 512
#define ISLAM_MAXLEN 512

int islam_it(int, struct event);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 }, /* always end it with a 0 */
	&islam_it, 
	"islam", 
	"Islam quotes", 
	"Shows quotes important to islam", 
	"nocturnal [at] swehack [dot] se", 
	"0.1a" 
};

char islam_quotes[ISLAM_MAXQUOTES][ISLAM_MAXLEN];
int islam_j = 0;

int islam_it(int sd, struct event events) {
	time_t t;

	t = time(&t) / random();
	srandom(getpid() + (int)((t >> 16) + t + time(&t)));

	if(islam_j > 0) {
		irccmd(sd, "PRIVMSG %s :%s", events.to, islam_quotes[random() % islam_j]);
	}

	return(0);
}

void _init(void) {
	int fd, n = 0, c, i = 0;
	ssize_t r;
	char buffer[512];
	char quotes_path[255+255+1];

	snprintf(quotes_path, 255+255+1, "%s/%s", DATA_PATH, ISLAM_QUOTESFILE);
	
	if((fd = open(quotes_path, O_RDONLY)) == -1) {
		return;
	}

	while((r = read(fd, buffer, 512)) > 0) {
		for(c=0;c < r && i < ISLAM_MAXLEN && islam_j < ISLAM_MAXQUOTES;++c) {
			if((buffer[c] == '\n' || buffer[c] == '\r') || (i == 0 && buffer[c] == ' ')) {
				if(i > 0) {
					islam_quotes[islam_j++][i+1] = '\0';
					i=0;
				}
				++n;
				continue;
			}
			
			islam_quotes[islam_j][i++] = buffer[c];
		}
	}

	close(fd);

	return;
}
