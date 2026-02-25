NAME=readme
default:
	CONFIG=examples/$(NAME)/config.cfg make -C ../.. -f makefile
	cp ../../bin/lib -r .
	gcc main.c lib/proto.ar -o $(NAME).o
