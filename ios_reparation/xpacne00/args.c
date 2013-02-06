/***************************************************************
* Subject: IOS                                                 *
* file:    args.c                                              *
* Author:  Jan Pacner                                          *
*          1BIT, krouzek 33                                    *
*          xpacne00@stud.fit.vutbr.cz                          *
* Name:    Projekt č. 2 - Spici holic                          *
* Date:    2011-04-08 15:14:55 CEST                            *
***************************************************************/

//#include <math.h>    /* trunc(), fabs() */
#include <limits.h>  /* UINT_MAX */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>  /* strtol() */
#include <string.h>  /* strcmp() */
#include "args.h"

/*
 * help message
 */
/*
const char *hlpmsg =
  "PROGRAM\n"
  "    Spici holic\n"
  "\n"
  "DESCRIPTION\n"
	"    A modified variant of the synchronization problem named\n"
	"    \"A sleeping barber\".\n"
  "    For exact description please see\n"
  "    http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/projekt2/projekt2.html\n"
  "\n"
  "USAGE\n"
	"    projekt2 Q GenC GenB N (filename|-)\n"
	"      Q    number of chairs\n"
	"      GenC range for customer generation [ms]\n"
	"      GenB range for deal period [ms]\n"
	"      N    count of processes to create (and then destroy)\n"
	"      F    name of file for output data\n"
	"      -    print output to stdin\n"
  "\n"
  "AUTHOR\n"
  "    Jan Pacner\n"
  "    xpacne00@stud.fit.vutbr.cz\n";
*/

/*
 * error messages
 * (must be synchronized with "t_state" enum)
 */
const char *errmsg[] =
{
  "the programmer has made some mistake\n",  // ERR_OK
  "unknown argument\n",                      // ERR_ARG_UNKNOWN
  "too much arguments\n",                    // ERR_ARG_TOO_MUCH
  "not enough arguments\n",                  // ERR_ARG_NOT_ENOUGH
  "number is out of range\n",                // ERR_NUM_OUT_OF_RANGE
  "number is not valid\n",                   // ERR_NUM_NOT_VALID
  "we are out of memory!\n",                 // ERR_OUT_OF_MEM
  "could not read the file:\n",              // ERR_FILE_WRITE
  "could not close the file\n",              // ERR_FILE_CLOSE
  "could not fork new process\n",            // ERR_FORK
  "could not attach shared memory\n",        // ERR_SHM_ATTACH
  "could not detach shared memory\n",        // ERR_SHM_DETACH
  "could not initialize semaphore\n",        // ERR_SEM_INIT
  "the programmer has made some mistake\n",  // ERR_MISC
};

/*
 * print error message
 */
void printErr(t_err err, const char *comment)
{
  /* error message prefix */
  fprintf (stderr, "ERROR: ");

  if (err > ERR_MISC) err = ERR_MISC;

  fprintf(stderr, "%s", errmsg[err]);

  if ((comment != NULL) && (comment[0] != '\0'))
    fprintf(stderr, "\"%s\"\n", comment);

  return;
}

/*
 * parse string to int decimal num
 */
int parseNum(const char *str, int *num)
{
  char *endptr = NULL;
  errno = 0; /* errno reset */

  unsigned long int ulnum = strtoul(str, &endptr, 10);

  if ((errno == ERANGE) || (ulnum > INT_MAX))
  {
    printErr(ERR_NUM_OUT_OF_RANGE, str);
    return ERR_NUM_OUT_OF_RANGE;
  }

  if ((errno != 0 && ulnum == 0) || (endptr == str) ||
    /* str needs to be fully converted */
    (*endptr != '\0'))
  {
    printErr(ERR_NUM_NOT_VALID, str);
    return ERR_NUM_NOT_VALID;
  }

  /* number successfully parsed */
  *num = (int)ulnum;

  return 0;
} /* parseNum() */

/*
 * parses the arguments
 */
int parseArgs(t_arg *args, int argc, char *argv[])
{
  t_err err = ERR_OK;

  /* try to avoid running the slow argument parsing below */
  if (argc > ARG_COUNT)
  {
    printErr(ERR_ARG_TOO_MUCH, "");
    return ERR_ARG_TOO_MUCH;
  }
  if (argc < ARG_COUNT)
  {
    printErr(ERR_ARG_NOT_ENOUGH, "");
    return ERR_ARG_NOT_ENOUGH;
  }

  /* program name */
  args->path = argv[0];

	if (
      (err = parseNum(argv[1], &args->q   ) != ERR_OK) ||  /* Q    */
      (err = parseNum(argv[2], &args->genc) != ERR_OK) ||  /* GenC */
      (err = parseNum(argv[3], &args->genb) != ERR_OK) ||  /* GenB */
      (err = parseNum(argv[4], &args->n   ) != ERR_OK)     /* N    */
     )
	{
    printf("%s", "");
		return err;
	}

  if (argv[5][0] == '\0')
  {
    printErr(ERR_ARG_UNKNOWN, "");
    return ERR_ARG_NOT_ENOUGH;
  }
  else if ((argv[5][0] != '-') || (argv[5][1] != '\0'))
  {
    args->file = argv[5];
  }

  return ERR_OK;
} /* parseArgs() */

