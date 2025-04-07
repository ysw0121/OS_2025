#include "lib.h"
#include "types.h"

int data = 0;

int uEntry(void)
{
	printf("==============TEST FOR BASIC==============\n");
	int ret = fork();
	int i = 8;
	int pid = getpid();

	if (ret == 0)
	{
		data = 2;
		while (i != 0)
		{
			i--;
			printf("Child Process (pid:%d): Pong %d, %d;\n", pid, data, i);
			sleep(128);
		}
		exit();	
		printf("If exit() worked, this message wouldn't appear.\n");
	}
	else if (ret != -1)
	{
		data = 1;
		while (i != 0)
		{
			i--;
			printf("Parent Process (pid:%d): Ping %d, %d;\n", pid, data, i);
			sleep(128);
		}
	}
	printf("===========TEST FOR BASIC OVER===========\n\n");
	exit(); 	// If you plan to test wait() function, please remove this line.
	/*
	sleep(256);

	printf("==============TEST 1 FOR WAIT=============\n");
	ret = fork();
	i = 4;
	pid = getpid();
	int ppid = getppid();
	if (ret == 0)
	{
		data = 2;
		while (i != 0)
		{
			i--;
			printf("Child Process (pid:%d, ppid:%d): Pong %d, %d;\n", pid, ppid, data, i);
			sleep(128);
		}
		exit();
		printf("If exit() worked, this message wouldn't appear.\n");
	}
	else if (ret != -1)
	{
		int wait_ret = 114514;
		wait_ret = wait();
		printf("first wait() returns: %d\n", wait_ret);
		wait_ret = wait();
		printf("second wait() returns: %d\n", wait_ret);
		data = 1;
		while (i != 0)
		{
			i--;
			printf("Parent Process (pid:%d, ppid:%d): Ping %d, %d;\n", pid, ppid, data, i);
			sleep(128);
		}
	}
	printf("===========TEST 1 FOR WAIT OVER===========\n\n");

	sleep(256);

	printf("==============TEST 2 FOR WAIT=============\n");
	ret = fork();
	i = 4;
	pid = getpid();
	
	if (ret == 0)
	{
		data = 2;
		while (i != 0)
		{	
			i--;
			ppid = getppid();
			printf("Child Process (pid:%d, ppid:%d): Pong %d, %d;\n", pid, ppid, data, i);
			sleep(128);
		}
		//exit();
	}
	else if (ret != -1)
	{
		data = 1;
		while (i != 0)
		{
			i--;
			ppid = getppid();
			printf("Parent Process (pid:%d, ppid:%d): Ping %d, %d;\n", pid, ppid, data, i);
		}
		exit();
		printf("If exit() worked, this message wouldn't appear.\n");
	}
	printf("===========TEST 2 FOR WAIT OVER===========\n\n");
	exit();
	*/
	while (1)
		;
	return 0;
}
