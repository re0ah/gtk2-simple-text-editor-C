CFLAGS+=-Wall -Wextra -O3
CFLAGS+=`pkg-config --cflags gtk+-2.0`
LIBS+=`pkg-config --libs gtk+-2.0`
PROG=text-editor

all:
	$(CC) $(CFLAGS) -o $(PROG) src/*.c $(LIBS)

clean:
	rm -f $(PROG)
