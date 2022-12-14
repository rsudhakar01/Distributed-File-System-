CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := client.c server.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS} run

${PROGS} : % : %.o Makefile
	${CC} $< -o $@ udp.c

run: libmfs.so
	gcc -o mkfs mkfs.c -Wall

libmfs.so: libmfs.o
	gcc -shared -Wl,-soname,libmfs.so -o libmfs.so libmfs.o -lc	

libmfs.o: libmfs.c
	gcc -fPIC -g -c -Wall libmfs.c

clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<
