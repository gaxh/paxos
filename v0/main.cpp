#include "env.h"

#include <random>
#include <sys/time.h>

int main(int argc, char** argv) {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  int seed = (int)(ms % 100000);

  srand(seed);
  Env e;
  e.init();
  e.run();
  return 0;
}