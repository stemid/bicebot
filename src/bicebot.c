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

/* bicebot - a simple ircbot 
 * by nocturnal [at] swehack [dot] se */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>

#include <ircbot.h>
#include <bicebot.h>

int *psocket = NULL;
unsigned int debug = 0, joined_channel = 0;
char nickname[NICKLEN], username[NICKLEN], channel[CHANLEN], ip[16], 
	 hostname[HOST_NAME_MAX+1], local_hostname[HOST_NAME_MAX+1];

struct mod **modules = NULL;
unsigned int num_modules = 0;

int main(int argc, char **argv) {
	int argch, i;
	unsigned int port = 6667;
	pid_t pid;

	int sd;
	int setsockoptval = 1;
	struct sockaddr_in sin;
	struct protoent *proto;

	int sockd, sets, maxfds, r = 0, s = 0;
	struct sockaddr csad;

	struct timeval timeout;
	fd_set rfds;
	fd_set rfds_bak;
	fd_set wfds;
	fd_set wfds_bak;

	char buf[MSG_MAX];
	char tmpbuf[MSG_MAX];
	socklen_t csalen;

	struct sigaction saction;

	/* irc related */
	struct event events;

	/* end of variable declarations */

	FD_ZERO(&rfds);
	FD_ZERO(&rfds_bak);
	FD_ZERO(&wfds);
	FD_ZERO(&wfds_bak);

	events.type = EVENT_NULL;

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	csalen = sizeof(csad);

	saction.sa_flags = SA_NOCLDWAIT;
	saction.__sigaction_u.__sa_handler = NULL;
	saction.__sigaction_u.__sa_sigaction = NULL;

	if(argc < 2) {
		usage(argv[0]);
		exit(-1);
	}
	
	while((argch = getopt(argc, argv, "vhdu:n:c:p:")) != -1) {
		switch(argch) {
			case 'c':
				if(strlen(optarg) < CHANLEN) {
					strncpy(channel, optarg, CHANLEN-1);
				} else {
					usage(argv[0]);
					exit(-1);
				}
				break;
			case 'p':
				if(strlen(optarg) > 0) {
					port = (unsigned int)atoi(optarg);
					if(port > 65535) {
						usage(argv[0]);
						exit(-1);
					}
				}
				break;
			case 'n':
				if(strlen(optarg) < NICKLEN) {
					strncpy(nickname, optarg, NICKLEN);
				} else {
					usage(argv[0]);
					exit(-1);
				}
				break;
			case 'u':
				if(strlen(optarg) < NICKLEN) {
					strncpy(username, optarg, NICKLEN);
				} else {
					usage(argv[0]);
					exit(-1);
				}
				break;
			case 'd':
				debug = 1;
				break;
			case 'h':
				usage(argv[0]);
				exit(-1);
			case 'v':
				printf("%s v%s by nocturnal [at] swehack [dot] se\n", APP_NAME, APP_VERSION);
				exit(-1);
			default:
				usage(argv[0]);
				exit(-1);
		}
	}

	if(strlen(nickname) <= 0) {
		usage(argv[0]);
		exit(-1);
	}

	if(argc-1 == optind) {
		if(strlen(argv[optind]) < 255) {
			strncpy(hostname, argv[optind], 255);
		} else {
			usage(argv[0]);
			exit(-1);
		}
	}
	
	if(argc-1 == optind) {
		struct hostent *hp = gethostbyname(hostname);
		if(hp == NULL) {
			fprintf(stderr, "gethostbyname: %s\n", hstrerror(h_errno));
			free(hp);
			exit(-1);
		}
		strncpy(ip, inet_ntoa(*(struct in_addr *)(hp->h_addr_list[0])), 16);
	} else {
		usage(argv[0]);
		exit(-1);
	}

	if(gethostname(local_hostname, HOST_NAME_MAX+1) == -1) {
		perror("gethostname");
		exit(-1);
	}

	if(strlen(username) <= 0) {
		strncpy(username, nickname, NICKLEN);
	}

	/* done reading arguments */

	if(init_mods() == -1) {
		exit(-1);
	}

	if(debug == 0) {
		if((pid = fork()) > 0) {
			fprintf(stderr, "Daemonizing... pid: %d\n", pid);
			exit(0);
		}
	}

	if((proto = getprotobyname("tcp")) == NULL) {
		perror("getprotobyname");
		exit(-1);
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if(argc-1 == optind) {
		if((sin.sin_addr.s_addr = inet_addr(ip)) == -1) {
			fprintf(stderr, "inet_addr: conversion failed\n");
			exit(-1);
		}
	} else {
		sin.sin_addr.s_addr = INADDR_ANY;
	}

	if((sd = socket(AF_INET, SOCK_STREAM, proto->p_proto)) == -1) {
		perror("socket");
		exit(-1);
	}

	psocket = &sd;

	atexit(close_conn);

	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &setsockoptval, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(-1);
	}

	if(connect(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("socket");
		exit(-1);
	}

	/* done making connection ready */

	if(signal(SIGINT, clean_exit) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	if(signal(SIGUSR1, reinit_mods) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	/* AHH! ZOMBIES!! */
	if(sigaction(SIGCHLD, &saction, 0) == -1) {
		perror("sigaction");
		exit(-1);
	}

	sockd = sd;
	psocket = &sockd;

	FD_SET(sockd, &rfds);
	FD_SET(sockd, &wfds);

	maxfds = sockd;

	if(irccmd(sockd, "NICK %s", nickname) == -1 || irccmd(sockd, "USER %s %s %s :%s", username, local_hostname, hostname, username) == -1) {
		perror("irccmd");
		clean_exit(-1);
	}

	signal_emit(EVENT_PREPROC, events);

	for(;;) {
		s = 0;
		r = 0;
		rfds_bak = rfds;
		wfds_bak = wfds;

		if((sets = select(maxfds+1, &rfds_bak, NULL, NULL, NULL)) == -1) {
			if(errno && errno != EINTR) {
				perror("select");
				exit(-1);
			}
		}

		signal_emit(EVENT_CONNECTED, events);

		if(FD_ISSET(sockd, &rfds_bak)) {
			if(joined_channel == 0) {
				if(irccmd(sockd, "JOIN %s", channel) == -1) {
					perror("irccmd");
					exit(-1);
				} else {
					joined_channel = 1;
				}
			}
			
			if((r = read(sockd, buf, MSG_MAX)) < 0) {
				perror("read");
				exit(-1);
			} else if(r == 0) {
				fprintf(stderr, "read error: connection reset by peer\n");
				exit(-1);
			} else {
				if(debug == 1) {
					if((s = write(1, buf, r)) == -1) {
						perror("write");
						exit(-1);
					}
				}

				if(*buf == ':') {
					parse_line(&events, buf, r);
					strncpy(events.servername, hostname, HOST_NAME_MAX);
					strncpy(events.channel, channel, CHANLEN-1);

					if(debug == 1) {
						dump_events(events);
					}

					if(events.type != EVENT_NULL) {
						signal_emit(events.type, events);
					}

					memset(&events, 0, sizeof(struct event));
				} else { /* we don't need to react on protocol commands */
					for(i=0;*(buf+i) != ' ';i++) {
						tmpbuf[i] = *(buf+i);
					}
					tmpbuf[i+1] = '\0';

					if(strncmp(tmpbuf, "PING", i) == 0) {
						irccmd(sd, "PONG %s", hostname);
					}
					memset(tmpbuf, '\0', sizeof(tmpbuf));
				}

				memset(buf, '\0', sizeof(buf));
			} /* end of event code */
		}
	}

	close(*psocket);
	exit(0);
}

void signal_emit(int signal, struct event events) {
	int i, j, mi, err; /* err is last known returned error code from modules */
	char cmd[SERVER_CMD_MAX];
	/*char arg[MSG_MAX];*/
	char *msg = events.msg;

	/* TODO: this could be more elegant with the new event handler 
	 * all this code needs to be fixed actually, i should create a 
	 * new event for bot commands so that modules can capture regular 
	 * text events if they want to look for stuff for example */
	
	/* handle bot commands */
	if(*msg == '!' && events.type == EVENT_BOTCMD) {
		for(i=0,mi=1;*(msg+mi) != ' ' && i < SERVER_CMD_MAX-1 && mi < MSG_MAX-2;i++,mi++) {
			cmd[i] = *(msg+mi);
		}
		cmd[i] = '\0';

		/* get command arguments */
		/* TODO: i dunno lol */
		/*if(strlen(msg) > strlen(cmd)+1) {
			for(i=0,mi++;*(msg+mi) != '\0' && mi < MSG_MAX;i++,mi++) {
				arg[i] = *(msg+mi);
			}
			arg[i] = '\0';
		}*/

		for(i=0;i<num_modules;++i) {
			if(!strncmp(cmd, modules[i]->m_cmd, mi)) {
				err = (*modules[i]->m_func)(*psocket, events);
				return;
			}
		}

		/* !mods - show modules */
		if(strncmp(cmd, "mods", mi) == 0) {
			for(i=0;i<num_modules;++i) {
				irccmd(*psocket, "PRIVMSG %s :!%s - %s %s, %s; by %s", events.sender.nickname, modules[i]->m_cmd, modules[i]->m_name, modules[i]->m_version, modules[i]->m_info, modules[i]->m_author);
			}
		}

		/* !help - manual */
		if(strncmp(cmd, "help", mi) == 0) {
			irccmd(*psocket, "PRIVMSG %s :My builtin commands are: !help, !mods (sends to query)", events.to);
			return;
		}
	} else if(events.type != EVENT_NULL) {
		for(i=0;i<num_modules;++i) {
			for(j=0;modules[i]->events[j] != 0 && j < EVENTS_MAX;++j) {
				if(events.type == modules[i]->events[j]) {
					err = (*modules[i]->m_func)(*psocket, events);
				}
			}
		}
	}

	return;
}

int init_mods(void) {
	int name_len;
	unsigned int alloc_count = 2;
	DIR *dp = NULL;
	struct dirent *dir = NULL;
	char modpath[256];
	void *mod = NULL;
	struct mod **tmp_modules = NULL;

	if((modules = malloc(sizeof(struct mod **)*16)) == NULL) {
		perror("malloc");
		return(-1);
	}

	if((dp = opendir(MODS_PATH)) == NULL) {
		perror("opendir");
		return(-1);
	}

	while((dir = readdir(dp)) != NULL) {
		if(dir->d_type == 8) {
			name_len = strlen(dir->d_name);
			if(dir->d_name[name_len-3] == '.' && dir->d_name[name_len-2] == 's' && dir->d_name[name_len-1] == 'o') {
				snprintf(modpath, 256, "%s/%s", MODS_PATH, dir->d_name);

				if(num_modules > 16*(alloc_count-1)) {
					if((tmp_modules = realloc(modules, sizeof(struct mod **)*(num_modules*alloc_count))) == NULL) {
						perror("realloc");
						return(-1);
					}

					++alloc_count;
					modules = NULL;
					modules = tmp_modules;
				}

				printf("Loading module: %s\n", modpath);
				if((mod = load_mod(modpath)) == NULL) {
					fprintf(stderr, "Failed loading module: %s\n", dlerror());
					return(-1);
				}

				if((modules[num_modules] = (struct mod *)dlsym(mod, "_module_info")) == NULL) {
					fprintf(stderr, "Failed locating symbol: %s\n", dlerror());
					dlclose(mod);
				} else {
					modules[num_modules]->m_ld = mod;
					++num_modules;
				}

				mod = NULL;
			}
		}
	}

	closedir(dp);

	return(0);
}

void reinit_mods(int signo) {
	int i;

	if(modules != NULL) { /* reinit_mods */
		for(i=0;i<num_modules;++i) {
			if(unload_mod(modules[i]->m_ld) == 0) {
				modules[i] = NULL;
			} else {
				fprintf(stderr, "Failed unloading module: %s\n", dlerror());
				return;
			}
		}
	}

	num_modules = 0;

	init_mods();

	return;
}

/* TODO: wrappers so we can do this with bot commands later */
void *load_mod(char *path) {
	return(dlopen(path, (RTLD_LAZY | RTLD_LOCAL)));
}
int unload_mod(void *mod_handle) {
	return(dlclose(mod_handle));
}

void clean_exit(int signo) {
	if(joined_channel == 1) {
		irccmd(*psocket, "QUIT :later fgts");
	}

	close(*psocket);

	if(debug == 1) {
		fprintf(stderr, "exiting on signal: %d\n", signo);
	}

	exit(signo);
}

void close_conn(void) {
	close(*psocket);

	return;
}

void usage(char *app_name) {
	fprintf(stderr, "Usage: %s [-hv] [-u username] [-p port] [-c #channel] <-n nickname> <host>\n", app_name);
	fprintf(stderr, 
			"\t-n nickname\t Bots nickname\n"
			"\t-u username\t Bots username (nickname by default)\n"
			"\t-c #channel\t Channel to join\n"
			"\t-p port\t\t Port to use\n"
			"\t-d\t\t Print debugging information\n"
			"\t-h\t\t Show this help text\n"
			"\t-v\t\t Show version\n");

	return;
}

