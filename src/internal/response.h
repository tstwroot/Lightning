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

#ifndef LIGHTNING_RESPONSE_H
#define LIGHTNING_RESPONSE_H

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  size_t capacity;
};

struct lightning_http_response
{
  char version[10];
  int status_code;
  char *status_message;

  struct header *headers;
  char *content_type;
  size_t content_length;
  time_t date;

  struct body *body;
};

struct header *add_header(struct lightning_http_response *resp, char *name, char *value)
{
  struct header *new_header = malloc(sizeof(struct header));
  if(new_header == NULL)
  {
    return NULL;
  }

  new_header->name = strdup(name);
  new_header->value = strdup(value);
  resp->headers->next = new_header;

  return new_header;
}

//      LIGHTNING_RESPONSE_H
#endif
