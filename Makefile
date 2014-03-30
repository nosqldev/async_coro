all: ev.o acoro.o echo_server echo_client infinite_connector chat_server coro_in_coro bg_worker echo_server2 \
	 sem_pingpong libacoro.a

CFLAGS=-Wall -Wextra -std=c99 -g -D_GNU_SOURCE

ev.o:
	cc -g -c vendor/ev.c -o example/ev.o

acoro.o:
	cc $(CFLAGS) -c -Ivendor src/acoro.c -o example/acoro.o

echo_server:
	cc $(CFLAGS) example/echo_server.c example/acoro.o example/ev.o -o example/echo_server -Isrc -Ivendor/ -lpthread -lm

echo_client:
	cc $(CFLAGS) example/echo_client.c example/acoro.o example/ev.o -o example/echo_client -Isrc -Ivendor/ -lpthread -lm

infinite_connector:
	cc $(CFLAGS) example/infinite_connector.c example/acoro.o example/ev.o -o example/infinite_connector -Isrc -Ivendor/ -lpthread -lm

chat_server:
	cc $(CFLAGS) example/chat_server.c example/acoro.o example/ev.o -o example/chat_server -Isrc -Ivendor/ -lpthread -lm

coro_in_coro:
	cc $(CFLAGS) example/coro_in_coro.c example/acoro.o example/ev.o -o example/coro_in_coro -Isrc -Ivendor/ -lpthread -lm

bg_worker:
	cc $(CFLAGS) example/bg_worker.c example/acoro.o example/ev.o -o example/bg_worker -Isrc -Ivendor/ -lpthread -lm

echo_server2:
	cc $(CFLAGS) example/echo_server2.c example/acoro.o example/ev.o -o example/echo_server2 -Isrc -Ivendor/ -lpthread -lm

sem_pingpong:
	cc $(CFLAGS) example/sem_pingpong.c example/acoro.o example/ev.o -o example/sem_pingpong -Isrc -Ivendor/ -lpthread -lm

libacoro.a:
	ar r example/libacoro.a example/ev.o example/acoro.o

clean:
	- rm -f src/*.o ev/ev.o example/echo_server example/*.o example/core.* example/*.a example/echo_client example/infinite_connector \
		example/chat_server example/coro_in_coro example/bg_worker example/echo_server2 example/sem_pingpong example/libacoro.a
