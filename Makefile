# Makefile for tcc

headers = \
	c/cuncuno.h \
	c/cuncuno/core.h \
	c/cuncuno/cuncuno.h \
	c/cuncuno/entity.h \
	c/cuncuno/learn.h \
	c/cuncuno/io.h \
	c/cuncuno/tree.h \
	c/stackiter/chooser.h \
	c/stackiter/loader.h \
	c/stackiter/stackiter-learner.h \
	c/stackiter/state.h

sources = \
	c/cuncuno/core.c \
	c/cuncuno/entity.c \
	c/cuncuno/io.c \
	c/cuncuno/learn.c \
	c/cuncuno/tree.c \
	c/stackiter/chooser.c \
	c/stackiter/loader.c \
	c/stackiter/stackiter-learner.c \
	c/stackiter/state.c

build: temp/stackiter-test

clean:
	rm temp/stackiter-test

temp:
	mkdir temp

temp/stackiter-test: temp $(sources) $(headers)
	tcc -Wall -Ic -o temp/stackiter-test $(sources) -lm

test: temp/stackiter-test
	temp/stackiter-test temp/stackiter-20101105-171808-671_drop-from-25.log
