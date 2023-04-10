/**
 * buff_test_data_t.h
 *
 * Buffered Task Test Data Structure
 *
 * Defines a data structure for testing buffered tasks and helper
 * functions.
 *
 */

#ifndef BUFF_TEST_DATA_H__
#define BUFF_TEST_DATA_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "task_time.h"

// Len of the Internal Buffer Data Buffer
#define BUFF_DATA_LEN 64

typedef struct
{
  uint8_t data[BUFF_DATA_LEN]; // Data buffer
  uint16_t data_len;           // Length of the Data stored in the Buffer (bytes)
} buffer_t;

/**
 * Buffer Test Data Structure
 */
typedef struct
{

  buffer_t buff;
  uint16_t buff_crc; // The CRC16 value of the data buffer

  uint32_t handler_count;  // Count of the number of times the handler was called.
  uint32_t crc_fail_count; // Count of the number of times the CRC check failed.

} buff_test_data_t;

/* Function for storing the calculated CRC value of the data buffer
 * in the test structure.
 */
void buff_crc_calc(buff_test_data_t *p_data);

/* Function for checking if the calculated CRC16 value of the
 * buffer matches the CRC16 value stored in the structure.
 */
bool buff_crc_check(buff_test_data_t *p_data);

/* Function for setting the buffer test data array to random data
 * values of a random length.
 */
void buff_randomize(buff_test_data_t *p_data);

// Function for initializing the buffer data structure.
void buff_init(buff_test_data_t *p_data);

#endif // BUFF_TEST_DATA_H__