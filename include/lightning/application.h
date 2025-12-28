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
 * @file application.h
 * @brief Application of Lightning web framework.
 * -      this will be what the user will actually use.
 */

#ifndef LIGHTNING_APPLICATION_H
#define LIGHTNING_APPLICATION_H

#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>

struct lightning_application;

struct lightning_application *lightning_new_application(const unsigned short port, const int max_connections);
void lightning_ride(struct lightning_application *application);
void lightning_destroy(struct lightning_application *application);

//      LIGHTNING_APPLICATION_H
#endif
