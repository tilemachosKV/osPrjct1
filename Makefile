build:
	gcc chat.c -o chat -lrt -pthread

run:
	./chat

test:
	valgrind --track-origins=yes --leak-check=yes ./chat -i -k test receive.log