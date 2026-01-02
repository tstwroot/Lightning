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
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <unistd.h>

#include "lightning/application.h"
#include "internal/server.h"

#define LIGHTNING_BANNER \
"░██         ░██████  ░██████  ░██     ░██ ░██████████░███    ░██ ░██████░███    ░██   ░██████ \n"\
"░██           ░██   ░██   ░██ ░██     ░██     ░██    ░████   ░██   ░██  ░████   ░██  ░██   ░██\n"\
"░██           ░██  ░██        ░██     ░██     ░██    ░██░██  ░██   ░██  ░██░██  ░██ ░██       \n"\
"░██           ░██  ░██  █████ ░██████████     ░██    ░██ ░██ ░██   ░██  ░██ ░██ ░██ ░██  █████\n"\
"░██           ░██  ░██     ██ ░██     ░██     ░██    ░██  ░██░██   ░██  ░██  ░██░██ ░██     ██\n"\
"░██           ░██   ░██  ░███ ░██     ░██     ░██    ░██   ░████   ░██  ░██   ░████  ░██  ░███\n"\
"░██████████ ░██████  ░█████░█ ░██     ░██     ░██    ░██    ░███ ░██████░██    ░███   ░█████░█  "

#define LIGHTNING_ERROR(error_message) \
  fprintf(stderr,                      \
          "[Lightning Error]: In <%s> line %d (%s)\n", __FUNCTION__, __LINE__, error_message)

struct lightning_worker
{
  struct lightning_server *server;
  pthread_t id;
};

struct lightning_application
{
  struct lightning_worker *workers;
  int workers_number;
  int max_connections;
  unsigned short port;
};

struct lightning_application *lightning_new_application(const unsigned short port)
{
  struct lightning_application *application = calloc(1, sizeof(struct lightning_application));

  if(application == NULL)
  {
    return NULL;
  }

  application->port = port;
  application->workers_number = sysconf(_SC_NPROCESSORS_CONF);

  if(application->workers_number < 1)
  {
    application->workers_number = 1;
  }

  application->workers = calloc(application->workers_number, sizeof(struct lightning_worker));

  if(application->workers == NULL)
  {
    return NULL;
  }

  struct rlimit rl;
  getrlimit(RLIMIT_NOFILE, &rl);
  int max_connections = rl.rlim_cur;

  for(int i = 0; i < application->workers_number; i++)
  {
    struct lightning_worker *current_worker = &application->workers[i];
    current_worker->server = lightning_create_server(application->port, max_connections);

    if(current_worker->server == NULL)
    {
      for(int j = 0; j < i; j++)
      {
        lightning_destroy_server(application->workers[j].server);
      }
      free(application->workers);
      free(application);
      return NULL;
    }
  }

  fprintf(stdout, "%s\n\n", LIGHTNING_BANNER);
  printf("Threads: %d\n", application->workers_number);
  printf("Max simultaneous connections: %d\n", max_connections);

  return application;
}

void lightning_ride(struct lightning_application *application)
{
  int created_threads = 0;

  for(int i = 0; i < application->workers_number; i++)
  {
    struct lightning_worker *current_worker = &application->workers[i];
    if(pthread_create(&current_worker->id, NULL, ride_the_lightning, (void *)current_worker->server) != 0)
    {
      for(int j = 0; j < application->workers_number; j++)
      {
        lightning_server_stop(application->workers[j].server);
      }

      for(int j = 0; j < created_threads; j++)
      {
        pthread_join(application->workers[j].id, NULL);
      }

      LIGHTNING_ERROR("failed to create worker thread");
      return;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);

    if(pthread_setaffinity_np(current_worker->id, sizeof(cpu_set_t), &cpuset) != 0)
    {
      fprintf(stderr, "Warning: Failed to set CPU affinity for worker %d\n", i);
    }

    created_threads++;
  }

  for(int i = 0; i < application->workers_number; i++)
  {
    pthread_join(application->workers[i].id, NULL); 
    printf("Thread[%d] finish.\n", i);
  }
}

void lightning_destroy(struct lightning_application *application)
{
  if(application == NULL)
  {
    return;
  }

  if(application->workers != NULL)
  {
    for(int i = 0; i < application->workers_number; i++)
    {
      if(application->workers[i].server != NULL)
      {
        lightning_destroy_server(application->workers[i].server);
      }
    }
    free(application->workers);
  }

  free(application);
}
