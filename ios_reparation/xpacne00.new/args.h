/***************************************************************
* Subject: IOS                                                 *
* file:    args.h                                              *
* Author:  Jan Pacner                                          *
*          1BIT, krouzek 33                                    *
*          xpacne00@stud.fit.vutbr.cz                          *
* Name:    Projekt č. 2 - Spici holic                          *
* Date:    2011-04-08 15:14:55 CEST                            *
***************************************************************/

#ifndef ARGS_H
#define ARGS_H

// exact count of arguments
#define ARG_COUNT 6

/*
 * structure containing parsed cmd arguments
 */
typedef struct
{
  int q;
  int genc;
  int genb;
  int n;
  char *path;
  char *file;
} t_arg;

/*
 * possible error states
 */
typedef enum
{
  ERR_OK,
  ERR_ARG_UNKNOWN,
  ERR_ARG_TOO_MUCH,
  ERR_ARG_NOT_ENOUGH,
  ERR_NUM_OUT_OF_RANGE,
  ERR_NUM_NOT_VALID,
  ERR_OUT_OF_MEM,
  ERR_FILE_WRITE,
  ERR_FILE_CLOSE,
  ERR_FORK,
  ERR_SHM_ATTACH,
  ERR_SHM_DETACH,
  ERR_SEM_INIT,
  ERR_MISC
} t_err;

/*
 * help message
 */
const char *hlpmsg;

/*
 * print error message
 * @param type of error
 * @param string containing some further specification,
 *        which will be printed as well (if any)
 * @return nothing
 */
void printErr(t_err err, const char *comment);

/*
 * parses the arguments
 * @param argument count
 * @return ERR_OK if OK; otherwise some error
 */
int parseArgs(t_arg *args, int argc, char *argv[]);

/*
 * parse string to int decimal num
 * @param string to parse
 * @param pointer to int where to store the result
 * @return 0 if OK, otherwise some error
 */
int parseNum(const char *str, int *num);

#endif
