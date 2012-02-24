all: ev.o acoro.o echo_server echo_client infinite_connector

ev.o:
	cc -g -c vendor/ev.c -o example/ev.o

acoro.o:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE -c -Ivendor src/acoro.c -o example/acoro.o

echo_server:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/echo_server.c example/acoro.o example/ev.o -o example/echo_server -Isrc -Ivendor/ -lpthread -lm

echo_client:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/echo_client.c example/acoro.o example/ev.o -o example/echo_client -Isrc -Ivendor/ -lpthread -lm

infinite_connector:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/infinite_connector.c example/acoro.o example/ev.o -o example/infinite_connector -Isrc -Ivendor/ -lpthread -lm

clean:
	- rm -f src/*.o ev/ev.o example/echo_server example/*.o example/core.* example/echo_client example/infinite_connector
