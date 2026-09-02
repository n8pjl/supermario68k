#pragma once

#include <stdint.h>
void RLEDecompress(uint8_t *output, uint8_t *input, uint16_t length);

uint16_t RLECompress(uint8_t *output, uint8_t *input, uint16_t length);