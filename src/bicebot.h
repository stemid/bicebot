#define APP_NAME "bicebot"
#define APP_VERSION "0.6b"
#define APP_URL "http://code.google.com/p/bicebot/"

#define MODS_PATH "modules"
#define DATA_PATH "data"

/* don't even think about changing anything below this line 
 * unless you want evil monkeys to kill you in your sleep */

struct mod {
	void *m_ld;
	unsigned int events[EVENTS_MAX];
	int (*m_func)(int, struct event);
	char m_cmd[16];
	char m_name[16];
	char m_info[128];
	char m_author[64];
	char m_version[8];
};

/* function prototypes */
void signal_emit(int, struct event);
int init_mods(void);
void reinit_mods(int);
void *load_mod(char *);
int unload_mod(void *);
void close_conn(void);
void clean_exit(int);
void usage(char *);

