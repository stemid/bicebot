bicebot: libircbot.so
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $(INCLUDES) -o bicebot $(SRC)/bicebot.c

libircbot.o:
	$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c $(SRC)/libircbot.c

libircbot.so: libircbot.o 
	$(LD) -shared -soname libircbot.so -o libircbot.so -lc libircbot.o
