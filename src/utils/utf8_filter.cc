/**
 * @author J Howard Johnson
 * @utf8_filter.cc Verify / Sanitize UTF8 very strictly
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <file_utils.h>
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
utf8_filter [options] [INFILE [OUTFILE]]\n\
\n\
  Copy INFILE to OUTFILE (default stdin to stdout) sanitizing any\n\
  invalid UTF-8 characters according to a very strict interpretation.\n\
  Each invalid octet in the stream is replaced by [__xx] where 'xx'\n\
  is the hex value of the offending octet.  The sequence '[__' is\n\
  replaced by '[___'.\n\
\n\
  This can be also used as a test for valid UTF-8 since the return\n\
  code is non-zero if any erroneous octets are seen.\n\
\n\
Options:\n\
\n\
  -v    Write number of bad octets, number of valid Unicode characters,\n\
        number of lines to cerr.\n\
  -n NL Copy / verify only first NL lines [0 = all]\n\
  -c    Consider control characters to be errors.\n\
  -x    Escape < > \\ for canoe input.\n\
";

// globals

static bool verbose = false;
static bool nocontrol = false;
static bool canoeescape = false;
static Uint num_lines = 0;
static string infile( "-" );
static string outfile( "-" );
static void getArgs( int argc, char* argv[] );

// main

int main( int argc, char* argv[] )
{
   getArgs( argc, argv );

   iSafeMagicStream istr( infile );
   oSafeMagicStream ostr( outfile );

   int get_stack[ 5 ];
   int get_stack_n = 0;
   int hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

#define get_char(x)      ((get_stack_n>0)?get_stack[--get_stack_n]:(istr.get()))
#define put_back(x)      (get_stack[get_stack_n++]=(x))
#define write_1(a)       ostr.put(a)
#define write_2(a,b)     ostr.put(a);ostr.put(b)
#define write_3(a,b,c)   ostr.put(a);ostr.put(b);ostr.put(c)
#define write_4(a,b,c,d) ostr.put(a);ostr.put(b);ostr.put(c);ostr.put(d)
#define write_hex(x)     write_1(hex[(x&0xf0)>>4]);write_1(hex[x&0xf])
#define write_error(x)   write_3('[','_','_');write_hex(x);write_1(']');errors++

   int octet1;
   int octet2;
   int octet3;
   int octet4;

   Uint lineno = 0;
   Uint errors = 0;
   Uint codept = 0;

   octet1 = get_char();
   while ( octet1 != EOF ) {

      if ( octet1 == '[' ) {
         write_1( octet1 ); ++codept;
         octet2 = get_char();
         if ( octet2 == '_' ) {
            write_1( octet2 ); ++codept;
            octet3 = get_char();
            if ( octet3 == '_' ) {
               write_2( octet3, '_' ); ++codept;
            }
            else {
               put_back( octet3 );
            }
         }
         else {
            put_back( octet2 );
         }
      }

      else if ( octet1 <= 0x7f ) {
         if (    nocontrol
              && octet1 <  ' '
              && octet1 != '\r'
              && octet1 != '\n' ) {
            write_error( octet1 );
         }
         else {
            if (    canoeescape
                 && ( octet1 == '<'
                   || octet1 == '>'
                   || octet1 == '\\' ) ) {
               write_1( '\\' );
            }
            write_1( octet1 ); ++codept;
            if ( octet1 == '\n' ) {
               lineno++;
               if ( num_lines > 0 && lineno >= num_lines ) {
                  if ( verbose ) {
                     cerr << errors << " "
                          << codept << " "
                          << lineno << "\n";
                  }
                  exit( errors != 0 );
               }
            }
         }
      }

      else if ( octet1 >= 0xc2 && octet1 <= 0xdf ) {
         octet2 = get_char();
         if ( octet2 >= 0x80 && octet2 <= 0xbf ) {
            write_2( octet1, octet2 ); ++codept;
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( octet1 == 0xe0 ) {
         octet2 = get_char();
         if ( octet2 >= 0xa0 && octet2 <= 0xbf ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( (octet1 >= 0xe1 && octet1 <= 0xec) || octet1 == 0xee ) {
         octet2 = get_char();
         if ( octet2 >= 0x80 && octet2 <= 0xbf ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( octet1 == 0xed ) {
         octet2 = get_char();
         if ( octet2 >= 0x80 && octet2 <= 0x9f ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( octet1 == 0xef ) {
         octet2 = get_char();
         if ( octet2 >= 0x80 && octet2 <= 0xb6 ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else if ( octet2 == 0xb7 ) {
            octet3 = get_char();
            if (    (octet3 >= 0x80 && octet3 <= 0x8f)
                 || (octet3 >= 0xb0 && octet3 <= 0xbf) ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else if ( octet2 >= 0xb8 && octet2 <= 0xbe ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else if ( octet2 == 0xbf ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbd ) {
               write_3( octet1, octet2, octet3 ); ++codept;
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( octet1 == 0xf0 ) {
         octet2 = get_char();
         if ( octet2 >= 0x90 && octet2 <= 0xbe ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbf ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else if ( octet2 == 0x9f || octet2 == 0xaf || octet2 == 0xbf ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbe ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbf ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else if ( octet3 == 0xbf ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbd ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( octet1 >= 0xf1 && octet1 <= 0xf3 ) {
         octet2 = get_char();
         if ( octet2 >= 0x80 && octet2 <= 0xbe ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbf ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else if (    octet2 == 0x8f || octet2 == 0x9f
                   || octet2 == 0xaf || octet2 == 0xbf ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbe ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbf ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else if ( octet3 == 0xbf ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbd ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else if ( octet1 == 0xf4 ) {
         octet2 = get_char();
         if ( octet2 >= 0x80 && octet2 <= 0x8e ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbf ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbf ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else if ( octet2 == 0x8f ) {
            octet3 = get_char();
            if ( octet3 >= 0x80 && octet3 <= 0xbe ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbf ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else if ( octet3 == 0xbf ) {
               octet4 = get_char();
               if ( octet4 >= 0x80 && octet4 <= 0xbd ) {
                  write_4( octet1, octet2, octet3, octet4 ); ++codept;
               }
               else {
                  put_back( octet4 );
                  put_back( octet3 );
                  put_back( octet2 );
                  write_error( octet1 );
               }
            }
            else {
               put_back( octet3 );
               put_back( octet2 );
               write_error( octet1 );
            }
         }
         else {
            put_back( octet2 );
            write_error( octet1 );
         }
      }
  
      else {
         write_error( octet1 );
      }

      octet1 = get_char();
   }

   if ( verbose ) {
      cerr << errors << " " << codept << " " << lineno << "\n";
   }
   exit( errors != 0 );
}

// arg processing

void getArgs( int argc, char* argv[] )
{
   const char* switches[] = { "v", "n:", "c", "x" };
   ArgReader arg_reader( ARRAY_SIZE(switches), switches, 0, 2, help_message );
   arg_reader.read( argc - 1, argv + 1 );

   arg_reader.testAndSet( "v", verbose );
   arg_reader.testAndSet( "c", nocontrol );
   arg_reader.testAndSet( "x", canoeescape );
   arg_reader.testAndSet( "n", num_lines );

   arg_reader.testAndSet( 0, "infile", infile );
   arg_reader.testAndSet( 1, "outfile", outfile );
}






/*

Code Points:

 0: 00000000
 1: 00000001
 2: 0000001 x
 3: 000001 xx
 4: 00001 xxx
 5: 0001 xxxx
 6: 001 xxxxx
 7: 01 xxxxxx
 8: 1100001 x / 10 xxxxxx
 9: 110001 xx / 10 xxxxxx
10: 11001 xxx / 10 xxxxxx
11: 1101 xxxx / 10 xxxxxx
12: 11100000  / 101 xxxxx / 10 xxxxxx
13: 11100001  / 10 xxxxxx / 10 xxxxxx
14: 1110001 x / 10 xxxxxx / 10 xxxxxx
15: 111001 xx / 10 xxxxxx / 10 xxxxxx
16: 11101 xxx / 10 xxxxxx / 10 xxxxxx
17: 11110000  / 1001 xxxx / 10 xxxxxx / 10 xxxxxx
18: 11110000  / 101 xxxxx / 10 xxxxxx / 10 xxxxxx
19: 11110001  / 10 xxxxxx / 10 xxxxxx / 10 xxxxxx
20: 1111001 x / 10 xxxxxx / 10 xxxxxx / 10 xxxxxx
21: 11110100  / 1000 xxxx / 10 xxxxxx / 10 xxxxxx

21: must be of form 1 0000 xxxx xxxx xxxx xxxx (plane 16)

0--7:   [\x00-\x7f]
8--11:  [\xc2-\xdf] [\x80-\xbf]
12:     [\xe0]      [\xa0-\xbf] [\x80-\xbf]
13--16: [\xe1-\xef] [\x80-\xbf] [\x80-\xbf]
17--18: [\xf0]      [\x90-\xbf] [\x80-\xbf] [\x80-\xbf]
19--20: [\xf1-\xf3] [\x80-\xbf] [\x80-\xbf] [\x80-\xbf]
21:     [\xf4]      [\x80-\x8f] [\x80-\xbf] [\x80-\xbf]

( [\x00-\x7f]
| ( [\xc2-\xdf]
  | [\xe0][\xa0-\xbf]
  | ( [\xe1-\xef]
    | [\xf0][\x90-\xbf]
    | [\xf1-\xf3][\x80-\xbf]
    | [\xf4][\x80-\x8f]
    ) [\x80-\xbf]
  ) [\x80-\xbf]
)

#=======================================================================
Code Points minus Surrogates:

Surrogates are in range: U+d800--U+dfff
    UCS2: 1101 1xxx xxxx xxxx
    UTF8: 11101101 / 101 xxxxx / 10 xxxxxx

16: 111010 xx / 10 xxxxxx / 10 xxxxxx
16: 11101100  / 10 xxxxxx / 10 xxxxxx
16: 11101101  / 100 xxxxx / 10 xxxxxx
16: 1110111 x / 10 xxxxxx / 10 xxxxxx

0--7:   [\x00-\x7f]
8--11:  [\xc2-\xdf] [\x80-\xbf]
12:     [\xe0]      [\xa0-\xbf] [\x80-\xbf]
13--15: [\xe1-\xe7] [\x80-\xbf] [\x80-\xbf]
16:     [\xe8-\xeb] [\x80-\xbf] [\x80-\xbf]
      | [\xec]      [\x80-\xbf] [\x80-\xbf]
      | [\xed]      [\x80-\x9f] [\x80-\xbf]
      | [\xee-\xef] [\x80-\xbf] [\x80-\xbf]
17--18: [\xf0]      [\x90-\xbf] [\x80-\xbf] [\x80-\xbf]
19--20: [\xf1-\xf3] [\x80-\xbf] [\x80-\xbf] [\x80-\xbf]
21:     [\xf4]      [\x80-\x8f] [\x80-\xbf] [\x80-\xbf]

( [\x00-\x7f]
| ( [\xc2-\xdf]
  | [\xe0][\xa0-\xbf]
  | [\xed][\x80-\x9f]
  | ( [\xe1-\xec\xee-\xef]
    | [\xf0][\x90-\xbf]
    | [\xf1-\xf3][\x80-\xbf]
    | [\xf4][\x80-\x8f]
    ) [\x80-\xbf]
  ) [\x80-\xbf]
)

#=======================================================================
Code Points minus Surrogates minus BMP Noncharacters:

Noncharacters in range: U+fdd0--U+fdef U+fffe U+ffff
    UCS2: 1111 1101 1101 xxxx
    UCS2: 1111 1101 1110 xxxx
    UCS2: 1111 1111 1111 111x
    UTF8: 11101111 / 10110111 / 1001 xxxx
    UTF8: 11101111 / 10110111 / 1010 xxxx
    UTF8: 11101111 / 10111111 / 1011111 x

16: 111010 xx / 10 xxxxxx / 10 xxxxxx
16: 11101100  / 10 xxxxxx / 10 xxxxxx

16: 11101101  / 100 xxxxx / 10 xxxxxx

16: 11101110  / 10 xxxxxx / 10 xxxxxx

16: 11101111  / 100 xxxxx / 10 xxxxxx
16: 11101111  / 1010 xxxx / 10 xxxxxx
16: 11101111  / 101100 xx / 10 xxxxxx
16: 11101111  / 1011010 x / 10 xxxxxx
16: 11101111  / 10110110  / 10 xxxxxx

16: 11101111  / 10110111  / 1000 xxxx

16: 11101111  / 10110111  / 1011 xxxx

16: 11101111  / 101110 xx / 10 xxxxxx
16: 11101111  / 1011110 x / 10 xxxxxx
16: 11101111  / 10111110  / 10 xxxxxx

16: 11101111  / 10111111  / 100 xxxxx
16: 11101111  / 10111111  / 1010 xxxx
16: 11101111  / 10111111  / 10110 xxx
16: 11101111  / 10111111  / 101110 xx
16: 11101111  / 10111111  / 1011110 x

0--7:   [\x00-\x7f]
8--11:  [\xc2-\xdf] [\x80-\xbf]
12:     [\xe0]      [\xa0-\xbf] [\x80-\xbf]
13--15: [\xe1-\xe7] [\x80-\xbf] [\x80-\xbf]
16:     [\xe8-\xec] [\x80-\xbf] [\x80-\xbf]
      | [\xed]      [\x80-\x9f] [\x80-\xbf]
      | [\xee]      [\x80-\xbf] [\x80-\xbf]
      | [\xef]      [\x80-\xb6] [\x80-\xbf]
      | [\xef]      [\xb7]      [\x80-\x8f]
      | [\xef]      [\xb7]      [\xb0-\xbf]
      | [\xef]      [\xb8-\xbe] [\x80-\xbf]
      | [\xef]      [\xbf]      [\x80-\xbd]
17--18: [\xf0]      [\x90-\xbf] [\x80-\xbf] [\x80-\xbf]
19--20: [\xf1-\xf3] [\x80-\xbf] [\x80-\xbf] [\x80-\xbf]
21:     [\xf4]      [\x80-\x8f] [\x80-\xbf] [\x80-\xbf]

( [\x00-\x7f]
| ( [\xc2-\xdf]
  | [\xe0][\xa0-\xbf]
  | [\xed][\x80-\x9f]
  | ( [\xe1-\xec\xee]
    | [\xf0][\x90-\xbf]
    | [\xf1-\xf3][\x80-\xbf]
    | [\xf4][\x80-\x8f]
    ) [\x80-\xbf]
  ) [\x80-\xbf]
| [\xef]
  ( [\x80-\xb6][\x80-\xbf]
  | [\xb7][\x80-\x8f\xb0-\xbf]
  | [\xb8-\xbe][\x80-\xbf]
  | [\xbf][\x80-\xbd]
  )
)

#=======================================================================
Code Points minus Surrogates minus Noncharacters:

Noncharacters: U+1fffe U+1ffff
    UCS4: 0000 0000 0000 0001 1111 1111 1111 111x
    UTF8: 11110000 / 10011111 / 10111111 / 1011111 x

Noncharacters: U+2fffe U+2ffff U+3fffe U+3ffff
    UCS4: 0000 0000 0000 001x 1111 1111 1111 111x
    UTF8: 11110000 / 101 x 1111 / 10111111 / 1011111 x

17,18: 11100000 / 10 yy 0 xxx / 10 xxxxxx / 10 xxxxxx
17,18: 11100000 / 10 yy 10 xx / 10 xxxxxx / 10 xxxxxx
17,18: 11100000 / 10 yy 110 x / 10 xxxxxx / 10 xxxxxx
17,18: 11100000 / 10 yy 1110  / 10 xxxxxx / 10 xxxxxx

17,18: 11100000 / 10 yy 1111  / 100 xxxxx / 10 xxxxxx
17,18: 11100000 / 10 yy 1111  / 1010 xxxx / 10 xxxxxx
17,18: 11100000 / 10 yy 1111  / 10110 xxx / 10 xxxxxx
17,18: 11100000 / 10 yy 1111  / 101110 xx / 10 xxxxxx
17,18: 11100000 / 10 yy 1111  / 1011110 x / 10 xxxxxx
17,18: 11100000 / 10 yy 1111  / 10111110  / 10 xxxxxx

17,18: 11100000 / 10 yy 1111  / 10111111  / 100 xxxxx
17,18: 11100000 / 10 yy 1111  / 10111111  / 1010 xxxx
17,18: 11100000 / 10 yy 1111  / 10111111  / 10110 xxx
17,18: 11100000 / 10 yy 1111  / 10111111  / 101110 xx
17,18: 11100000 / 10 yy 1111  / 10111111  / 1011110 x
where yy in { 01, 10, 11 }

Noncharacters: U+4fffe U+4ffff U+5fffe U+5ffff U+6fffe U+6ffff U+7fffe
               U+7ffff
    UCS4: 0000 0000 0000 01xx 1111 1111 1111 111x
    UTF8: 11110001 / 10 xx 1111 / 10111111 / 1011111 x

Noncharacters: U+8fffe U+8ffff U+afffe U+affff U+bfffe U+bffff U+cfffe
               U+cffff U+dfffe U+dffff U+efffe U+effff U+ffffe U+fffff
    UCS4: 0000 0000 0000 1xxx 1111 1111 1111 111x
    UTF8: 1111001 x / 10 xx 1111 / 10111111 / 1011111 x

19,20: 111000 yy / 10 xx 0 xxx / 10 xxxxxx / 10 xxxxxx
19,20: 111000 yy / 10 xx 10 xx / 10 xxxxxx / 10 xxxxxx
19,20: 111000 yy / 10 xx 110 x / 10 xxxxxx / 10 xxxxxx
19,20: 111000 yy / 10 xx 1110  / 10 xxxxxx / 10 xxxxxx

19,20: 111000 yy / 10 xx 1111  / 100 xxxxx / 10 xxxxxx
19,20: 111000 yy / 10 xx 1111  / 1010 xxxx / 10 xxxxxx
19,20: 111000 yy / 10 xx 1111  / 10110 xxx / 10 xxxxxx
19,20: 111000 yy / 10 xx 1111  / 101110 xx / 10 xxxxxx
19,20: 111000 yy / 10 xx 1111  / 1011110 x / 10 xxxxxx
19,20: 111000 yy / 10 xx 1111  / 10111110  / 10 xxxxxx

19,20: 111000 yy / 10 xx 1111  / 10111111  / 100 xxxxx
19,20: 111000 yy / 10 xx 1111  / 10111111  / 1010 xxxx
19,20: 111000 yy / 10 xx 1111  / 10111111  / 10110 xxx
19,20: 111000 yy / 10 xx 1111  / 10111111  / 101110 xx
19,20: 111000 yy / 10 xx 1111  / 10111111  / 1011110 x
where yy in { 01, 10, 11 }

Noncharacters: U+10fffe U+10ffff
    UCS4: 0000 0000 0001 0000 1111 1111 1111 111x
    UTF8: 11110100 / 10001111 / 10111111 / 1011111 x

21: 11100100 / 10000 xxx / 10 xxxxxx / 10 xxxxxx
21: 11100100 / 100010 xx / 10 xxxxxx / 10 xxxxxx
21: 11100100 / 1000110 x / 10 xxxxxx / 10 xxxxxx
21: 11100100 / 10001110  / 10 xxxxxx / 10 xxxxxx

21: 11100100 / 10001111  / 100 xxxxx / 10 xxxxxx
21: 11100100 / 10001111  / 1010 xxxx / 10 xxxxxx
21: 11100100 / 10001111  / 10110 xxx / 10 xxxxxx
21: 11100100 / 10001111  / 101110 xx / 10 xxxxxx
21: 11100100 / 10001111  / 1011110 x / 10 xxxxxx
21: 11100100 / 10001111  / 10111110  / 10 xxxxxx

21: 11100100 / 10001111  / 10111111  / 100 xxxxx
21: 11100100 / 10001111  / 10111111  / 1010 xxxx
21: 11100100 / 10001111  / 10111111  / 10110 xxx
21: 11100100 / 10001111  / 10111111  / 101110 xx
21: 11100100 / 10001111  / 10111111  / 1011110 x

0--7:   [\x00-\x7f]
8--11:  [\xc2-\xdf] [\x80-\xbf]
12:     [\xe0]      [\xa0-\xbf] [\x80-\xbf]
13--15: [\xe1-\xe7] [\x80-\xbf] [\x80-\xbf]
16:     [\xe8-\xec] [\x80-\xbf] [\x80-\xbf]
      | [\xed]      [\x80-\x9f] [\x80-\xbf]
      | [\xee]      [\x80-\xbf] [\x80-\xbf]
      | [\xef]      [\x80-\xb6] [\x80-\xbf]
      | [\xef]      [\xb7]      [\x80-\x8f]
      | [\xef]      [\xb7]      [\xb0-\xbf]
      | [\xef]      [\xb8-\xbe] [\x80-\xbf]
      | [\xef]      [\xbf]      [\x80-\xbd]
17--18: [\xf0]      [\x90-\xbe]    [\x80-\xbf] [\x80-\xbf]
      | [\xf0]      [\x9f\xaf\xbf] [\x80-\xbe] [\x80-\xbf]
      | [\xf0]      [\x9f\xaf\xbf] [\xbf]      [\x80-\xbd]
19--20: [\xf1-\xf3] [\x80-\xbe]        [\x80-\xbf] [\x80-\xbf]
      | [\xf1-\xf3] [\x8f\x9f\xaf\xbf] [\x80-\xbe] [\x80-\xbf]
      | [\xf1-\xf3] [\x8f\x9f\xaf\xbf] [\xbf]      [\x80-\xbd]
21:     [\xf4]      [\x80-\x8e]        [\x80-\xbf] [\x80-\xbf]
      | [\xf4]      [\x8f]             [\x80-\xbe] [\x80-\xbf]
      | [\xf4]      [\x8f]             [\xbf]      [\x80-\xbd]

( [\x00-\x7f]
| ( [\xc2-\xdf]
  | [\xe0][\xa0-\xbf]
  | [\xe1-\xec\xee][\x80-\xbf]
  | [\xed][\x80-\x9f]
  ) [\x80-\xbf]
| [\xef]
  ( [\x80-\xb6][\x80-\xbf]
  | [\xb7][\x80-\x8f\xb0-\xbf]
  | [\xb8-\xbe][\x80-\xbf]
  | [\xbf][\x80-\xbd]
  )
| [\xf0]
  ( [\x90-\xbe][\x80-\xbf][\x80-\xbf]
  | [\x9f\xaf\xbf]
    ( [\x80-\xbe][\x80-\xbf]
    | [\xbf][\x80-\xbd]
    )
  )
| [\xf1-\xf3]
  ( [\x80-\xbe][\x80-\xbf][\x80-\xbf]
  | [\x8f\x9f\xaf\xbf]
    ( [\x80-\xbe][\x80-\xbf]
    | [\xbf][\x80-\xbd]
    )
  )
| [\xf4]
  ( [\x80-\x8e][\x80-\xbf][\x80-\xbf]
  | [\x8f]
    ( [\x80-\xbe][\x80-\xbf]
    | [\xbf][\x80-\xbd]
    )
  )
)

#=======================================================================
*/
