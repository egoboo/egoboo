#include <stdio.h> /* for NULL */
#include <sys/time.h>

static double startuptime;

void sys_initialize()
{
  struct timeval now;
  gettimeofday( &now, NULL );
  startuptime = now.tv_sec + now.tv_usec / 1000000.0f;
}

void sys_shutdown()
{
}

double sys_getTime()
{
  struct timeval now;
  gettimeofday( &now, NULL );
  return now.tv_sec + now.tv_usec / 1000000.0f - startuptime;
}

int sys_frameStep()
{
  return 0;
}

int main( int argc, char* argv[] )
{
  return SDL_main( argc, argv );
}
