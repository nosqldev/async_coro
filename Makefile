all: ev.o acoro.o echo_server echo_client infinite_connector chat_server coro_in_coro bg_worker echo_server2 libacoro.a

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

chat_server:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/chat_server.c example/acoro.o example/ev.o -o example/chat_server -Isrc -Ivendor/ -lpthread -lm

coro_in_coro:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/coro_in_coro.c example/acoro.o example/ev.o -o example/coro_in_coro -Isrc -Ivendor/ -lpthread -lm

bg_worker:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/bg_worker.c example/acoro.o example/ev.o -o example/bg_worker -Isrc -Ivendor/ -lpthread -lm

echo_server2:
	cc -Wall -Wextra -std=c99 -g -D_GNU_SOURCE example/echo_server2.c example/acoro.o example/ev.o -o example/echo_server2 -Isrc -Ivendor/ -lpthread -lm

libacoro.a:
	ar r example/libacoro.a example/ev.o example/acoro.o

clean:
	- rm -f src/*.o ev/ev.o example/echo_server example/*.o example/core.* example/*.a example/echo_client example/infinite_connector example/chat_server example/coro_in_coro example/bg_worker example/echo_server2
