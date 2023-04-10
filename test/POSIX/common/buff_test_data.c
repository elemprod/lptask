#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "crc16.h"
#include "buff_test_data.h"

void buff_crc_calc(buff_test_data_t *p_data)
{
  assert(p_data != NULL);
  p_data->buff_crc = crc16((const char *)&p_data->buff, sizeof(buffer_t));
}

bool buff_crc_check(buff_test_data_t *p_data)
{
  assert(p_data != NULL);
  uint16_t crc_calc = crc16((const char *)&p_data->buff, sizeof(buffer_t));
  return (crc_calc == p_data->buff_crc);
}

void buff_randomize(buff_test_data_t *p_data)
{
  assert(p_data != NULL);

  // Zero the data array.
  memset(p_data->buff.data, 0x00, BUFF_DATA_LEN);

  // Generate a random length for the data array.
  uint32_t data_len = (rand() % (BUFF_DATA_LEN - 1)) + 1;
  assert(data_len <= BUFF_DATA_LEN);
  p_data->buff.data_len = data_len;

  // Fill the data array with random data
  for (uint32_t index; index < data_len; index++)
  {
    p_data->buff.data[index] = (uint8_t)(rand() % UINT8_MAX);
  }
}

void buff_init(buff_test_data_t *p_data)
{
  assert(p_data != NULL);

  // Zero the entire structure.
  memset(p_data, 0x00, sizeof(buff_test_data_t));

  // Generate a new set of random data
  buff_randomize(p_data);

  // Update the CRC value.
  buff_crc_calc(p_data);
}