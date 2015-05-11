CC=gcc
CFLAGS=-W -Wall -O2
LDFLAGS=
mobyt=sendsms2mobyt
smsmode=sendsms2smsmode

all: $(mobyt) $(smsmode)

sendsms2mobyt: sendsms2mobyt.c
	$(CC) -o $@ $< $(CFLAGS)
	strip $(mobyt)

sendsms2smsmode: sendsms2smsmode.c
	$(CC) -o $@ $< $(CFLAGS)
	strip $(smsmode)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
