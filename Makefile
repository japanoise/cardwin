all:
	$(CC) -g cardwin.c -o cardwin `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`

