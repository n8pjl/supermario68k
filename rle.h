#pragma once

void RLEDecompress(unsigned char *output, unsigned char *input,
                   unsigned short length);

unsigned short RLECompress(unsigned char *output, unsigned char *input,
                           unsigned short length);