// Copyright (C) 2012, by the NoSQL project team.
// Hacked by Baojian Hua.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dbtime.h"

#define NANO (1000000000)
#define MS (1000000)

char *dbtime_filename = 0;
static FILE *dbtime_outfile = 0;

static char *defaultName = "dbtime.time";

// Initialize along the way to catch bugs.
static int flag = 0;;
static struct timespec start = {0, 0L};
static struct timespec end = {0, 0L};

// Linux kernel-style.
void 
dbtime_start ()
{
  if (flag){
    fprintf (stderr, "error\n");
    exit (0);
  }
  // outfile has not been set, then
  // set a default.
  if (0==dbtime_filename){
    dbtime_filename = defaultName;
    dbtime_outfile = stdout;//fopen (defaultName, "w+");
  }
  else if (!dbtime_outfile){
    char a[100];
    
    strcpy (a, dbtime_filename);
    strcat (a, ".time");
    dbtime_outfile = fopen (a, "w+");
    if (!dbtime_outfile){
      fprintf (stderr, "error open file\n");
      exit (1);
    }
  }
  flag = 1;
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &start);
  return;
}

void 
dbtime_startTest (const char *testname)
{
  if (flag){
    printf ("error\n");
    exit (0);
  }
  // outfile has not been set, then
  // set a default.
  if (0==dbtime_filename){
    dbtime_filename = defaultName;
    dbtime_outfile = fopen (defaultName, "w+");
  }
  else if (!dbtime_outfile){
    char a[100];
    
    strcpy (a, dbtime_filename);
    strcat (a, ".time");
    dbtime_outfile = fopen (a, "w+");
    if (!dbtime_outfile){
      fprintf (stderr, "error open file\n");
      exit (1);
    }
  }
  flag = 1;
  fprintf (dbtime_outfile, "\n%s:\n", testname);
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &start);
  return;
}

void 
dbtime_end ()
{
  if (!flag){
    fprintf (stderr, "error\n");
    exit (0);
  }
  flag = 0;
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &end);
  return;
}

void 
dbtime_endAndShow ()
{
  if (!flag){
    fprintf (stderr, "error\n");
    exit (0);
  }
  flag = 0;
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &end);
  dbtime_show ();
  return;
}

void 
dbtime_show ()
{
  struct timespec ltime;
  double us, ms, s;

  if (flag){
    fprintf (stderr, "error\n");
    exit (0);
  }
  if (end.tv_nsec <= start.tv_nsec){
    ltime.tv_nsec = NANO + end.tv_nsec - start.tv_nsec;
    ltime.tv_sec = end.tv_sec - start.tv_sec - 1;
  }
  else {
    ltime.tv_nsec = end.tv_nsec - start.tv_nsec;
    ltime.tv_sec = end.tv_sec - start.tv_sec;
  }
  // pretty-printing it
  us = ltime.tv_nsec * 1.0 / 1000;
  ms = ltime.tv_nsec * 1.0 / 1000000;
  s = ltime.tv_nsec * 1.0 / NANO;
  
  // to shut up the compiler
  us = us;
  ms = ms;
  
  // For the purpose of time bookkeeping, I modify this
  // to use a much cleaner representation.
  /*
  fprintf (dbtime_outfile
           , "%ds + (%luns, %lfus, %lfms, %lfs)\n"
           , (int)ltime.tv_sec
           , ltime.tv_nsec
           , us
           , ms
           , s);
  */
  fprintf (dbtime_outfile
	   , "%lf\n"
	   , (int)ltime.tv_sec + s);
  return;
}

void 
dbtime_finalize ()
{
  if (dbtime_outfile)
    fclose (dbtime_outfile);
  return;
}
