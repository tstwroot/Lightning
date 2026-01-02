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

#ifndef LIGHTNING_REQUEST_H
#define LIGHTNING_REQUEST_H

#include <stdio.h>

enum http_methods
{
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_HEAD,
  HTTP_OPTIONS,
  HTTP_PATCH,
  HTTP_UNKNOWN
};

struct header
{
  char *name;
  char *value;
  struct header *next;
};

struct body
{
  void *data;
  size_t length;
};

struct lightning_http_request
{
  enum http_methods method;
  char *uri;
  char *path;
  char *query_string;
  char version[10];

  struct header *headers;
  char *host;
  char *content_type;
  size_t content_length;
  char *user_agent;

  struct body *body;
};

//      LIGHTNING_REQUEST_H
#endif
