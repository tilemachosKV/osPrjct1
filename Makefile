build:
	gcc chat.c -o chat -lrt -pthread

run:
	./chat

test:
	valgrind --leak-check=full ./chat