
targets := msq_talker server client port

all: $(targets)

msq_talker: src/sysv/msq_talker.c
	$(CC) -g $< -o $@

server: src/socket/server.c
	$(CC) -g $< -o $@

client: src/socket/client.c
	$(CC) -g $< -o $@

port: src/socket/port.c
	$(CC) -g $< -o $@

clean:
	$(RM) -r $(targets)

.PHONY: all clean