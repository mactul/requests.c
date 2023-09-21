CFLAGS = -Wall -Wextra -pedantic -g -I ./
COMPILE = gcc -c $^ -o $@ $(CFLAGS)
STATIC = ar cr $@ $^

clean:
	rm -rf bin/*
	rm -rf lib/*

test: bin/test
	rm test
	ln $^ $@

bin/test: lib/test.o bin/requests.a
	gcc -o $@ $^ -lssl -lcrypto $(CFLAGS)

bin/requests.a: lib/requests.o lib/utils.o lib/easy_tcp_tls.o lib/parser_tree.o
	@mkdir -p $(@D)
	$(STATIC)

lib/%.o: %.c
	@mkdir -p $(@D)
	$(COMPILE)