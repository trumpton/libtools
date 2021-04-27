#
#
#

LIBRARY := libtools.a
LIBDBG := libtools-dbg.a

SOURCES := src/httpd.c src/str.c src/log.c src/mem.c src/mdns.c src/rdata.c 

#
#
#

OBJECTS := ${SOURCES:.c=.o}
DBGOBJS := ${SOURCES:.c=.d}

default: ${LIBRARY}

all: ${LIBRARY} ${LIBDBG}

debug: ${LIBDBG}

clean: 
	/bin/rm -f ${LIBRARY} ${LIBDBG} ${OBJECTS} ${DBGOBJS}


${LIBRARY}: ${OBJECTS}
	ar -rcs $@ $^

${LIBDBG}: ${DBGOBJS}
	ar -rcs $@ $^

%.o : %.c
	gcc -c -o $@ $^

%.d : %.c
	gcc -g -D DEBUG -c -o $@ $^

%.c : %.h

