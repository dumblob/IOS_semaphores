#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/* When a SIGUSR1 signal arrives, set this variable. */
volatile sig_atomic_t usr_interrupt = 0;

// zavola se po prijeti signalu SIGUSR1
void synch_signal (int sig) {
  usr_interrupt = 1;
}

/* The child process executes this function. */
void child_function (void) {
  ...
  /* Let the parent know you’re here. */
  kill (getppid(), SIGUSR1);
  ...
}

int main (void) {
  struct sigaction usr_action;
  sigset_t block_mask, old_mask;
  pid_t pid;

  // nastav reakci na signal SIGUSR1
  sigfillset (&block_mask);
  usr_action.sa_handler = synch_signal;
  usr_action.sa_mask = block_mask;
  usr_action.sa_flags = 0;
  sigaction (SIGUSR1, &usr_action, NULL);

  if ((pid = fork()) == 0)
    child_function ();
  else if (pid==-1)
  {
    /* A problem with fork - exit. */
  }

  // pockej az dite posle signal SIGUSR1
  sigemptyset (&block_mask);
  sigaddset (&block_mask, SIGUSR1);
  sigprocmask (SIG_BLOCK, &block_mask, &old_mask);

  while (!usr_interrupt) sigsuspend (&old_mask);
  sigprocmask (SIG_UNBLOCK, &block_mask, NULL);
  ...

