#include <lightning.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    struct lightning_application *app = lightning_new_application(8080);

  if(app == NULL)
  {
    fprintf(stderr, "Error: can't allocate memory for the application\n");
    exit(EXIT_FAILURE);
  }

  lightning_ride(app);
  lightning_destroy(app);
  return 0;
}
