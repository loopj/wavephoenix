#include "unity.h"

extern void test_bch3121();
extern void test_packet();

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

  test_bch3121();
  test_packet();

  return UNITY_END();
}
