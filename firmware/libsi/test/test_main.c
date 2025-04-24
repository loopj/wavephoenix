#include "unity.h"

extern void test_commands(void);
extern void test_gc_controller(void);

__attribute__((weak)) void suiteSetUp(void)
{
}

void setUp(void)
{
}
void tearDown(void)
{
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();

  suiteSetUp();

  test_commands();
  test_gc_controller();

  return UNITY_END();
}
