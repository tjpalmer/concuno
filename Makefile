# Makefile for tcc

headers = \
	c/cuncuno.h \
	c/cuncuno/core.h \
	c/cuncuno/cuncuno.h \
	c/cuncuno/io.h \
	c/stackiter/chooser.h \
	c/stackiter/loader.h \
	c/stackiter/stackiter-learner.h \
	c/stackiter/state.h

sources = \
	c/stackiter/loader.c \
	c/stackiter/stackiter-learner.c \
	c/cuncuno/core.c \
	c/cuncuno/io.c
	#c/stackiter/chooser.c \
	#c/stackiter/state.c \

test: temp/stackiter-test
	temp/stackiter-test temp/stackiter-20101105-171808-671_drop-from-25.log

temp:
	mkdir temp

temp/stackiter-test: temp $(sources)
	tcc -Wall -Ic -o temp/stackiter-test $(sources)
