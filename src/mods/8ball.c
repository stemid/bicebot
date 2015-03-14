#include <stdlib.h>
#include <time.h>

#include "ircbot.h"
#include "bicebot.h"

int eightball(int, struct event);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 },
	&eightball,
	"8ball",
	"Magic 8ball",
	"Ask the magic eight ball a question",
	"nocturnal [at] swehack [dot] se",
	"0.1"
};

#define SZ(a) sizeof(a) / sizeof(char *)

static char *answers[] = {
	"As I see it, yes", 
	"Ask again later", 
	"Better not tell you now", 
	"Cannot predict now", 
	"Concentrate and ask again", 
	"Don't count on it", 
	"It is certain", 
	"It is decidedly so", 
	"Most likely", 
	"My reply is no", 
	"My sources say no", 
	"Outlook good", 
	"Outlook not so good", 
	"Reply hazy, try again", 
	"Signs point to yes", 
	"Very doubtful", 
	"Without a doubt", 
	"Yes", 
	"Yes - definitely", 
	"You may rely on it", 
};

int eightball(int sd, struct event events) {
	int getpid();
	time_t now;

	now = time(&now) / random();
	srandom(getpid() + (int)((now >> 16) + now + time(&now)));

	if(events.to[0] != '#') {
		irccmd(sd, "PRIVMSG %s :%s", events.sender.nickname, answers[random() % SZ(answers)]);
	} else {
		irccmd(sd, "PRIVMSG %s :%s", events.to, answers[random() % SZ(answers)]);
	}

	return(0);
}
