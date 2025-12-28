#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lightning/connection.h>

struct lightning_connection *lightning_create_connection(int max_connections)
{
  struct lightning_connection *connections = calloc(max_connections, sizeof(struct lightning_connection));
  if(connections == NULL)
  {
    return NULL;
  }

  for(int i = 0; i < max_connections; i++)
  {
    lightning_connection_reset(&connections[i]);
  }

  return connections;
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
