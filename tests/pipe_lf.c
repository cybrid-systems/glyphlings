#define GLYPHLINGS_NO_MAIN
#include "../display/glyphlings-tty.c"

#include <stdio.h>

int main(void) {
    GlyphFrame st;
    char out[16];
    glyphlings_frame_init(&st);
    if (glyphlings_frame(&st, 0x0A, out, sizeof out) != 0)
        return 1;
    if (glyphlings_frame(&st, 0x0A, out, sizeof out) != 0)
        return 1;
    if (glyphlings_frame(&st, (unsigned char)'a', out, sizeof out) != 1)
        return 1;
    if (!(out[0] == 'a' && out[1] == '\n'))
        return 1;
    printf("GLYPHLINGS_OK\n");
    return 0;
}
