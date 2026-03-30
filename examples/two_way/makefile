NAME=two_way
default: heater.c controller.c
	CONFIG=examples/$(NAME)/config.cfg make -C ../.. -f makefile
	cp ../../bin/lib -r .
	gcc heater.c lib/libproto.a -o heater.o
	gcc controller.c lib/libproto.a -o controller.o
