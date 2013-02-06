/***************************************************************
* Subject: IOS                                                 *
* file:    projekt2.c                                          *
* Author:  Jan Pacner                                          *
*          1BIT, krouzek 33                                    *
*          xpacne00@stud.fit.vutbr.cz                          *
* Name:    Projekt č. 2 - Spici holic                          *
* Date:    2011-04-08 15:14:55 CEST                            *
***************************************************************/

/* FIXME There is no checking for error return values in the forked processes
   because of it's complexity to synchronize these processes afterwards.
   These checks are useless for such a school project and make the code
   far more worst readable. */

/* FIXME I've made a choice of using the "real" queue instead of using
   signals for transfering data (PIDs) between processes. It's because
   I wanted to try using the shared memory for complex tasks. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>     /* strtol() */
#include <signal.h>     /* signal macros */
#include <sys/wait.h>   /* waitpid() */
#include <sys/stat.h>   /* stat(), access rights consts */
#include <sys/types.h>  /* kill(), ftok() */
#include <sys/shm.h>    /* shmget() */
#include <sys/ipc.h>    /* ftok() */
#include <unistd.h>     /* getpid() */
#include <sys/sem.h>    /* semget(), semctl(), semop() */
#include <errno.h>
#include "args.h"       /* args, error states (and messages) handling */

/*
 * semaphore ID, used by all processes
 */
int semid;

/*
 * semaphore union
 */
union semuni
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

/*
 * wrapper for sleep() and usleep()
 * @param time in miliseconds
 * @return nothing
 */
void msleep(const int t)
{
  int tmp;

  if ((tmp = t/1000) > 0) sleep(tmp);

  if ((tmp = t%1000) > 0) usleep(tmp);

  return;
}

/*
 * shared memory IDs structure (we can use globar variables, but
 * this seems to be easy to take in)
 */
struct st_shmid
{
  int action_no;      /* global action counter               */
  int barber_sleeps;  /* global indicator of sleeping barber */
  int record;         /* queue structure                     */
  int buf;            /* queue buffer                        */
};

/*
 * queue structure
 */
struct st_queue
{
  int max;      /* queue size */
  pid_t *buf;   /* data       */
  int pos_st;   /* head       */
  int pos_ret;  /* tail       */
};

/*
 * enqueue given PID
 * @param pointer to the queue structure
 * @param key (PID)
 * @return true if the PID was enqueued, otherwise false
 */
bool queue_st(struct st_queue *record, pid_t key)
{
  if (record->pos_st == record->max - 1) return false;

  record->pos_st++;
  record->buf[record->pos_st] = key;

  if (record->pos_ret == -1) record->pos_ret++;

  return true;
}

/*
 * dequeue given PID
 * @param pointer to the queue structure
 * @return PID if the queue wasn't empty, otherwise 0
 */
pid_t queue_ret(struct st_queue *record)
{
  /* 0 indicates empty queue */
  if (record->pos_ret == -1) return 0;

  pid_t key = record->buf[record->pos_ret];

  if (record->pos_ret == record->pos_st)
  {
    record->pos_ret = -1;
    record->pos_st = -1;
  }
  else
  {
    record->pos_ret++;
  }

  return key;
}

/*
 * write certain messages to the given FILE
 * @param NULL for stdout, otherwise pointer to opened FILE
 * @param string to shout
 * @param ID of the caller
 * @parma pointer to the global action counter
 * @return nothing
 */
int shout(const t_arg *args, const char *str, const int id, int *action_no)
{
  FILE *fw = NULL;

  /* we use NULL as idicator of stdout */
  if (args->file == NULL)
  {
    fw = stdout;
  }
  else
  {
    /* open file for writing */
    if ((fw = fopen(args->file, "a")) == NULL)
    {
      printErr(ERR_FILE_WRITE, args->file);
      return ERR_FILE_WRITE;
    }
  }

  if (id == 0)
    fprintf(fw,"%d: barber: %s\n",      ++(*action_no), str);
  else
    fprintf(fw,"%d: customer %d: %s\n", ++(*action_no), id, str);

  if (fw != stdout) fclose(fw);

  return 0;
}

/*
 * semaphore lock (wait)
 * @param nothing
 * @return nothing
 */
void lock()
{
  /* array of structures for semop() */
  struct sembuf sops[1];

  /* use the first semaphore (with index 0) */
  sops[0].sem_num = 0;

  /* decrement by 1 */
  sops[0].sem_op = -1;

  /* post to semaphore (undo the operation) when the process get
     terminated abnormally prior to posting the semaphore */
  sops[0].sem_flg = SEM_UNDO;

  /* wait */
  semop(semid, sops, 1);

  return;
}

/*
 * semaphore unlock (post)
 * @param nothing
 * @return nothing
 */
void unlock()
{
  struct sembuf sops[1];

  sops[0].sem_num = 0;
  sops[0].sem_op = 1;  /* increment by 1 */
  sops[0].sem_flg = SEM_UNDO;

  /* post */
  semop(semid, sops, 1);

  return;
}

/*
 * attach shm record and barber_sleeps, and construct the queue structure
 * @param double pointer to the queue structure, where to save the built one
 * @parma double pointer to the barber_sleeps var
 * @param structure with shm IDs
 * @return nothing
 */
void shmatt_record_barber_sleeps(struct st_queue **record,
  bool **barber_sleeps, const struct st_shmid *shmid)
{
  *barber_sleeps = (bool *)shmat(shmid->barber_sleeps, NULL, 0);

  *record = (struct st_queue *)shmat(shmid->record, NULL, 0);
  (*record)->buf = (pid_t *)shmat(shmid->buf, NULL, 0);

  return;
}

/*
 * detach shm record and barber_sleeps
 * @param pointer to the queue structure, which to detach
 * @prama poniter to the barber_sleeps var, which to detach
 * @return nothing
 */
void shmdet_record_barber_sleeps(struct st_queue *record,
  bool *barber_sleeps)
{
  shmdt((void *)record->buf);
  shmdt((void *)record);

  shmdt((void *)barber_sleeps);

  return;
}

/*
 * pause until SIGUSR- arrives and if needed, send given
 *   signal and switch context before starting wait
 * @param signal number to send to the given PID
 * @param PID of process to which send the signal
 *        -1 means do not send anything
 * @param signal number to wait for
 * return nothing
 */
void send_wait_signal(const int sigsend, const pid_t pid, const int sigwait)
{
  sigset_t mask_new, mask_old;

  /* initialize with all sigs excluded */
  sigemptyset(&mask_new);
  sigemptyset(&mask_old);

  /* add the signal to wait for to the mask */
  sigaddset(&mask_new, sigwait);

  /* save the current mask and start enqueing all mask_new sigs */
  sigprocmask(SIG_BLOCK, &mask_new, &mask_old);

  if (pid != -1)
  {
    /* send signal */
    kill(pid, sigsend);
  }

  unlock();

  /* replace the current mask and release blocked sigs */
  sigsuspend(&mask_old);

  /* unblock all (currently blocked) mask_new sigs */
  sigprocmask(SIG_UNBLOCK, &mask_new, NULL);

  return;
}

/*
 * simulate barber as a process
 * @param file where to write messages
 * @param structure with shared memory IDs
 * @param structure with parsed arguments
 * return nothing
 */
void process_barber(const struct st_shmid *shmid, const t_arg *args)
{
  pid_t pid = 0;
  int *action_no = NULL;
  struct st_queue *record = NULL;
  bool *barber_sleeps = NULL;

  /* generate pseudo-random sequence */
  srand(getpid());

  /* iterate while not retrieve the SIGKILL */
  for (;;)
  {
    lock();

    /* attach needed shm segments */
    action_no = (int *)shmat(shmid->action_no, NULL, 0);

    shout(args, "checks", 0, action_no);

    /* detach needed shm segments */
    shmdt((void *)action_no);

    /* attach needed shm segments */
    shmatt_record_barber_sleeps(&record, &barber_sleeps, shmid);

    /* pick the new customer if any, evaluate max 2x, iterate max 1x */
    while ((pid = queue_ret(record)) == 0)
    {
      *barber_sleeps = true;

      /* detach needed shm segments */
      shmdet_record_barber_sleeps(record, barber_sleeps);

      /* wait for awake */
      send_wait_signal(0, -1, SIGUSR2);

      /* needed for condition evaluation in next iteration */
      lock();

      shmatt_record_barber_sleeps(&record, &barber_sleeps, shmid);
    }

    shmdet_record_barber_sleeps(record, barber_sleeps);

    action_no = (int *)shmat(shmid->action_no, NULL, 0);
    shout(args, "ready", 0, action_no);
    shmdt((void *)action_no);

    /* inform customer I'm ready and wait for answer */
    send_wait_signal(SIGUSR1, pid, SIGUSR2);

    /* barber cuts the customer */
    if (args->genb > 0) msleep(rand()%args->genb);

    lock();
    action_no = (int *)shmat(shmid->action_no, NULL, 0);
    shout(args, "finished", 0, action_no);
    shmdt((void *)action_no);
    unlock();

    /* inform customer he is cut */
    kill(pid, SIGUSR1);
  } /* for(;;) */

  return;
}

/*
 * simulate customer as a process
 * @param file where to write messages
 * @param structure with shared memory IDs
 * @param barber's PID
 * @param id of current process (customer)
 * @return nothing
 */
void process_customer(const struct st_shmid *shmid, const t_arg *args,
  const pid_t barber_pid, const int id)
{
  int *action_no = NULL;
  struct st_queue *record = NULL;
  bool *barber_sleeps = NULL;

  lock();
  action_no = (int *)shmat(shmid->action_no, NULL, 0);
  shout(args, "created", id, action_no);
  shmdt((void *)action_no);
  unlock();

  lock();
  shmatt_record_barber_sleeps(&record, &barber_sleeps, shmid);

  if (queue_st(record, getpid()) == true)
  {
    action_no = (int *)shmat(shmid->action_no, NULL, 0);
    shout(args, "enters", id, action_no);
    shmdt((void *)action_no);

    if (*barber_sleeps == true)
    {
      *barber_sleeps = false;

      /* wake up barber */
      kill(barber_pid, SIGUSR2);
    }

    shmdet_record_barber_sleeps(record, barber_sleeps);

    /* wait until barber is ready */
    send_wait_signal(0, -1, SIGUSR1);

    lock();
    action_no = (int *)shmat(shmid->action_no, NULL, 0);
    shout(args, "ready", id, action_no);
    shmdt((void *)action_no);

    /* inform barber, I'm ready and wait until I'm cut */
    send_wait_signal(SIGUSR2, barber_pid, SIGUSR1);

    lock();
    action_no = (int *)shmat(shmid->action_no, NULL, 0);
    shout(args, "served", id, action_no);
    shmdt((void *)action_no);
    unlock();
  }
  else
  {
    action_no = (int *)shmat(shmid->action_no, NULL, 0);
    shout(args, "refused", id, action_no);
    shmdt((void *)action_no);

    shmdet_record_barber_sleeps(record, barber_sleeps);
    unlock();
  }

  return;
}

/*
 * deallocate shared memory in the given structure of IDs
 * @param structure with shm IDs
 * @return nothing
 */
void ungetshm(struct st_shmid *shmid)
{
  if (shmid->buf           != -1) shmctl(shmid->buf,           IPC_RMID, NULL);
  if (shmid->record        != -1) shmctl(shmid->record,        IPC_RMID, NULL);
  if (shmid->barber_sleeps != -1) shmctl(shmid->barber_sleeps, IPC_RMID, NULL);
  if (shmid->action_no     != -1) shmctl(shmid->action_no,     IPC_RMID, NULL);

  return;
}

/*
 * allocate shared memory and save IDs to the given structure
 *   in case of ERR, deallocate the allocated memory
 * @param structure with shm IDs
 * @param structure with parsed arguments
 * @return 0 if OK, otherwise some ERR
 */
int getshm(struct st_shmid *shmid, const t_arg *args)
{
  /* init the structure with shared memory segments */
  shmid->action_no     = -1;
  shmid->barber_sleeps = -1;
  shmid->buf           = -1;
  shmid->record        = -1;

  errno = 0;
  /* allocate shm segment and initialize it with 0 */
  if (
      /* int action_no */
      ((shmid->action_no = shmget(ftok(args->path, 'a'), sizeof(int),
         IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) ||

      /* bool barber_sleeps */
      ((shmid->barber_sleeps = shmget(ftok(args->path, 'b'), sizeof(bool),
         IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) ||

      /* struct st_queue record */
      ((shmid->record = shmget(ftok(args->path, 'd'), sizeof(struct st_queue),
         IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) ||

      /* pid_t *buf */
      (args->q > 0) ?  /* shmget() returns error if the size is 0 */
      ((shmid->buf = shmget(ftok(args->path, 'c'), args->q * sizeof(pid_t),
         IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0)
      : false
     )
  {
    ungetshm(shmid);
    printErr(ERR_OUT_OF_MEM, "");
    return ERR_OUT_OF_MEM;
  }

  return 0;
}

/*
 * function doing nothing
 * @param integer number
 * @return nothing
 */
void void_fn(const int integer)
{
  /* avoid compilator throwing warnings */
  (void) integer;

  return;
}

/*
 * main thread/process (parent)
 * @param structure with parsed arguments
 * @return 0 if OK, otherwise some error
 */
int general(const t_arg *args)
{
  t_err err = ERR_OK;
  pid_t *pid = NULL;      /* array with PIDs */
  struct st_shmid shmid;  /* struct with shared memory segments */

  /* get mem for array of PIDs */
  if ((pid = calloc((args->n + 1), sizeof(pid_t))) == NULL)
  {
    printErr(ERR_OUT_OF_MEM, "");
    return ERR_OUT_OF_MEM;
  }

  /* get shared memory and save IDs to the structure */
  if ((err = getshm(&shmid, args)) != ERR_OK)
  {
    free(pid);
    return err;
  }

  /* reset errno */
  errno = 0;

  /* mount shm segment */
  struct st_queue *record = (struct st_queue *)shmat(shmid.record, NULL, 0);

  if (errno)
  {
    shmdt((void *)record);
    ungetshm(&shmid);
    free(pid);
    printErr(ERR_SHM_ATTACH, "shmid.record");
    return ERR_SHM_ATTACH;
  }

  record->max     = args->q; /* chairs in waiting room */
  record->pos_st  = -1;      /* reserved value */
  record->pos_ret = -1;      /* reserved value */

  /* unmount shm segment */
  shmdt((void *)record);

  /* create 1 semaphore with rw rights for the creator */
  if ((semid = semget(ftok(args->path, 'z'), 1,
                      IPC_CREAT | IPC_EXCL | 0600)) == -1)
  {
    shmdt((void *)record);
    ungetshm(&shmid);
    free(pid);
    printErr(ERR_SEM_INIT, "");
    return ERR_SEM_INIT;
  }

  /* semaphore initialization */
  unsigned short int semset[1];  /* use 1 semaphore in set */
  semset[0] = 1;                 /* semaphore "index" number (not ID) */

  union semuni arg =
  {
    .array = semset,
  };

  /* set semaphore options in the whole semset */
  semctl(semid, 0, SETALL, arg);

  /* reset user signals (childs "inherit") */
  sigset_t block_mask;      /* create new mask */
  sigfillset(&block_mask);  /* fulfill with every signal */

  struct sigaction sigact =
  {
    .sa_handler = void_fn,  /* do nothing */
    .sa_mask    = block_mask,
    .sa_flags   = 0,
  };

  sigaction(SIGUSR1, &sigact, NULL);  /* apply the new structure */
  sigaction(SIGUSR2, &sigact, NULL);  /* apply the new structure */

  /* generate pseudo-random sequence */
  srand(getpid());

  int i = 0;
  int j = args->n;

  /* give birth to the barber and all customers */
  for (i = 0; i <= args->n; i++)
  {
    /* sleep for a few miliseconds */
    if (i != 0 && args->genc > 0) msleep(rand()%args->genc);

    /* child */
    if ((pid[i] = fork()) == 0)
    {
      if (i == 0)
      {
        process_barber(&shmid, args);
        free(pid);
        exit(0); /* exit child */
      }
      else
      {
        process_customer(&shmid, args, pid[0], i);
        free(pid);
        exit(0); /* exit child */
      }
    }
    /* something bad happened */
    else if (pid[i] == -1)
    {
      err = ERR_FORK;

      for (j = 1; j < i; j++)
        kill(pid[j], SIGKILL);  /* no more interest in the return value */

      break;
    }
  } /* for(;;) */

  /* wait until all customers are gone */
  for (i = 1; i <= j; i++)
  {
    if (pid[i] != 0) waitpid(pid[i], NULL, 0);
  }

  /* close the barbershop forever and kill the barber */
  kill(pid[0], SIGKILL);

  /* wait until barber definitely dies */
  waitpid(pid[0], NULL, 0);

  /* remove semaphore */
  semctl(semid, 1, IPC_RMID, arg);

  /* deallocate shm segments */
  ungetshm(&shmid);

  /* deallocate pid array */
  free(pid);

  return err;  /* exit parent */
}

int main(int argc, char *argv[])
{
  t_err errnum = ERR_OK;

  /* init the argument structure */
  t_arg args =
  {
    .q    = 0,
    .genc = 0,
    .genb = 0,
    .n    = 0,
    .path = NULL,
    .file = NULL,  /* NULL means output to stdout (instead of file) */
  };

  /* transform arguments into structure in memory
     and check them for compatibility */
  if ((errnum = parseArgs(&args, argc, argv)) != ERR_OK)
    return errnum;

  if ((errnum = general(&args)) != ERR_OK)
    return errnum;

  return EXIT_SUCCESS;
} // main()
