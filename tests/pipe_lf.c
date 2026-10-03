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
    {
        GlyphSpell sp;
        char patch[256];
        const char *frame =
            "\x1b[32ma\x1b[0m  \x1b[32mp\x1b[0m  \x1b[32mp\x1b[0m  "
            "\x1b[1m\x1b[33ml\x1b[0m  \x1b[2me\x1b[0m\n";
        int n;
        glyphlings_spell_init(&sp);
        if (glyphlings_spell_note(&sp, frame, strlen(frame)) != 1)
            return 1;
        if (!(sp.n == 5 && sp.at == 3 && sp.letters[3] == 'l'))
            return 1;
        if (glyphlings_spell_key(&sp, (unsigned char)'L') != 1)
            return 1;
        if (sp.at != 4)
            return 1;
        n = glyphlings_spell_patch(&sp, patch, sizeof patch);
        if (n <= 0)
            return 1;
        if (strstr(patch, "\x1b[32ml\x1b[0m") == NULL)
            return 1;
        if (strstr(patch, "\x1b[1m\x1b[33me\x1b[0m") == NULL)
            return 1;
        if (strstr(patch, "请按这个键  e") == NULL)
            return 1;
        if (strstr(patch, "按到啦") != NULL)
            return 1;
        if (glyphlings_spell_key(&sp, (unsigned char)'x') != 0)
            return 1;
    }
    printf("GLYPHLINGS_OK\n");
    return 0;
}
