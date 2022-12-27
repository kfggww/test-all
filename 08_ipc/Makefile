
targets := msq_talker server client

all: $(targets)

msq_talker: src/sysv/msq_talker.c
	$(CC) -g $< -o $@

server: src/socket/server.c
	$(CC) -g $< -o $@

client: src/socket/client.c
	$(CC) -g $< -o $@

clean:
	$(RM) -r $(targets)

.PHONY: all clean