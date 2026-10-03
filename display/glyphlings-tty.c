/* Byte pipe. One code point in, one line out.
   The spell colors are copied from the line the child already painted.
   This file does not keep its own word list. */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned char buf[8];
    int have;
    int need;
} GlyphFrame;

void glyphlings_frame_init(GlyphFrame *st) {
    st->have = 0;
    st->need = 0;
}

/* 1 = a line was written into out (including the newline). 0 = drop or wait. */
int glyphlings_frame(GlyphFrame *st, unsigned char b, char *out, size_t cap) {
    if (b == 0x0A) {
        st->have = 0;
        st->need = 0;
        return 0;
    }
    if (st->have == 0) {
        int need = 0;
        if (b < 0x80) {
            if (cap < 3)
                return 0;
            out[0] = (char)b;
            out[1] = '\n';
            out[2] = '\0';
            return 1;
        }
        if ((b & 0xE0) == 0xC0)
            need = 2;
        else if ((b & 0xF0) == 0xE0)
            need = 3;
        else if ((b & 0xF8) == 0xF0)
            need = 4;
        else
            return 0;
        st->buf[0] = b;
        st->have = 1;
        st->need = need;
        return 0;
    }
    if ((b & 0xC0) != 0x80) {
        st->have = 0;
        st->need = 0;
        return 0;
    }
    if (st->have < 8)
        st->buf[st->have++] = b;
    if (st->have < st->need)
        return 0;
    if ((size_t)st->have + 2 > cap) {
        st->have = 0;
        st->need = 0;
        return 0;
    }
    memcpy(out, st->buf, (size_t)st->have);
    out[st->have] = '\n';
    out[st->have + 1] = '\0';
    st->have = 0;
    st->need = 0;
    return 1;
}

#define GLYPHLINGS_SPELL_CAP 8
#define GLYPHLINGS_SPELL_ROW 15

typedef struct {
    char letters[GLYPHLINGS_SPELL_CAP];
    int n;
    int at;
    int base_at;
    char line[480];
    int linelen;
    int overflow;
} GlyphSpell;

void glyphlings_spell_init(GlyphSpell *s) {
    memset(s, 0, sizeof *s);
    s->at = -1;
    s->base_at = -1;
}

static int spell_line(const char *s, int len, char *letters, int *at_out) {
    int n = 0;
    int at = -1;
    int mode = 0;
    int i = 0;
    while (i < len && n < GLYPHLINGS_SPELL_CAP) {
        if (s[i] == '\x1b' && i + 1 < len && s[i + 1] == '[') {
            int j = i + 2;
            int ps = j;
            while (j < len && s[j] != 'm' && s[j] != 'H' && s[j] != 'J' && s[j] != '\x1b')
                j++;
            if (j < len && s[j] == 'm') {
                int plen = j - ps;
                if (plen == 2 && s[ps] == '3' && s[ps + 1] == '2')
                    mode = 1;
                else if (plen == 2 && s[ps] == '3' && s[ps + 1] == '3')
                    mode = 2;
                else if (plen == 1 && s[ps] == '2')
                    mode = 3;
                else
                    mode = 0;
                i = j + 1;
                continue;
            }
            i = (j < len) ? j + 1 : len;
            continue;
        }
        if (s[i] >= 'a' && s[i] <= 'z' && mode >= 1 && mode <= 3) {
            if (mode == 2 && at < 0)
                at = n;
            letters[n++] = s[i];
            mode = 0;
        }
        i++;
    }
    if (n < 3 || n > 5)
        return 0;
    if (at < 0)
        at = n;
    *at_out = at;
    return n;
}

static void spell_take(GlyphSpell *s, const char *letters, int n, int at) {
    int same = s->n == n && n > 0 && memcmp(s->letters, letters, (size_t)n) == 0;
    memcpy(s->letters, letters, (size_t)n);
    s->n = n;
    if (!same) {
        s->at = at;
        s->base_at = at;
        return;
    }
    s->base_at = at;
    if (s->at < 0 || at > s->at)
        s->at = at;
}

int glyphlings_spell_note(GlyphSpell *s, const char *buf, size_t n) {
    size_t i;
    int hit = 0;
    for (i = 0; i < n; i++) {
        char c = buf[i];
        if (c == '\n') {
            if (!s->overflow) {
                char letters[GLYPHLINGS_SPELL_CAP];
                int at = -1;
                int got = spell_line(s->line, s->linelen, letters, &at);
                if (got > 0) {
                    spell_take(s, letters, got, at);
                    hit = 1;
                }
            }
            s->linelen = 0;
            s->overflow = 0;
            continue;
        }
        if (s->linelen >= (int)sizeof s->line)
            s->overflow = 1;
        else
            s->line[s->linelen++] = c;
    }
    return hit;
}

int glyphlings_spell_ahead(const GlyphSpell *s) {
    return s->n >= 3 && s->at > s->base_at;
}

int glyphlings_spell_patch(const GlyphSpell *s, char *out, size_t cap) {
    int n = 0;
    int i;
    int w;
    if (s->n < 3 || s->at < 0 || cap < 64)
        return 0;
    if (s->at < s->n) {
        w = snprintf(out + n, cap - (size_t)n,
                     "\x1b[1;1H\x1b[1m请按这个键  %c\x1b[0m\x1b[K",
                     s->letters[s->at]);
        if (w < 0 || (size_t)w >= cap - (size_t)n)
            return 0;
        n += w;
    }
    w = snprintf(out + n, cap - (size_t)n, "\x1b[%d;1H", GLYPHLINGS_SPELL_ROW);
    if (w < 0 || (size_t)w >= cap - (size_t)n)
        return 0;
    n += w;
    for (i = 0; i < s->n; i++) {
        const char *open = (i < s->at) ? "\x1b[32m" : (i == s->at) ? "\x1b[1m\x1b[33m" : "\x1b[2m";
        if (i > 0) {
            if ((size_t)n + 2 >= cap)
                return 0;
            out[n++] = ' ';
            out[n++] = ' ';
        }
        w = snprintf(out + n, cap - (size_t)n, "%s%c\x1b[0m", open, s->letters[i]);
        if (w < 0 || (size_t)w >= cap - (size_t)n)
            return 0;
        n += w;
    }
    if ((size_t)n + 3 >= cap)
        return 0;
    out[n++] = '\x1b';
    out[n++] = '[';
    out[n++] = 'K';
    out[n] = '\0';
    return n;
}

int glyphlings_spell_key(GlyphSpell *s, unsigned char b) {
    if (b >= 'A' && b <= 'Z')
        b = (unsigned char)(b - 'A' + 'a');
    if (b == 8 || b == 127) {
        if (s->n >= 3 && s->at > 0) {
            s->at--;
            return 1;
        }
        return 0;
    }
    if (b < 'a' || b > 'z')
        return 0;
    if (s->n < 3 || s->at < 0 || s->at >= s->n)
        return 0;
    if (s->letters[s->at] != (char)b)
        return 0;
    s->at++;
    return 1;
}

#ifndef GLYPHLINGS_NO_MAIN

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>

static struct termios saved_tty;
static int raw_on = 0;

static void restore_tty(void) {
    if (raw_on) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
        raw_on = 0;
    }
}

static void enter_raw(void) {
    struct termios t;
    if (!raw_on)
        tcgetattr(STDIN_FILENO, &saved_tty);
    t = saved_tty;
    t.c_lflag = (t.c_lflag & ~(tcflag_t)(ICANON | ECHO)) | (tcflag_t)ISIG;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    raw_on = 1;
}

static void on_int(int sig) {
    restore_tty();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void on_tstp(int sig) {
    (void)sig;
    restore_tty();
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

static void on_cont(int sig) {
    (void)sig;
    signal(SIGTSTP, on_tstp);
    if (isatty(STDIN_FILENO))
        enter_raw();
}

static int rows_ok(void) {
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0)
        return 1;
    return ws.ws_row >= 24;
}

static void write_all(int fd, const char *p, size_t n) {
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        p += w;
        n -= (size_t)w;
    }
}

static void ensure_env(const char *key, const char *val) {
    if (getenv(key) == NULL)
        setenv(key, val, 1);
}

static void mkdir_runtime(void) {
    if (mkdir("runtime", 0755) != 0 && errno != EEXIST)
        return;
}

int main(void) {
    int to_child[2];
    int from_child[2];
    pid_t kid;
    const char *bin;
    GlyphFrame st;
    GlyphSpell spell;
    int sent_small = 0;
    int busy = 0;
    int seen = 0;
    static const char wake[] = "\x1b[H\x1b[2J\x1b[1m小兽正在醒，等一等\x1b[0m\n";

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "请在终端里玩\n");
        return 1;
    }
    if (pipe(to_child) != 0 || pipe(from_child) != 0)
        return 1;

    ensure_env("AURA_BIN", "/home/dev/code/grok-dev/aura-grok/build/aura");
    ensure_env("AURA_PATH", "/home/dev/code/grok-dev/aura-grok/lib");
    ensure_env("AURA_SANDBOX", "off");
    ensure_env("AURA_PIPELINE_STRICT", "force-soa");
    bin = getenv("AURA_BIN");

    kid = fork();
    if (kid < 0)
        return 1;
    if (kid == 0) {
        dup2(to_child[0], STDIN_FILENO);
        dup2(from_child[1], STDOUT_FILENO);
        close(to_child[0]);
        close(to_child[1]);
        close(from_child[0]);
        close(from_child[1]);
        execl(bin, bin, "aura/main.aura", (char *)NULL);
        fprintf(stderr, "找不到 Aura 二进制：%s\n", bin);
        _exit(127);
    }
    close(to_child[0]);
    close(from_child[1]);

    signal(SIGINT, on_int);
    signal(SIGTERM, on_int);
    signal(SIGTSTP, on_tstp);
    signal(SIGCONT, on_cont);
    signal(SIGPIPE, SIG_IGN);
    atexit(restore_tty);
    enter_raw();
    write_all(STDOUT_FILENO, wake, sizeof wake - 1);
    mkdir_runtime();
    glyphlings_frame_init(&st);
    glyphlings_spell_init(&spell);

    if (!rows_ok()) {
        write_all(to_child[1], "!\n", 2);
        sent_small = 1;
    }

    for (;;) {
        fd_set rd;
        int mx = from_child[0] > STDIN_FILENO ? from_child[0] : STDIN_FILENO;
        int rc;
        FD_ZERO(&rd);
        FD_SET(STDIN_FILENO, &rd);
        FD_SET(from_child[0], &rd);
        rc = select(mx + 1, &rd, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (FD_ISSET(from_child[0], &rd)) {
            char buf[4096];
            ssize_t n = read(from_child[0], buf, sizeof buf);
            if (n <= 0)
                break;
            char patch[256];
            int pn;
            write_all(STDOUT_FILENO, buf, (size_t)n);
            if (glyphlings_spell_note(&spell, buf, (size_t)n) &&
                glyphlings_spell_ahead(&spell)) {
                pn = glyphlings_spell_patch(&spell, patch, sizeof patch);
                if (pn > 0)
                    write_all(STDOUT_FILENO, patch, (size_t)pn);
            }
            busy = 0;
            seen = 1;
        }
        if (FD_ISSET(STDIN_FILENO, &rd)) {
            unsigned char b;
            ssize_t n = read(STDIN_FILENO, &b, 1);
            char line[16];
            if (n <= 0)
                break;
            /* Boot paints the first frame. Keys before that would sit in the pipe. */
            if (!seen)
                continue;
            /* One key at a time. Growing the animal recompiles the workspace. */
            if (busy) {
                glyphlings_frame_init(&st);
                continue;
            }
            if (!rows_ok()) {
                if (!sent_small) {
                    write_all(to_child[1], "!\n", 2);
                    sent_small = 1;
                }
                continue;
            }
            sent_small = 0;
            if (glyphlings_frame(&st, b, line, sizeof line) == 1) {
                char patch[256];
                int pn;
                /* The child repaints after the workspace write. Color this letter first. */
                if (glyphlings_spell_key(&spell, b)) {
                    pn = glyphlings_spell_patch(&spell, patch, sizeof patch);
                    if (pn > 0)
                        write_all(STDOUT_FILENO, patch, (size_t)pn);
                }
                write_all(to_child[1], line, strlen(line));
                busy = 1;
            }
        }
    }

    close(to_child[1]);
    close(from_child[0]);
    waitpid(kid, NULL, 0);
    restore_tty();
    return 0;
}

#endif
