// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 YANDEX LLC.  All Rights Reserved.
// Author: Dmitry Monakhov <dmtrmonakhov@yandex-team.ru>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <limits.h>

char *progname;

void usage(void)
{
	printf("Usage %s: [-b uid] [-e uid] [-g] [-P] [-t timeout] FNAME\n", progname);
	printf("\t\t -b: begin of uid range\n");
	printf("\t\t -e: length of uid range\n");
	printf("\t\t -g: change group uid\n");
	printf("\t\t -t: loop timeout\n");
	printf("\t\t -P: Do not run chowner loop, only prepare files\n");
	exit(1);
}
int main(int argc, char *const *argv)
{
	char *filename;
	int fd;
	int c;
	uid_t begin = 2000;
	uid_t end = 12000;
	unsigned int timeout = 10;
	struct timeval start, now, delta = { 0, 0 };
	uid_t uid;
	gid_t gid = getegid();
	int do_group = 0;
	int do_prepare = 0;

	progname = argv[0];
	while ((c = getopt(argc, argv, "b:e:gPt:")) != -1) {
		switch (c) {
		case 'b':
			begin = atoi(optarg);
			break;
		case 'e':
			end = atoi(optarg);
			break;
		case 'g':
			do_group = 1;
			break;
		case 'P':
			do_prepare = 1;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			usage();
		}
	}
	if (optind == argc-1)
		filename = argv[optind];
	else
		usage();
	if (do_prepare) {
		char path[PATH_MAX];
		for (uid = begin; uid < end;uid++) {
			sprintf(path, "%s.%d", filename, uid);
			fd = open(path, O_RDWR|O_CREAT, 0666);
			if (fd < 0) {
				perror("open");
				exit(1);
			}
			if (do_group)
				gid = uid;
			if (fchown(fd, uid, gid)) {
				perror("chown");
				exit(1);
			}
			close(fd);
		}
		return 0;
	}
	fd = open(filename, O_RDWR|O_CREAT, 0666);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	gettimeofday(&start, NULL);

	while (1) {
		for (uid = begin; uid < end;uid++) {
			if (do_group)
				gid = uid;
			if (fchown(fd, uid, gid)) {
				perror("chown");
				exit(1);
			}
		}
		gettimeofday(&now, NULL);
		timersub(&now, &start, &delta);
		if (delta.tv_sec >= timeout)
			break;
	}
	return 0;
}
