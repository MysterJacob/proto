NAME=two_way
default: heater.c controller.c
	CONFIG=examples/$(NAME)/config.cfg make -C ../.. -f makefile
	cp ../../bin/lib -r .
	gcc heater.c lib/proto.ar -o heater.o
	gcc controller.c lib/proto.ar -o controller.o
