#include <winsock2.h>
#include "quickjs-debugger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


struct js_transport_data {
    SOCKET handle;
} js_transport_data;

static int js_transport_read(void *udata, char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle == INVALID_SOCKET)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL)
        return -3;

	int ret = recv( data->handle, (void*)buffer, length, 0);

    if (ret == SOCKET_ERROR )
        return -4;

    if (ret == 0)
        return -5;

    if (ret > length)
        return -6;

    return ret;
}

static int js_transport_write(void *udata, const char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle == INVALID_SOCKET)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL) {
        return -3;
	}

	int ret = send( data->handle, (const void *) buffer, length, 0);
    if (ret <= 0 || ret >  length)
        return -4;

    return ret;
}

static int js_transport_peek(void *udata) {
    WSAPOLLFD  fds[1];
    int poll_rc;

    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle == INVALID_SOCKET)
        return -1;

    fds[0].fd = data->handle;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    poll_rc = WSAPoll(fds, 1, 0);
    if (poll_rc < 0)
        return -2;
    if (poll_rc > 1)
        return -3;
    // no data
    if (poll_rc == 0)
        return 0;
    // has data
    return 1;
}

static void js_transport_close(JSRuntime* rt, void* udata) {
    struct js_transport_data* data = (struct js_transport_data*)udata;
    if (data->handle == INVALID_SOCKET)
        return;
    closesocket(data->handle);
    data->handle = 0;
    free(udata);
    WSACleanup();
}

// todo: fixup asserts to return errors.
static struct sockaddr_in js_debugger_parse_sockaddr(const char* address) {
    char* port_string = strstr(address, ":");
    assert(port_string);

    int port = atoi(port_string + 1);
    assert(port);

    char host_string[256];
    strcpy(host_string, address);
    host_string[port_string - address] = 0;

    struct hostent* host = gethostbyname(host_string);
    assert(host);
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy((char*)&addr.sin_addr.s_addr, (char*)host->h_addr, host->h_length);
    addr.sin_port = htons(port);

    return addr;
}

void js_debugger_connect(JSContext *ctx, const char *address) {

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
    struct sockaddr_in addr = js_debugger_parse_sockaddr(address);
    SOCKET client = socket(AF_INET, SOCK_STREAM, 0);

	connect(client, (const struct sockaddr *)&addr, sizeof(addr));
	    
    struct js_transport_data *data = (struct js_transport_data *)malloc(sizeof(struct js_transport_data));
    data->handle = client;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}

void js_debugger_wait_connection(JSContext* ctx, const char* address) {
    struct sockaddr_in addr = js_debugger_parse_sockaddr(address); 

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    assert(server != INVALID_SOCKET);

    int reuseAddress = 1;
    assert(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAddress, sizeof(reuseAddress)) >= 0);

    assert(bind(server, (struct sockaddr*)&addr, sizeof(addr)) >= 0);

    listen(server, 1);

    struct sockaddr_in client_addr;
    int client_addr_size = sizeof(addr);
    SOCKET client = accept(server, (struct sockaddr*)&client_addr, &client_addr_size);
    closesocket(server);
    assert(client != INVALID_SOCKET);

    struct js_transport_data* data = (struct js_transport_data*)malloc(sizeof(struct js_transport_data));
    memset(data, 0, sizeof(js_transport_data));
    data->handle = client;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}