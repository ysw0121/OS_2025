#include "lib.h"
#include "types.h"

int uEntry(void)
{
	// For lab4.1
	// Test 'scanf'
	int dec = 0;
	int hex = 0;
	char str[6];
	char cha = 0;
	int ret = 0;
	while (1) {
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
	}

	// For lab4.2
	// Test 'Semaphore'
	int i = 4;

	sem_t sem;
	printf("Parent Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 2);
	if (ret == -1) {
		printf("Parent Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0) {
		while (i != 0)
		{
			i--;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf(".. Wait Over: ret = %d\n", ret);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		ret = sem_destroy(&sem);
		printf(".. Destroy ret = %d\n", ret);
		exit();
	} else if (ret != -1) {
		while (i != 0)
		{
			i--;
			printf("Parent Process: Sleeping.\n");
			sleep(128);
			printf("Parent Process: Semaphore Posting.\n");
			ret = sem_post(&sem);
			printf(".. Post ret = %d\n", ret);
		}
		printf("Parent Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		// exit();
	}

	// For lab4.3
	// Test 'Shared Variable'
	printf("==============TEST SHAREDVARIABLE=============\n");
	int number = 114514;

	sharedvar_t svar;
	ret = createSharedVariable(&svar, number);
	printf("Parent Process: create Shared Variable: %d  with value: %d\n", svar, number);
	if (ret == -1) exit();

	ret = fork();
	if (ret == 0) {
		number = readSharedVariable(&svar);
		printf("Child Process: readShared Variable: %d get value: %d\n", svar, number);
		sleep(128);

		number = readSharedVariable(&svar);
		printf("Child Process: readShared Variable: %d get value: %d\n", svar, number);
		number = 2333;
		writeSharedVariable(&svar, number);
		printf("Child Process: writeShared Variable: %d with value: %d\n", svar, number);

		exit();
	} else if (ret != -1) {
		number = -5678;
		sleep(64);

		writeSharedVariable(&svar, number);
		printf("Parent Process: writeShared Variable: %d with value: %d\n", svar, number);
		sleep(128);

		number = readSharedVariable(&svar);
		printf("Parent Process: readShared Variable: %d get value: %d\n", svar, number);
		sleep(128);

		destroySharedVariable(&svar);
		printf("Parent Process: destroyShared Variable: %d\n", svar);
		exit();
	}

	// For lab4.4
	// TODO: You need to design and test the problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.

	return 0;
}
