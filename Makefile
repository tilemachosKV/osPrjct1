build:
	gcc chat.c -o chat

run:
	./chat

test:
	valgrind --leak-check=full ./chat