//----------------------------------------------------------------------------
//                  Robokid
//----------------------------------------------------------------------------
// scripts.c : Set of inbuilt scripts
// =========
//
// Description
//
// Author                Date          Comment
//----------------------------------------------------------------------------
// Jim Herd           24/05/2011      created
//                       
//----------------------------------------------------------------------------

#pragma INTO_ROM
const char script_example[] =
"10 gosub 100\n\
20 for i = 1 to 11\n\
30 print i\n\
40 next i\n\
50 print \"end\"\n\
60 end\n\
100 print \"subroutine\"\n\
110 return\n";

#pragma INTO_ROM
const char script1[] =
"1 text \"Script1\"\n\
6 let h = 50\n\
7 speed h h\n\
8 leds 1 0 0 0\n\
9 motors 1 1\n\
10 wait 20\n\
11 leds 0 1 0 0\n\
15 motors 0 0\n\
20 wait 20\n\
25 leds 0 0 1 0\n\
30 motors 2 2\n\
35 wait 20\n\
40 motors 0 0\n\
45 leds 0 0 0 1\n\
50 text \"end\"\n\
90 end\n";

