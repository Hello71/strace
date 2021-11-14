/*
 * Test -Y/--decode-pids=comm option.
 *
 * Copyright (c) 2021 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * The executable built from this source file should
 * have a long name (> 16) to test how strace reports
 * the initial value of /proc/$pid/comm.
 * Even if linux returns a longer name, strace should
 * not crash.
 */

#include "tests.h"
#include "scno.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NEW_NAME "0123456789abcdefghijklmnopqrstuvwxyz"

static int
do_default_action(void)
{
	static const char proc_self_exe[] = "/proc/self/exe";
	skip_if_unavailable(proc_self_exe);

	char comm[sizeof(NEW_NAME)];
	if (prctl(PR_GET_NAME, comm))
		perror_msg_and_skip("PR_GET_NAME");

	pid_t pid  = getpid();
	pid_t ppid = getppid();

	printf("%-5d<%s> getppid() = %d\n", pid, comm, ppid);
	fflush(stdout);

	pid_t child = fork();
	if (child < 0)
		perror_msg_and_fail("fork");
	else if (child == 0) {
		pid = getpid();
		ppid = getppid();
		printf("%-5d<%s> getppid() = %d\n", pid, comm, ppid);

		strcpy(comm, NEW_NAME);
		prctl(PR_SET_NAME, comm);
		prctl(PR_GET_NAME, comm);

		ppid = getppid();
		printf("%-5d<%s> getppid() = %d\n", pid, comm, ppid);

		long rc = syscall(__NR_tgkill, ppid, ppid, SIGCONT);
		printf("%-5d<%s> tgkill(%d, %d, SIGCONT) = %s\n",
		       pid, comm, ppid, ppid, sprintrc(rc));

		fflush(stdout);
		char *args[] = { (char *) "unused", (char *) "execve", NULL };
		execve(proc_self_exe, args, NULL);
		perror_msg_and_fail("execve: %s", proc_self_exe);
	} else {
		int status;
		wait(&status);
		printf("%-5d<exe> +++ exited with 0 +++\n", child);

		ppid = getppid();
		printf("%-5d<%s> getppid() = %d\n", pid, comm, ppid);
		printf("%-5d<%s> +++ exited with 0 +++\n", pid, comm);
		return WEXITSTATUS(status);
	}
	return 0;
}

static int
do_execve_action(int argc, char **argv)
{
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc < 2)
		return do_default_action();
	else if (strcmp(argv[1], "execve") == 0)
		return do_execve_action(argc, argv);
	error_msg_and_fail("unexpected argument: %s", argv[1]);
}
