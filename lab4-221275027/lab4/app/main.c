#include "lib.h"
#include "types.h"

void producer_customer() {
	int i = 0;
	int id = 0;
	sem_t empty, full, mutex;
	sem_init(&empty, 5);
	sem_init(&full, 0);
	sem_init(&mutex, 1);
	for (i = 0; i < 4; i++) {
		if (id == 0) id = fork();
		else if (id > 0) break;
	}
	while (1) {
		if (id == 0){
			sem_wait(&full);
			sem_wait(&mutex);
			printf("Consumer : consume\n");
			sleep(128);
			sem_post(&mutex);
			sem_post(&empty);
		}
		else{
			sem_wait(&empty);
			sem_wait(&mutex);
			printf("Producer %d: produce\n", id - 1);
			sleep(128);
			sem_post(&mutex);
			sem_post(&full);
		}
	}
}


// 根据伪代码写的，按照要求应该是两个策略，这个函数没用了
// void reader_writer() {
// 	// 初始化共享变量和信号量
// 	sharedvar_t shared_data;
// 	sem_t write_mutex, count_mutex;
// 	int reader_count = 0;

// 	sem_init(&write_mutex, 1);  // 控制读写互斥
// 	sem_init(&count_mutex, 1);  // 控制对读者计数的互斥修改
// 	createSharedVariable(&shared_data, 0);

// 	// 创建读者进程
// 	if (fork() == 0) {
// 		// 读者进程
// 		while (1) {
// 			sem_wait(&count_mutex);
// 			if (reader_count == 0) {
// 				sem_wait(&write_mutex);
// 			}
// 			reader_count++;
// 			sem_post(&count_mutex);

// 			// 读操作
// 			int value = readSharedVariable(&shared_data);
// 			printf("Reader: read value = %d\n", value);
// 			sleep(64);

// 			sem_wait(&count_mutex);
// 			reader_count--;
// 			if (reader_count == 0) {
// 				sem_post(&write_mutex);
// 			}
// 			sem_post(&count_mutex);
			
// 			sleep(128);
// 		}
// 	} 
// 	else {
// 		// 写者进程
// 		while (1) {
// 			sem_wait(&write_mutex);
			
// 			// 写操作
// 			int value = readSharedVariable(&shared_data);
// 			value++;
// 			writeSharedVariable(&shared_data, value);
// 			printf("Writer: wrote value = %d\n", value);
// 			sleep(128);

// 			sem_post(&write_mutex);
// 			sleep(256);
// 		}
// 	}
// }


void reader_priority() {
	
	sem_t mutex, wrt;
	int read_count = 0;
	sharedvar_t shared_var;

	// 初始化信号量和共享变量
	sem_init(&mutex, 1);
	sem_init(&wrt, 1);
	createSharedVariable(&shared_var, 0);

	int id=fork();
	if (id == 0) { // 子进程作为读者
		while (1) {
			
			sem_wait(&mutex);
			read_count++;
			if (read_count == 1) {
				sem_wait(&wrt); // 第一个读者阻止写者
			}
			sem_post(&mutex);
			

			// 读操作
			int value = readSharedVariable(&shared_var);
			printf("Reader: read value = %d\n", value);
			sleep(64);

			
			sem_wait(&mutex);
			read_count--;
			if (read_count == 0) {
				sem_post(&wrt); // 最后一个读者允许写者
			}
			sem_post(&mutex);
			sleep(128);
		}
	} else { // 父进程作为写者
		while (1) {
			sem_wait(&wrt);

			// 写操作
			int value = readSharedVariable(&shared_var);
			value++;
			writeSharedVariable(&shared_var, value);
			printf("Writer: wrote value = %d\n", value);
			sleep(128);

			sem_post(&wrt);
			sleep(256);
		}
	}
   
}

void fair() {
	sem_t mutex, rw, w;
	int readcount = 0;
	sharedvar_t shared_var;

	// 初始化信号量和共享变量
	sem_init(&mutex, 1);
	sem_init(&rw, 1);
	sem_init(&w, 1);
	createSharedVariable(&shared_var, 0);


	int id=fork();
	
	if (id == 0) { // 子进程作为读者
		while (1) {
			sem_wait(&w);
			sem_wait(&mutex);
			if (readcount == 0) {
				sem_wait(&rw);
			}
			readcount++;
			sem_post(&mutex);
			sem_post(&w);

			// 读操作
			int value = readSharedVariable(&shared_var);
			printf("Reader: read value = %d\n", value);
			sleep(64);

			sem_wait(&mutex);
			readcount--;
			if (readcount == 0) {
				sem_post(&rw);
			}
			sem_post(&mutex);
			sleep(128);
		}
	} else { // 父进程作为写者
		while (1) {
			sem_wait(&w);
			sem_wait(&rw);

			// 写操作
			int value = readSharedVariable(&shared_var);
			value++;
			writeSharedVariable(&shared_var, value);
			printf("Writer: wrote value = %d\n", value);
			sleep(128);

			sem_post(&rw);
			sem_post(&w);
			sleep(256);
		}
	}
}


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
			// printf(".. Wait Over: ret = %d\n", ret);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		ret = sem_destroy(&sem);
		// printf(".. Destroy ret = %d\n", ret);
		exit();
	} else if (ret != -1) {
		while (i != 0)
		{
			i--;
			printf("Parent Process: Sleeping.\n");
			sleep(128);
			printf("Parent Process: Semaphore Posting.\n");
			ret = sem_post(&sem);
			// printf(".. Post ret = %d\n", ret);
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
		// exit();
	}

	// For lab4.4
	// TODO: You need to design and test the problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.

	// printf("==============TEST PRO_CUS PROB=============\n");
	// producer_customer();
	// printf("============================================\n");

	// printf("==================TEST READER_PRIORITY PROB=============\n");
	// reader_priority();
	// printf("=====================================================\n");

	// printf("==================TEST FAIR PROB=============\n");
	// fair();
	// printf("=====================================================\n");


	// printf("==============TEST READER_WRITER PROB=============\n");
	// reader_writer();
	// printf("============================================\n");
	
	return 0;
}
