#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include "a2_helper.h"

sem_t logSem;
sem_t sem_waiting_P2_T12;
sem_t sem_procede_P2_T12;
sem_t sem_procede_waiting_threads;

sem_t *sem_P9_T4;
sem_t *sem_P5_T1;

pthread_mutex_t lock;

int running_threads;
int P2_T12_completed;

void* p9_t_create(void* thread_number){
	int threadNo = (int)(long) thread_number;

	if(threadNo == 3){
		info(BEGIN, 9, threadNo);

		pthread_t tid2;
		pthread_create(&tid2, NULL, p9_t_create, (void*)(long)2);
		pthread_join(tid2, NULL);

		info(END, 9, threadNo);
	}
	else if(threadNo == 4){
		sem_wait(sem_P9_T4);

		info(BEGIN, 9, threadNo);
		info(END, 9, threadNo);

		sem_post(sem_P5_T1);
	}
	else{
		info(BEGIN, 9, threadNo);
		info(END, 9, threadNo);
	}

	return NULL;
}

void p9_t(){
	pthread_t tid1, tid3, tid4;

	pthread_create(&tid1, NULL, p9_t_create, (void*)(long)1);
	pthread_create(&tid3, NULL, p9_t_create, (void*)(long)3);
	pthread_create(&tid4, NULL, p9_t_create, (void*)(long)4);

	pthread_join(tid1, NULL);
	pthread_join(tid3, NULL);
	pthread_join(tid4, NULL);
}

void* p2_t_create(void* thread_number){
	int threadNo = (int)(long) thread_number;
	sem_wait(&logSem);

	if(P2_T12_completed){
		info(BEGIN, 2, threadNo);
		info(END, 2, threadNo);
	}
	else if (threadNo == 12){
		info(BEGIN, 2, threadNo);

		sem_post(&sem_waiting_P2_T12);
		sem_wait(&sem_procede_P2_T12);
		P2_T12_completed = 1;

		info(END, 2, threadNo);

		for (int i = 0; i < 5; ++i){
			sem_post(&sem_procede_waiting_threads);
		}
	}
	else{
		pthread_mutex_lock(&lock);

		info(BEGIN, 2, threadNo);
		running_threads ++;

		if(running_threads < 5){
			pthread_mutex_unlock(&lock);
			sem_wait(&sem_procede_waiting_threads);
		}
		else{
			pthread_mutex_unlock(&lock);
			sem_post(&sem_procede_P2_T12);
			sem_wait(&sem_procede_waiting_threads);
		}

		info(END, 2, threadNo);
	}

	sem_post(&logSem);

	return NULL;
}

void p2_t(){
	pthread_t tids[39];
	sem_init(&logSem, 0, 6);
	sem_init(&sem_procede_P2_T12, 0, 0);
	sem_init(&sem_waiting_P2_T12, 0, 0);
	sem_init(&sem_procede_waiting_threads, 0, 0);

	pthread_mutex_init(&lock, NULL);

	pthread_create(&tids[11], NULL, p2_t_create, (void*)(long)12);

	sem_wait(&sem_waiting_P2_T12);

	for (int i = 0; i < 39; ++i){
		if(i != 11){
			pthread_create(&tids[i], NULL, p2_t_create, (void*)(long)(i + 1));
		}
	}

	for (int i = 0; i < 39; ++i){
			pthread_join(tids[i], NULL);
	}

	sem_destroy(&logSem);
	sem_destroy(&sem_procede_P2_T12);
	sem_destroy(&sem_waiting_P2_T12);
	sem_destroy(&sem_procede_waiting_threads);

	pthread_mutex_destroy(&lock);
}

void* p5_t_create(void* thread_number){
	int threadNo = (int)(long) thread_number;

	if(threadNo == 1){
		sem_wait(sem_P5_T1);
		info(BEGIN, 5, threadNo);
		info(END, 5, threadNo);
	}
	else if(threadNo == 2){
		info(BEGIN, 5, threadNo);
		info(END, 5, threadNo);
		sem_post(sem_P9_T4);
	}
	else{
		info(BEGIN, 5, threadNo);
		info(END, 5, threadNo);
	}

	return NULL;
}

void p5_t(){
	pthread_t tids[5];

	for (int i = 0; i < 5; ++i){
		pthread_create(&tids[i], NULL, p5_t_create, (void*)(long)(i + 1));
	}

	for (int i = 0; i < 5; ++i){
		pthread_join(tids[i], NULL);
	}
}

void problem(){
	sem_P5_T1 = sem_open("/sem_P5_T1", O_CREAT, 0666, 0);
	sem_P9_T4 = sem_open("/sem_P9_T4", O_CREAT, 0666, 0);

	info(BEGIN, 1, 0);

	pid_t pid;
	pid = fork();

	if(pid == 0){
		info(BEGIN, 2, 0);

		pid_t pid3;
		pid3 = fork();

		if(pid3 == 0){
			info(BEGIN, 3, 0);

			pid_t pid6;
			pid6 = fork();

			if(pid6 == 0){
				info(BEGIN, 6, 0);
				info(END, 6, 0);
			}
			else{
				wait(NULL);
				info(END, 3, 0);
			}
		}
		else{
			pid_t pid4;
			pid4 = fork();

			if(pid4 == 0){
				info(BEGIN, 4, 0);
				pid_t pid7;
				pid7 = fork();

				if(pid7 == 0){
					info(BEGIN, 7, 0);
					info(END, 7, 0);
				}
				else{
					wait(NULL);
					info(END, 4, 0);
				}
			}
			else{
				pid_t pid5;
				pid5 = fork();

				if(pid5 == 0){
					info(BEGIN, 5, 0);

					p5_t();

					info(END, 5, 0);
				}
				else{
					pid_t pid8;
					pid8 = fork();

					if(pid8 == 0){
						info(BEGIN, 8, 0);
						info(END, 8, 0);
					}
					else{
						p2_t();

						wait(NULL);
						wait(NULL);
						wait(NULL);
						wait(NULL);
						
						info(END, 2, 0);
					}
				}
			}
		}
	}
	else{
		pid_t pid9;
		pid9 = fork();

		if(pid9 == 0){
			info(BEGIN, 9, 0);

			p9_t();

			info(END, 9, 0);
		}
		else{
			wait(NULL);
			wait(NULL);

			info(END, 1, 0);
		}
	}

	sem_close(sem_P5_T1);
	sem_close(sem_P9_T4);

	sem_unlink("/sem_T5_P1");
	sem_unlink("/sem_P9_T4");
}

int main(){
	init();

	problem();

	return 0;
}