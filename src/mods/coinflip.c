#include <stdlib.h>
#include <time.h>

#include "ircbot.h"
#include "bicebot.h"

int coinflip(int, struct event);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 },
	&coinflip,
	"flip",
	"Coin Flip",
	"Flip a coin",
	"nocturnal [at] swehack [dot] se",
	"0.1",
};

int coinflip(int sd, struct event events) {
	int getpid();
	time_t now;

	now = time(&now) / random();
	srandom(getpid() + (int)((now >> 16) + now + time(&now)));

	if(events.to[0] != '#') {
		irccmd(sd, "PRIVMSG %s :%s", events.sender.nickname, random() % 2 ? "heads" : "tails");
	} else {
		irccmd(sd, "PRIVMSG %s :%s", events.to, random() % 2 ? "heads" : "tails");
	}

	return(0);
}
