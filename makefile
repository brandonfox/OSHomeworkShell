# build a program from 2 files and one shared header
CC = gcc
CCFLAGS = -pthread
FILES = icshell.c jobs.c
FILENAME = icsh

program: ${FILES}
	$(CC) ${CCFLAGS} -o ${FILENAME} $(FILES)