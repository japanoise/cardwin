CFLAGS=$(shell pkg-config --cflags gtk+-2.0)
LDFLAGS=$(shell pkg-config --libs gtk+-2.0)
PROGNAME=cardwin
OBJECTS=cardwin.o

all: $(PROGNAME)

debug: CFLAGS+=-g -O0
debug: $(PROGNAME)

$(PROGNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

crd2json: crd2json.o data.o
	$(CC) -o $@ $^ $(LDFLAGS)

savetest: savetest.o data.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf *.exe
	rm -rf $(PROGNAME)
