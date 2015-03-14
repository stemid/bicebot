#include <stdlib.h>

#include <ircbot.h>
#include <bicebot.h>

int info(int, struct event);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 },
	&info,
	"info",
	"Information",
	"Shows some information about the ircbot",
	"nocturnal [at] swehack [dot] se",
	"0.1"
};

int info(int sd, struct event events) {
	irccmd(sd, "PRIVMSG %s :I am %s v%s, an automatic program designed for the IRC-protocol. I was written in ANSI C by a nerd called nocturnal. Download me at %s", events.to, APP_NAME, APP_VERSION, APP_URL);

	return(0);
}
