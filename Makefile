compile:
	gcc -Wall -g3 -fsanitize=address -pthread server_NT117.c -o server_NT117
	gcc -Wall -g3 -fsanitize=address -pthread client_NT117.c -o client_NT117
FLAGS    = -L /lib64
LIBS     = -lusb-1.0 -l pthread

