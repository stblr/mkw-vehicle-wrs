CFLAGS = -Wall -Wextra -Wpedantic -O3
LIBS = -lcurl -ljson-c

all: mkw-vehicle-wrs

mkw-vehicle-wrs: main.c
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
