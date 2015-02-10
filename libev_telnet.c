/*
 *  Echo server
 *  It will be a part of clinoise
 *
 *  Copyright (c) 2015 by Credo Semiconductor (Shanghai) Inc.
 *  Written by Shanjin Yang <sjyang@credosemi.com>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution
 */

#include <ev.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BUFFER_SIZE 100
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	char buffer[BUFFER_SIZE];
	ssize_t read_len;

	bzero(buffer, BUFFER_SIZE);
	if (EV_ERROR & revents) {
		fprintf(stderr, "error event in read callback\n");
		return;
	}
	/* server recv, read stream to buffer */
	read_len = recv(watcher->fd, buffer, BUFFER_SIZE, 0);
	if (read_len < 0) {
		fprintf(stderr, "read error\n");
		return;
	}

	if (read_len == 0) {
		close(watcher->fd);
		ev_io_stop(loop, watcher);
		fprintf(stderr, "client disconnected.\n");
	}
	else {
		fprintf(stderr, "%s", buffer);
	}

	/* sever send to client */
	send(watcher->fd, buffer, read_len, 0);
	bzero(buffer, read_len);
}

static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	int client_fd;
	struct sockaddr_in client_addr;
	struct ev_io *client_watcher;
	socklen_t client_len;

	if (EV_ERROR & revents) {
		fprintf(stderr, "error event in accept\n");
		return;
	}

	client_len = sizeof(client_addr);
	if ((client_fd = accept(watcher->fd, (struct sockaddr*) &client_addr,
				&client_len))) {
		fprintf(stderr, "accepted connection\n");
	}
	else {
		fprintf(stderr, "accept error\n");
	}

	client_watcher = (struct ev_io*) malloc(sizeof(struct ev_io));
	assert(client_watcher);//XXX

	/*Register read event*/
	ev_io_init(client_watcher, read_cb, client_fd, EV_READ);
	ev_io_start(loop, client_watcher);
}

int main(void)
{
	int fd;
	struct sockaddr_in addr;
	int port;
	struct ev_loop *loop;
	struct ev_io *accept_watcher;
	int on = 1;
	int addr_len   = sizeof(addr);

	accept_watcher = (struct ev_io*)malloc(sizeof(struct ev_io));

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("socket");

	port = 8000;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(fd, SOMAXCONN) < 0) {
		perror("listen");
		return -1;
	}

	fprintf(stderr, "Listening on port %d\n", port);

	loop = ev_default_loop(0);
	ev_io_init(accept_watcher, accept_cb, fd, EV_READ);
	ev_io_start(loop, accept_watcher);

	ev_run(loop, 0);
	return 0;
}
