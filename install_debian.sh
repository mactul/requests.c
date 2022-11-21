sudo apt-get install openssl, openssl-dev

gcc -o test test.c -I/openssl/* requests.c easy_tcp_tls.c -lcrypto -lssl

./test