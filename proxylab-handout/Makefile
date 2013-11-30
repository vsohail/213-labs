#
# Makefile for proxy lab. 
#
# You may modify is file any way you like (except for the handin
# rule). Autolab will execute the command "make" on your specific 
# Makefile to build your proxy from sources.
#
CC = gcc
CFLAGS = -g -Wall -Werror
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c $(LDFLAGS)

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c $(LDFLAGS)

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can hand in to
# Autolab. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude .proxy --exclude .noproxy --exclude driver.sh --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh)

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

