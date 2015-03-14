#define HOST_NAME_MAX 255
#define SERVER_CMD_MAX 8

/* this depends on the irc-server configuration */
#define NICKLEN 24
    
/* rfc 1459 says this can be 200 characters but freenode has it set to 
 * 30 including null padding while hybrid and many other servers has it 
 * set to 50 including padding by default */
#define CHANLEN 30

/* rfc defines this to 512 bytes */
#define MSG_MAX 512

#define CMDPREFIX "!"

#define EVENTS_MAX 8

/* events that the modules can react on */
#define EVENT_NULL      0x0 /* i dunno lol */
#define EVENT_PREPROC   0x1
#define EVENT_CONNECTED 0x2
#define EVENT_MSG       0x3
#define EVENT_QUIT      0x4
#define EVENT_PART      0x5
#define EVENT_JOIN      0x6
#define EVENT_TOPIC     0x7
#define EVENT_BOTCMD	0x8

struct from {
	char nickname[NICKLEN];
	char username[NICKLEN];
	char hostname[HOST_NAME_MAX+1];
};

struct event {
	int type;
	struct from sender;
	char srvcmd[SERVER_CMD_MAX];
	char to[CHANLEN];
	char msg[MSG_MAX-2];
	char channel[CHANLEN];
	char servername[HOST_NAME_MAX+1];
};

/* function prototypes */
void parse_line(struct event *, char *, int);
int irccmd(int, char *, ...);
void dump_events(struct event);

