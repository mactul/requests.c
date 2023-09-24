CFLAGS_RELEASE = -Wall -Wextra -pedantic -O3 -DNDEBUG -I ./
CFLAGS_DEBUG = -Wall -Wextra -pedantic -g -DDEBUG -I ./
COMPILE = gcc -c $^ -o $@ $(CFLAGS)
STATIC = ar cr $@ $^


all: release

clean:
	rm -rf bin/*
	rm -rf temp_bin/*

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: bin/debug/test

release: CFLAGS = $(CFLAGS_RELEASE)
release: bin/release/test


bin/%/test: temp_bin/test.o bin/%/requests.a
	gcc -o $@ $^ -lssl -lcrypto $(CFLAGS)

bin/%/requests.a: temp_bin/requests.o temp_bin/utils.o temp_bin/easy_tcp_tls.o temp_bin/parser_tree.o
	@mkdir -p $(@D)
	$(STATIC)

temp_bin/%.o: %.c
	@mkdir -p $(@D)
	$(COMPILE)

.PRECIOUS: bin/%/requests.a