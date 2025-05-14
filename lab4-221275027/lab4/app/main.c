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


static unsigned int next = 1;
int rand() {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}
void reader_writer(){
	// 多个读者进程可以同时读取共享变量，但写者进程必须独占访问。
	// 需要实现两种同步策略：
	// 	1.读者优先（Reader-Priority）：只要还有读者在读，写者就必须等待。
	// 	2.读写公平（Fair）：避免读者或写者无限等待，确保公平竞争。
	// Tips: 你需要实现SharedVariable以及相应的操作函数，然后完成本问题。


	// // 共享变量初始化
	// static sharedvar_t shared_data = -1;
	// static int initialized = 0;
	
	// // 同步变量 - 读者优先策略
	// static sem_t mutex = -1;      // 保护读者计数器
	// static sem_t write_lock = -1; // 写者锁
	// static int reader_count = 0;  // 当前读者数量
	
	// // 同步变量 - 公平策略
	// static sem_t queue = -1;      // 公平队列
	// static sem_t r_mutex = -1;    // 读者计数器保护
	// // static int r_count = 0;       // 读者计数器
	// static sem_t w_mutex = -1;    // 写者计数器保护 
	// static int w_count = 0;       // 写者计数器
	
	// // 初始化同步变量
	// if (!initialized) {
	// 	createSharedVariable(&shared_data, 0);
	// 	sem_init(&mutex, 1);
	// 	sem_init(&write_lock, 1);
	// 	sem_init(&queue, 1);
	// 	sem_init(&r_mutex, 1);
	// 	sem_init(&w_mutex, 1);
	// 	initialized = 1;
	// }
	
	// // 获取进程ID用于区分读者/写者
	// int id = fork();
	// if (id == 0) {
	// 	// 子进程 - 读者
	// 	while (1) {
	// 		// 读者优先策略
	// 		sem_wait(&mutex);
	// 		reader_count++;
	// 		if (reader_count == 1) {
	// 			sem_wait(&write_lock);
	// 		}
	// 		sem_post(&mutex);
			
	// 		// 读取共享变量
	// 		int value = readSharedVariable(&shared_data);
	// 		printf("Reader %d: read value %d\n", id, value);
	// 		sleep(64);
			
	// 		// 读者优先策略
	// 		sem_wait(&mutex);
	// 		reader_count--;
	// 		if (reader_count == 0) {
	// 			sem_post(&write_lock);
	// 		}
	// 		sem_post(&mutex);
			
	// 		sleep(128);
	// 	}
	// } else {
	// 	// 父进程 - 写者
	// 	while (1) {
	// 		// 公平策略
	// 		sem_wait(&queue);
	// 		sem_wait(&w_mutex);
	// 		w_count++;
	// 		if (w_count == 1) {
	// 			sem_wait(&r_mutex);
	// 		}
	// 		sem_post(&w_mutex);
	// 		sem_post(&queue);
			
	// 		sem_wait(&write_lock);
			
	// 		// 写入共享变量
	// 		int new_value = rand() % 100;
	// 		writeSharedVariable(&shared_data, new_value);
	// 		printf("Writer: wrote value %d\n", new_value);
	// 		sleep(128);
			
	// 		sem_post(&write_lock);
			
	// 		// 公平策略
	// 		sem_wait(&w_mutex);
	// 		w_count--;
	// 		if (w_count == 0) {
	// 			sem_post(&r_mutex);
	// 		}
	// 		sem_post(&w_mutex);
			
	// 		sleep(256);
	// 	}
	// }


	// 共享变量初始化
    static sharedvar_t shared_data = -1;
    static int initialized = 0;
    
    // 同步变量
    static sem_t mutex = -1;      // 保护读者计数器
    static sem_t write_lock = -1; // 写者锁
    static int reader_count = 0;   // 当前读者数量
    
    // 初始化同步变量
    if (!initialized) {
        createSharedVariable(&shared_data, 0);
        sem_init(&mutex, 1);
        sem_init(&write_lock, 1);
        initialized = 1;
    }
    
    // 创建读者进程
    int reader_id = 0;
    for (int i = 0; i < 3; i++) { // 创建3个读者
        if (fork() == 0) {
            reader_id = i + 1; // 读者ID从1开始
            break;
        }
    }
    
    if (reader_id > 0) {
        // 读者进程
        while (1) {
            sem_wait(&mutex);
            reader_count++;
            if (reader_count == 1) {
                sem_wait(&write_lock);
            }
            sem_post(&mutex);
            
            // 读取共享变量
            int value = readSharedVariable(&shared_data);
            printf("Reader %d: read value %d\n", reader_id, value);
            sleep(64);
            
            sem_wait(&mutex);
            reader_count--;
            if (reader_count == 0) {
                sem_post(&write_lock);
            }
            sem_post(&mutex);
            
            sleep(128);
        }
    } else {
        // 写者进程
        while (1) {
            sem_wait(&write_lock);
            
            // 写入共享变量
            int new_value = rand() % 100;
            writeSharedVariable(&shared_data, new_value);
            printf("Writer: wrote value %d\n", new_value);
            sleep(128);
            
            sem_post(&write_lock);
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

	// printf("==============TEST READER_WRITER PROB=============\n");
	// reader_writer();
	// printf("============================================\n");

	return 0;
}
