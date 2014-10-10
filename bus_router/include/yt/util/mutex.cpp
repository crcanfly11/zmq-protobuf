#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/sem.h>
#include <yt/util/mutex.h>

namespace yt
{
	ThreadMutex::ThreadMutex()
	{
		pthread_mutex_init(&mutex_, NULL);
	}
	ThreadMutex::~ThreadMutex()
	{
		pthread_mutex_destroy(&mutex_);
	}
	int ThreadMutex::Acquire(bool block)
	{
		if ( block ) {
			return pthread_mutex_lock(&mutex_);
		}
		else {
			return pthread_mutex_trylock(&mutex_);
		}
	}
	int ThreadMutex::Release()
	{
		return pthread_mutex_unlock(&mutex_);
	}
	NullMutex * NullMutex::Instance()
	{
		static NullMutex oNullMutex;
		return &oNullMutex;
	}
	SemMutex::SemMutex(key_t key, int oflag)
	{
		if ( (semid_ = semget(key, 1, oflag | IPC_CREAT | IPC_EXCL)) >= 0 ) {
			semctl(semid_, 0, SETVAL, 1);
			Acquire();      // init seminfo.sem_otime
			Release();
		}
		else if ( errno == EEXIST ) {
			semid_ = semget(key, 0, oflag);
			struct semid_ds seminfo;			
			while (1) {
				semctl(semid_, 0, IPC_STAT, &seminfo);
				if ( seminfo.sem_otime != 0 ) {
					break;
				}
				usleep(20);
			}
		}

		return;
	}
	int SemMutex::Acquire(bool block)
	{
		struct sembuf op_on;
		op_on.sem_num = 0;
		op_on.sem_op = -1;
		op_on.sem_flg = SEM_UNDO;
		if ( block == false ) {
			op_on.sem_flg |= IPC_NOWAIT;
		}

		while (semop(semid_, &op_on, 1) < 0 ) {
			if (errno == EINTR) {	
				continue;
			}
			else {
				return -1;
			}
		}
		return 0;
	}
	int SemMutex::Release()
	{
		struct sembuf op_on;
		op_on.sem_num = 0;
		op_on.sem_op = 1;
		op_on.sem_flg = SEM_UNDO;

		while (semop(semid_, &op_on, 1) < 0 ) {
			if (errno == EINTR) {	
				continue;
			}
			else {
				return -1;
			}
		}
		return 0;		
	}
}

