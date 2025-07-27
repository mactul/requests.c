#ifndef RH_EASY_TCP_TLS_H
    #define RH_EASY_TCP_TLS_H
    #include <stdbool.h>

    #define rh_ADDRSTRLEN 22

    typedef struct _rh_client_data {
        char ip[rh_ADDRSTRLEN];
        uint16_t port;
    } rh_ClientData;

    typedef struct _rh_socket_handler rh_SocketHandler;

    #ifdef __cplusplus
    extern "C"{
    #endif

    /**
     * @brief Internal function used to init the sockets for windows
     */
    void _rh_socket_start(void);

    /**
     * @brief Internal function that destroy all sockets on windows
     */
    void _rh_socket_cleanup(void);

    /**
     * @brief This function works like socket_client_init, but it will create an ssl secured socket connection.
     * 
     * @param server_hostname the targeted server ip, formatted like "127.0.0.1", like "2001:0db8:85a3:0000:0000:8a2e:0370:7334" or like "example.com"
     * @param server_port the opened server port that listen the connection
     * @return - when it succeeds, it returns a pointer to a structure handler.
     * @return - when it fails, it returns `NULL` and `rh_print_last_error()` can tell what happened
     */
    rh_SocketHandler* rh_socket_ssl_client_init(const char* server_hostname, uint16_t server_port);


    /**
     * @brief This function will create the socket and returns a socket handler.
     * 
     * @param server_hostname the targeted server host name, formatted like "127.0.0.1", like "2001:0db8:85a3:0000:0000:8a2e:0370:7334" or like "example.com"
     * @param server_port the opened server port that listen the connection
     * @return - when it succeeds, it returns a pointer to a structure handler.
     * @return - when it fails, it returns `NULL` and `rh_print_last_error()` can tell what happened
     */
    rh_SocketHandler* rh_socket_client_init(const char* server_hostname, uint16_t server_port);

    /**
     * @brief This function will send the data contained in the buffer array through the socket
     * 
     * @param s a pointer to a SocketHandler. If you are in a client application, it's the handler returned by `rh_socket_client_init` or `rh_socket_ssl_client_init`. If you are in a server application, it's the handler returned by `rh_socket_accept`
     * @param buffer a buffer containing all the data you want to send
     * @param n the size of the data, this can be different from the `sizeof(buffer)` if your buffer isn't full.
     * @return - when it succeeds, it returns the number of bytes sended
     * @return - when it fails, it returns -1 and errno contains more information.
     */
    ssize_t rh_socket_send(rh_SocketHandler* s, const char* buffer, size_t n);

    /**
     * @brief This function will wait for data to arrive in the socket and fill a buffer with them.
     * 
     * @param s a pointer to a SocketHandler. If you are in a client application, it's the handler returned by `rh_socket_client_init` or `rh_socket_ssl_client_init`. If you are in a server application, it's the handler returned by `rh_socket_accept`.
     * @param buffer an empty buffer that will be filled with data from the socket.
     * @param n the size of your buffer, you can simply provide `sizeof(buffer)`.
     * @return - when it succeeds, it returns the number of bytes read
     * @return - when it fails, it returns -1 and errno contains more information.
     */
    ssize_t rh_socket_recv(rh_SocketHandler* s, char* buffer, size_t n);


    /**
     * @brief This function take the address of the pointer on the handler to release all the stuff and put the rh_SocketHandler pointer to NULL.
     * 
     * @param pps the address of the pointer on the socket
     */
    void rh_socket_close(rh_SocketHandler** pps);


    /**
     * @brief Change a number from little endian to big endian or from big endian to little endian but only if the machine is little-endian. Otherwise, it do nothing.
     * 
     * @param input the number to convert
     * @return the number converted
     */
    uint64_t rh_socket_ntoh64(uint64_t input);

    #ifdef __cplusplus
    }
    #endif
#endif