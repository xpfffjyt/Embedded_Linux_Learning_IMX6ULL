/*
 * led_test.c - M4: user-space side of the ebf_led driver.
 *
 * Build: make user CROSS_COMPILE=arm-linux-gnueabihf-
 * Run  : ./led_test            (blinks /dev/ebf_led 5 times)
 *        ./led_test /dev/ebf_led 10 500
 *
 * Blink : on 500ms / off 500ms, 5 times, then read back the state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int write_cmd(int fd, char c)
{
	ssize_t n;
	char buf[2];

	buf[0] = c;
	buf[1] = '\n';

	n = write(fd, buf, 2);
	if (n < 0) {
		fprintf(stderr, "write failed: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	const char *path = (argc > 1) ? argv[1] : "/dev/ebf_led";
	int times = (argc > 2) ? atoi(argv[2]) : 5;
	int delay_ms = (argc > 3) ? atoi(argv[3]) : 500;
	int fd, i;
	char rbuf[8];
	ssize_t n;

	if (times <= 0)
		times = 5;
	if (delay_ms <= 0)
		delay_ms = 500;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
		fprintf(stderr, "hint: is the module loaded and did the dts match?\n");
		return 1;
	}

	for (i = 0; i < times; i++) {
		printf("blink %d/%d: on\n", i + 1, times);
		if (write_cmd(fd, '1') < 0)
			break;
		usleep(delay_ms * 1000);

		printf("blink %d/%d: off\n", i + 1, times);
		if (write_cmd(fd, '0') < 0)
			break;
		usleep(delay_ms * 1000);
	}

	/* Read back the current logical state (0/1). */
	if (lseek(fd, 0, SEEK_SET) == 0) {
		n = read(fd, rbuf, sizeof(rbuf) - 1);
		if (n > 0) {
			rbuf[n] = '\0';
			printf("driver reports LED state: %s", rbuf);
		}
	}

	close(fd);
	return 0;
}
