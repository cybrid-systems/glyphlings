/* Byte pipe. One code point in, one line out. No words and no pictures. */
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
    return ws.ws_row >= 16;
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
    int sent_small = 0;

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
    mkdir_runtime();
    glyphlings_frame_init(&st);

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
            write_all(STDOUT_FILENO, buf, (size_t)n);
        }
        if (FD_ISSET(STDIN_FILENO, &rd)) {
            unsigned char b;
            ssize_t n = read(STDIN_FILENO, &b, 1);
            char line[16];
            if (n <= 0)
                break;
            if (!rows_ok()) {
                if (!sent_small) {
                    write_all(to_child[1], "!\n", 2);
                    sent_small = 1;
                }
                continue;
            }
            sent_small = 0;
            if (glyphlings_frame(&st, b, line, sizeof line) == 1)
                write_all(to_child[1], line, strlen(line));
        }
    }

    close(to_child[1]);
    close(from_child[0]);
    waitpid(kid, NULL, 0);
    restore_tty();
    return 0;
}

#endif
