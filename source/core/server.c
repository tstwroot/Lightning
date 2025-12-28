/**
 * Lightning - Web Framework.
 * Copyright (C) 2025  Pablo Vitorino Panciera (tstwroot) <tstwroot@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#define LIGHTNING_MAX_CONNECTIONS 1024
#define LIGHTNING_READ_BUFFER_SIZE 8192
#define LIGHTNING_EPOLL_MAX_EVENTS 64
#define LIGHTNING_EPOLL_TIMEOUT_MS -1

#define LIGHTNING_ERROR(error_message) \
  fprintf(stderr,                      \
          "[Lightning Error]: In <%s> line %d (%s)\n", __FUNCTION__, __LINE__, error_message)

enum lightning_connection_state
{
  CONN_STATE_CLOSED = 0,
  CONN_STATE_READING_REQUEST,
  CONN_STATE_PROCESSING,
  CONN_STATE_WRITING_RESPONSE,
  CONN_STATE_CLOSING
};

struct lightning_connection
{
  struct sockaddr_in client_addr;
  char read_buffer[LIGHTNING_READ_BUFFER_SIZE];
  char *write_buffer;
  size_t read_pos;
  size_t read_total;
  size_t write_total;
  size_t write_pos;
  time_t last_activity;
  enum lightning_connection_state state;
  int fd;
};

struct lightning_server
{
  struct lightning_connection *connections;
  struct sockaddr_in address;
  int socket_fd;
  int epoll_fd;
  int max_connections;
  int active_connections;
  unsigned short port;
  bool running;
};

void lightning_connection_init(struct lightning_connection *conn, int fd, struct sockaddr_in *addr);
void lightning_connection_reset(struct lightning_connection *conn);
static void accept_new_connection(struct lightning_server *server);
static void close_connection(struct lightning_server *server, int fd);
static void handle_client_read(struct lightning_server *server, int fd);
static void handle_client_write(struct lightning_server *server, int fd);
static int is_request_complete(struct lightning_connection *conn);
static int set_socket_nonblocking(int fd);

struct lightning_server *lightning_create_server(unsigned short port, const int max_connections)
{
  struct lightning_server *server = malloc(sizeof(struct lightning_server));
  if(server == NULL)
  {
    LIGHTNING_ERROR("can not allocate struct lightning_server");
    return NULL;
  }

  server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server->socket_fd < 0)
  {
    LIGHTNING_ERROR("can not create a socket");
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  server->port = port;
  server->address.sin_family = AF_INET;
  server->address.sin_addr.s_addr = INADDR_ANY;
  server->address.sin_port = htons(server->port);
  memset(server->address.sin_zero, 0, sizeof(server->address.sin_zero));

  setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
  if(setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0)
  {
    LIGHTNING_ERROR("can not set the socketopt to SO_REUSEPORT (linux kernel > 3.9 ?)");
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  if(bind(server->socket_fd, (struct sockaddr_in *)&server->address, sizeof(struct sockaddr_in)) < 0)
  {
    LIGHTNING_ERROR("can not make bind");
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  if(listen(server->socket_fd, SOMAXCONN) < 0)
  {
    LIGHTNING_ERROR("can not listen");
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  int flags = fcntl(server->socket_fd, F_GETFL, 0);
  if(flags == -1 || fcntl(server->socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
  {
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  server->epoll_fd = epoll_create1(0);
  if(server->epoll_fd == -1)
  {
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  server->max_connections = max_connections;

  struct rlimit rl;
  if(getrlimit(RLIMIT_NOFILE, &rl) == 0)
  {
    rlim_t required_fds = max_connections + 100;
    if(rl.rlim_cur < required_fds)
    {
      rl.rlim_cur = (required_fds <= rl.rlim_max) ? required_fds : rl.rlim_max;
      if(setrlimit(RLIMIT_NOFILE, &rl) == -1)
      {
        fprintf(stderr, "Warning: Failed to set file descriptor limit to %lu. Current limit: %lu\n", required_fds, rl.rlim_cur);
      }
    }
  }

  server->connections = calloc(server->max_connections, sizeof(struct lightning_connection));
  if(server->connections == NULL)
  {
    LIGHTNING_ERROR("allocating connections array");
    close(server->epoll_fd);
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  for(int i = 0; i < server->max_connections; i++)
  {
    lightning_connection_reset(&server->connections[i]);
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = server->socket_fd;

  if(epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->socket_fd, &ev) == -1)
  {
    LIGHTNING_ERROR("epoll_ctl can not add the event");
    free(server->connections);
    close(server->epoll_fd);
    close(server->socket_fd);
    free(server);
    return NULL;
  }

  server->active_connections = 0;
  server->running = true;

  return server;
}

void *ride_the_lightning(void *args)
{
  struct lightning_server *server = (struct lightning_server *)args;

  if(server == NULL)
  {
    return NULL;
  }

  struct epoll_event events[LIGHTNING_EPOLL_MAX_EVENTS];

  while(server->running)
  {
    int fd_counter = epoll_wait(server->epoll_fd, events, LIGHTNING_EPOLL_MAX_EVENTS, LIGHTNING_EPOLL_TIMEOUT_MS);

    if(fd_counter == -1)
    {
      if(errno == EINTR)
      {
        continue;
      }
      fprintf(stderr, "epoll_wait() error: %s\n", strerror(errno));
      break;
    }

    for(int i = 0; i < fd_counter; i++)
    {
      int fd = events[i].data.fd;
      uint32_t events_mask = events[i].events;

      if(fd == server->socket_fd)
      {
        accept_new_connection(server);
      }
      else
      {
        if(events_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        {
          close_connection(server, fd);
          continue;
        }

        if(events_mask & EPOLLIN)
        {
          handle_client_read(server, fd);
          continue;
        }

        if(events_mask & EPOLLOUT)
        {
          handle_client_write(server, fd);
          continue;
        }
      }
    }
  }

  printf("Lightning say: bye...\n");
  return NULL;
}

void lightning_server_stop(struct lightning_server *server)
{
  if(server == NULL)
  {
    return;
  }

  server->running = false;
}

void lightning_destroy_server(struct lightning_server *server)
{
  if(server == NULL)
  {
    return;
  }

  server->running = false;

  for(int i = 0; i < server->max_connections; i++)
  {
    struct lightning_connection *conn = &server->connections[i];
    if(conn->fd >= 0)
    {
      close(conn->fd);
    }
    lightning_connection_reset(conn);
  }

  free(server->connections);

  if(server->epoll_fd >= 0)
  {
    close(server->epoll_fd);
  }

  if(server->socket_fd >= 0)
  {
    shutdown(server->socket_fd, SHUT_RDWR);
    close(server->socket_fd);
  }

  free(server);
}

static void accept_new_connection(struct lightning_server *server)
{
  while(1)
  {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(server->socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if(client_fd == -1)
    {
      if(errno == EAGAIN || errno == EWOULDBLOCK)
      {
        break;
      }
      else
      {
        fprintf(stderr, "accept() error: %s\n", strerror(errno));
        break;
      }
    }

    if(set_socket_nonblocking(client_fd) == -1)
    {
      fprintf(stderr, "Error: Failed to set client socket non-blocking.\n");
      close(client_fd);
      continue;
    }

    if(client_fd >= server->max_connections)
    {
      fprintf(stderr, "Error: Client fd %d exceeds max connections limit.\n", client_fd);
      char error_buffer[] = "Error.";
      send(client_fd, error_buffer, sizeof(error_buffer), 0);
      close(client_fd);
      continue;
    }

    struct lightning_connection *conn = &server->connections[client_fd];
    lightning_connection_init(conn, client_fd, &client_addr);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = client_fd;

    if(epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
      fprintf(stderr, "epoll_ctl() failed for client fd %d: %s\n", client_fd, strerror(errno));
      close(client_fd);
      lightning_connection_reset(conn);
      continue;
    }

    server->active_connections++;
  }
}

static int set_socket_nonblocking(int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags == -1)
  {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void close_connection(struct lightning_server *server, int fd)
{
  if(fd < 0 || fd >= server->max_connections)
  {
    return;
  }

  struct lightning_connection *conn = &server->connections[fd];
  if(conn->fd == -1)
  {
    return;
  }

  epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  close(fd);
  lightning_connection_reset(conn);
  server->active_connections--;
}

static void handle_client_read(struct lightning_server *server, int fd)
{
  struct lightning_connection *conn = &server->connections[fd];

  size_t remaining = LIGHTNING_READ_BUFFER_SIZE - conn->read_pos;
  if(remaining == 0)
  {
    fprintf(stderr, "Read buffer full for fd %d\n", fd);
    close_connection(server, fd);
    return;
  }

  ssize_t data_length = recv(fd, conn->read_buffer + conn->read_pos, remaining, 0);

  if(data_length > 0)
  {
    conn->read_pos += data_length;
    conn->last_activity = time(NULL);

    if(is_request_complete(conn))
    {
      const char *html_body =
          "<html><body>"
          "<h1>Lightning Web Server</h1>"
          "<p>Ride the lightning</p>"
          "</body></html>";

      size_t body_len = strlen(html_body);

      char headers[512];
      int headers_len = snprintf(headers, sizeof(headers),
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/html; charset=UTF-8\r\n"
                                 "Content-Length: %zu\r\n"
                                 "Connection: close\r\n"
                                 "\r\n",
                                 body_len);

      conn->write_total = headers_len + body_len;
      conn->write_buffer = calloc(1, conn->write_total);

      if(conn->write_buffer == NULL)
      {
        LIGHTNING_ERROR("can not allocate the response");
        close_connection(server, fd);
        return;
      }

      memcpy(conn->write_buffer, headers, headers_len);
      memcpy(conn->write_buffer + headers_len, html_body, body_len);

      conn->write_pos = 0;
      conn->state = CONN_STATE_WRITING_RESPONSE;
    }

    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLRDHUP;
    ev.data.fd = fd;

    if(epoll_ctl(server->epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
      fprintf(stderr, "Error: epoll_ctl MOD failed for fd %d: %s\n", fd, strerror(errno));
      close_connection(server, fd);
    }
  }
  else if(data_length == 0)
  {
    close_connection(server, fd);
  }
  else
  {
    if(errno != EAGAIN && errno != EWOULDBLOCK)
    {
      fprintf(stderr, "Error: recv() error on fd %d: %s\n", fd, strerror(errno));
      close_connection(server, fd);
    }
  }
}

static void handle_client_write(struct lightning_server *server, int fd)
{
  struct lightning_connection *conn = &server->connections[fd];

  size_t remaining = conn->write_total - conn->write_pos;
  ssize_t n = send(fd, conn->write_buffer + conn->write_pos, remaining, 0);

  if(n > 0)
  {
    conn->write_pos += n;
    conn->last_activity = time(NULL);

    if(conn->write_pos >= conn->write_total)
    {
      close_connection(server, fd);
    }
  }
  else if(n < 0)
  {
    if(errno != EAGAIN && errno != EWOULDBLOCK)
    {
      fprintf(stderr, "send() error on fd %d: %s\n", fd, strerror(errno));
      close_connection(server, fd);
    }
  }
}

void lightning_connection_init(struct lightning_connection *conn, int fd, struct sockaddr_in *addr)
{
  if(conn == NULL)
  {
    return;
  }

  conn->fd = fd;
  conn->state = CONN_STATE_READING_REQUEST;

  memset(conn->read_buffer, 0, sizeof(conn->read_buffer));
  conn->read_pos = 0;
  conn->read_total = 0;

  conn->write_buffer = NULL;
  conn->write_total = 0;
  conn->write_pos = 0;

  if(addr != NULL)
  {
    memcpy(&conn->client_addr, addr, sizeof(struct sockaddr_in));
  }

  conn->last_activity = time(NULL);
}

void lightning_connection_reset(struct lightning_connection *conn)
{
  if(conn == NULL)
  {
    return;
  }

  if(conn->write_buffer != NULL)
  {
    free(conn->write_buffer);
    conn->write_buffer = NULL;
  }

  conn->fd = -1;
  conn->state = CONN_STATE_CLOSED;
  conn->read_pos = 0;
  conn->read_total = 0;
  conn->write_total = 0;
  conn->write_pos = 0;
}

static int is_request_complete(struct lightning_connection *conn)
{
  if(conn == NULL || conn->read_pos == 0)
  {
    return 0;
  }

  for(size_t i = 0; i < conn->read_pos - 3; i++)
  {
    if(conn->read_buffer[i] == '\r' && conn->read_buffer[i + 1] == '\n' &&
       conn->read_buffer[i + 2] == '\r' && conn->read_buffer[i + 3] == '\n')
    {
      return 1;
    }
  }

  return 0;
}
