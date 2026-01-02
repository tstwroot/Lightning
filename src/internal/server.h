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

/**
 * @file server.h
 * @brief Server creation, run and free
 */

#ifndef LIGHTNING_SERVER_H
#define LIGHTNING_SERVER_H

#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define LIGHTNING_EPOLL_MAX_EVENTS 64
#define LIGHTNING_EPOLL_TIMEOUT_MS -1

#define LIGHTNING_ERROR(error_message) \
  fprintf(stderr,                      \
          "[Lightning Error]: In <%s> line %d (%s)\n", __FUNCTION__, __LINE__, error_message)

struct lightning_server;

struct lightning_server *lightning_create_server(const unsigned short port, int max_connections);
void *ride_the_lightning(void *args);
void lightning_destroy_server(struct lightning_server *server);
void lightning_server_stop(struct lightning_server *server);

//      LIGHTNING_SERVER_H
#endif
