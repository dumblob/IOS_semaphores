# make [  | all | debug | submit | depend | clean ]

# .h header files
HDR = args.h
# .c source files
SRC = projekt2.c args.c
# executable name
EXE = barbers
# basename for the submission file
SUB = xpacne00# NO TRAILING SPACES
# compiler
CC = gcc
#CC = i486-mingw32-gcc
CFLAGS = -std=gnu99 -W -Wall -Wextra -Werror -pedantic #-lm

OBJ = ${SRC:.c=.o}

all : ${EXE}

debug : ${OBJ}
	${CC} ${CFLAGS} -g -o ${EXE} ${OBJ}
	#-DDEBUG

${EXE} : ${OBJ}
	${CC} ${CFLAGS} -O2 -o $@ ${OBJ}

submit :
	#tar cvzf ${SUB}.tar.gz ${SRC} ${HDR} makefile
	zip -q ${SUB}.zip -9 ${SRC} ${HDR} Makefile

depends depend :
	${CC} -E -MM ${SRC} > depends

clean :
	rm -f ${OBJ} ${EXE} ${SUB}.tar.gz *~
	#rm -f ${OBJ} ${EXE} ${SUB}.tar.gz *~ depends

.SUFFIXES : .o .c

.c.o :
	${CC} ${CFLAGS} -c $<

#include depends
