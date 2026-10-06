// pingpong.c - bounce one byte between two processes over a pair of pipes
// and report the rate in exchanges per second.
//
// Build:  add $U/_pingpong to UPROGS in the Makefile, then make qemu
// Usage:  pingpong [exchanges]      (default 100,000)
//
// Time comes from uptime(), which counts ticks of about a tenth of a
// second, so the reported rate is approximate.
//
// One "exchange" = parent writes a byte, child reads it and writes it back,
// parent reads it. That is 4 system calls and (at least) 2 context switches
// when both processes share a CPU.

#include "kernel/types.h"
#include "user/user.h"

static void die(const char *msg) {
    fprintf(2, "pingpong: %s failed\n", msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 100000;
    if (n <= 0) {
        fprintf(2, "usage: %s [exchanges > 0]\n", argv[0]);
        exit(1);
    }

    int p2c[2];  // parent -> child
    int c2p[2];  // child  -> parent
    if (pipe(p2c) < 0 || pipe(c2p) < 0)
        die("pipe");

    int pid = fork();
    if (pid < 0)
        die("fork");

    if (pid == 0) {
        // Child: echo every byte back until the parent closes its write end.
        close(p2c[1]);
        close(c2p[0]);
        char b;
        int r;
        while ((r = read(p2c[0], &b, 1)) == 1) {
            if (write(c2p[1], &b, 1) != 1)
                die("child write");
        }
        if (r < 0)
            die("child read");
        exit(0);
    }

    // Parent: close the ends it doesn't use. Closing p2c[0] matters less,
    // but closing c2p[1] ensures read() returns 0 if the child dies.
    close(p2c[0]);
    close(c2p[1]);

    char b = 'x';
    int t0 = uptime();

    for (int i = 0; i < n; i++) {
        if (write(p2c[1], &b, 1) != 1)
            die("parent write");
        if (read(c2p[0], &b, 1) != 1)
            die("parent read");
    }

    int ticks = uptime() - t0;

    close(p2c[1]);  // child sees EOF and exits
    close(c2p[0]);
    wait(0);

    printf("%d exchanges in %d ticks\n", n, ticks);
    if (ticks > 0)
        printf("%d exchanges/second (%d us per exchange)\n",
               n * 10 / ticks, ticks * 100000 / n);
    else
        printf("too fast to measure, try more exchanges\n");
    exit(0);
}
