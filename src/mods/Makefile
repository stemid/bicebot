MODS_SRCS=$(wildcard *.c)
MODS_OBJS=$(MODS_SRCS:%.c=%.o)
MODS_TRGT=$(MODS_OBJS:%.o=%.so)
LDFLAGS+=-L../..

all: $(MODS_TRGT)

$(MODS_OBJS): $(MODS_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c $(@:%.o=%.c)

$(MODS_TRGT): $(MODS_OBJS)
	$(LD) $(LDFLAGS) $(LIBS) -shared -soname $@ -o ../../$(MODULES)/$@ -lc $(@:%.so=%.o)
