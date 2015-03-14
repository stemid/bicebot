#include <stdlib.h>

#include <ircbot.h>
#include <bicebot.h>

int version(int, struct event);

struct mod _module_info = {
	NULL,
	{ EVENT_BOTCMD, 0 },
	&version,
	"version",
	"Version",
	"Shows version info",
	"nocturnal [at] swehack [dot] se",
	"0.1"
};

int version(int sd, struct event events) {
	irccmd(sd, "PRIVMSG %s :%s v%s", events.to, APP_NAME, APP_VERSION);

	return(0);
}
