build:
	gcc chat.c -o chat -lrt -pthread

test:
	valgrind --track-origins=yes --leak-check=yes ./chat -i -k test receive.txt