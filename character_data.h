#include <stdint.h>

#ifdef __cplusplus
extern "C"
#endif

void write_char_at(char c, int x, int y, uint16_t fg_colour, uint16_t bg_colour, uint16_t buffer[], int width, int height);
void write_string_at(const char* str, int x, int y, uint16_t fg_colour, uint16_t bg_colour, uint16_t buffer[], int width, int height);

#ifdef __cplusplus
}
#endif
