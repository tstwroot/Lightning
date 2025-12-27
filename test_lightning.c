#include <stdio.h>
#include <lightning.h>

int main(void)
{
  struct lightning_server *server = lightning_create(8080);

  if(server == NULL)
  {
    return -1;
  }

  lightning_ride(server);
  lightning_destroy(server);
  return 0;
}
