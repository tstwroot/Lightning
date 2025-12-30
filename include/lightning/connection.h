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

#ifndef LIGHTNING_CONNECTION_H
#define LIGHTNING_CONNECTION_H

#include <arpa/inet.h>

#define LIGHTNING_READ_BUFFER_SIZE 8192
#define LIGHTNING_WRITE_BUFFER_SIZE 8192

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
  char write_buffer[LIGHTNING_WRITE_BUFFER_SIZE];
  size_t read_pos;
  size_t read_total;
  size_t write_total;
  size_t write_pos;
  time_t last_activity;
  enum lightning_connection_state state;
  int fd;
};

struct lightning_connection *lightning_create_connection(int max_connections);
void lightning_connection_init(struct lightning_connection *conn, int fd, struct sockaddr_in *addr);
void lightning_connection_reset(struct lightning_connection *conn);

//      LIGHTNING_CONNECITON_H
#endif
