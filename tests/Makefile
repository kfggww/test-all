
all: server client

server: test-epoll-server.c
	$(CC) $< -lrt -o $@

client: test-epoll-client.c
	$(CC) $< -lrt -o $@

clean:
	rm -rf server client

clean_mq:
	rm -rf /dev/mqueue/*

.PHONY: all clean