#include <stdbool.h>
#include <stdint.h>

#define LCD_SIZE 3840

uint8_t renderbuf[LCD_SIZE][8][4];

void render(uint8_t* restrict light, uint8_t* restrict dark) {
    for (int i = 0; i < LCD_SIZE; i++) {
        uint8_t l = ~light[i];
        uint8_t d = ~dark[i];
        for (int j = 0; j < 8; j++) {
            uint8_t color = (((l >> (7 - j)) & 1) * 0b01010101) |
                            (((d >> (7 - j)) & 1) * 0b10101010);

            renderbuf[i][j][0] = color;
            renderbuf[i][j][1] = color;
            renderbuf[i][j][2] = color;
            renderbuf[i][j][3] = 255;
        }
    }
}
