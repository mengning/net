// Copyright (C) 2012, by the NoSQL project team.
// Hacked by Baojian Hua.

#ifndef DBTIME_H
#define DBTIME_H

// May also print to "outfile".
extern char *dbtime_filename;

// start timing
void dbtime_start (void);
// start a test with name "testname".
void dbtime_startTest (const char *testname);
// end timing, but does nothing else.
void dbtime_end (void);
// end timing, and meanwhile, write out time status
void dbtime_endAndShow (void);
// Show the different between the last time sequence.
// For now, only supports unnested timing. Do we need
// nested timing?
void dbtime_show (void);
// may also prints to file.
void dbtime_show2 (FILE*);

//
void dbtime_finalize (void);

#endif
