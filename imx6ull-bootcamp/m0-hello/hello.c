/*
 * hello.c - M0: the first cross-compiled program for the i.MX6ULL board.
 *
 * Build:  make CROSS_COMPILE=arm-linux-gnueabihf-
 * Check:  file hello          -> "ELF 32-bit LSB executable, ARM, EABI5"
 * Run  :  ./hello             (on the board)
 *
 * Keep this file ASCII-only: the board console / old toolchains may not like
 * UTF-8 comments, and mojibake in a serial log is a needless distraction.
 */

#include <stdio.h>
#include <sys/utsname.h>

int main(void)
{
    struct utsname u;

    printf("Hello from the i.MX6ULL board!\n");

    if (uname(&u) == 0) {
        printf("sysname : %s\n", u.sysname);
        printf("nodename: %s\n", u.nodename);
        printf("release : %s\n", u.release);
        printf("machine : %s\n", u.machine);
    } else {
        perror("uname");
        return 1;
    }

    return 0;
}
