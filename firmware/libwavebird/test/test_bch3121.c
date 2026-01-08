#include "unity.h"

#include "wavebird/bch3121.h"

static const uint32_t valid_codeword = 0x0394a9d0;
static const uint32_t valid_message  = 0x00015620;

void setUp(void)
{
}

void tearDown(void)
{
}

// Test bch3121_decode decodes a valid codeword
static void test_decode()
{
  uint32_t decoded_message;
  uint32_t syndrome = bch3121_decode(&decoded_message, valid_codeword);

  TEST_ASSERT_EQUAL(0, syndrome);
  TEST_ASSERT_EQUAL_HEX32(valid_message, decoded_message);
}

// Test bch3121_decode fails for a single-bit error in every position
static void test_decode_failure()
{
  uint32_t decoded_message;

  for (int i = 0; i < 31; i++) {
    uint32_t corrupted_codeword = valid_codeword ^ (1 << i);
    uint32_t syndrome           = bch3121_decode(&decoded_message, corrupted_codeword);

    TEST_ASSERT_NOT_EQUAL(0, syndrome);
  }
}

// Test bch3121_decode_and_correct can correct a single-bit error in every position
static void test_decode_correct_single_error()
{
  uint32_t decoded_message;

  for (int i = 0; i < 31; i++) {
    uint32_t corrupted_codeword = valid_codeword ^ (1 << i);
    int errors_corrected        = bch3121_decode_and_correct(&decoded_message, corrupted_codeword);

    TEST_ASSERT_EQUAL(1, errors_corrected);
    TEST_ASSERT_EQUAL_HEX32(valid_message, decoded_message);
  }
}

// Test bch3121_decode_and_correct can correct a double-bit error in every pair of positions
static void test_decode_correct_double_error()
{
  uint32_t decoded_message;

  for (int i = 0; i < 31; i++) {
    for (int j = i + 1; j < 31; j++) {
      uint32_t corrupted_codeword = valid_codeword ^ (1 << i) ^ (1 << j);
      int errors_corrected        = bch3121_decode_and_correct(&decoded_message, corrupted_codeword);

      TEST_ASSERT_EQUAL(2, errors_corrected);
      TEST_ASSERT_EQUAL_HEX32(valid_message, decoded_message);
    }
  }
}

// Test bch3121_decode_and_correct fails for a triple-bit error
static void test_decode_correct_triple_error()
{
  uint32_t decoded_message;
  uint32_t corrupted_codeword = valid_codeword ^ 0x7;
  int rcode                   = bch3121_decode_and_correct(&decoded_message, corrupted_codeword);

  TEST_ASSERT_EQUAL(-BCH3121_ERR_UNCORRECTABLE, rcode);
}

// Test bch3121_encode encodes a message
static void test_encode()
{
  uint32_t encoded_codeword = bch3121_encode(valid_message);
  TEST_ASSERT_EQUAL_HEX32(valid_codeword, encoded_codeword);
}

static void test_encode_decode()
{
  uint32_t codeword = bch3121_encode(0x12345);
  uint32_t message;
  int rcode = bch3121_decode_and_correct(&message, codeword);

  TEST_ASSERT_EQUAL(0, rcode);
  TEST_ASSERT_EQUAL_HEX32(0x12345, message);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();

  RUN_TEST(test_decode);
  RUN_TEST(test_decode_failure);
  RUN_TEST(test_decode_correct_single_error);
  RUN_TEST(test_decode_correct_double_error);
  RUN_TEST(test_decode_correct_triple_error);
  RUN_TEST(test_encode);
  RUN_TEST(test_encode_decode);

  return UNITY_END();
}