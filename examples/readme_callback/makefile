NAME=readme_callback
default:
	CONFIG=examples/$(NAME)/config make -C ../.. -f makefile
	cp ../../bin/lib -r .
	gcc main.c lib/proto.ar -o $(NAME).o
