// $Id$
/**
 * @author J Howard Johnson
 * @file sigprune_fet.cc
 * @brief Calculate Fisher's exact test significance levels
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <cmath>

#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
sigprune_fet [options] [INFILE [OUTFILE]]\n\
\n\
  Copy INFILE to OUTFILE (default stdin to stdout)\n\
  adding 3 fields to the beginning:\n\
     (1) sort key for log p level\n\
     (2) log p level\n\
     (3) flag\n\
         0 : anti-linked (p_level > 0.50)\n\
         1 : linked with log p level > alpha\n\
         2 : linked with log p level = alpha (1-1-1's)\n\
         3 : linked with log p level < alpha\n\
\n\
Options:\n\
\n\
  -v    Write progress reports to cerr.\n\
  -l    Minimum flag value to output (for pruning)\n\
  -n NL Copy only first NL lines [0 = all]\n\
";

// globals

static bool verbose = false;
static Uint num_lines = 0;
static Uint flag_threshold = 0;
static string infile( "-" );
static string outfile( "-" );

// arg processing

void getArgs( int argc, char* argv[] )
{
   const char* switches[] = { "v", "n:", "l:" };
   ArgReader arg_reader( ARRAY_SIZE( switches ), switches, 0, 2, help_message );
   arg_reader.read( argc - 1, argv + 1 );

   arg_reader.testAndSet( "v", verbose );
   arg_reader.testAndSet( "n", num_lines );
   arg_reader.testAndSet( "l", flag_threshold );

   arg_reader.testAndSet( 0, "infile", infile );
   arg_reader.testAndSet( 1, "outfile", outfile );
}

#define MINUS_INFINITY -1.0e10

double gammln2( double x )
{
   static double c[ 8 ] = {
      676.5203681218851,
    -1259.1392167224028,
      771.32342877765313,
     -176.61502916214059,
       12.507343278686905,
       -0.13857109526572012,
        0.0000099843695780195716,
        0.00000015056327351493116
   };
   double y = x;
   double t = x + 7.5;
   t -= ( x + 0.5 ) * log( t );
   double s = 0.99999999999980993;
   int j;
   for ( j = 0; j <= 7; j++ ) {
      s += c[ j ] / ++y;
   }
   return -t + log( 2.5066282746310005 * s / x );
}

double lnfact( double x )
{
   return gammln2( x + 1.0 );
}

double lnsum( double ln_x, double ln_y )
{
   if ( ln_y > ln_x ) {
      double t = ln_x;
      ln_x = ln_y;
      ln_y = t;
   }
   double del = ln_x - ln_y;
   if ( del > 35.0 ) return ln_x;
   double eps = exp( -del );
   if ( del < 12.0 ) return ln_x + log( 1.0 + eps );
   return ln_x + eps - eps * eps / 2.0;
}

double ln_hyper( long long C_x, long long C_y, long long C_xy, long long nn )
{
   return   lnfact( C_x )
          + lnfact( nn - C_x )
          + lnfact( C_y )
          + lnfact( nn - C_y )
          - lnfact( nn )
          - lnfact( C_xy )
          - lnfact( C_x - C_xy )
          - lnfact( C_y - C_xy )
          - lnfact( nn - C_x - C_y + C_xy );
}

double ln_p_value( long long C_x, long long C_y, long long C_xy, long long nn )
{
   double ln_p = ln_hyper( C_x, C_y, C_xy, nn );
   double result = ln_p;
   long long C_xY = C_x - C_xy;
   long long C_Xy = C_y - C_xy;
   long long C_XY = nn - C_x - C_y + C_xy;
   long long C_xy_lim = ( ( C_x < C_y ) ? C_x : C_y );
   double last_result = 0.0;
   for ( ++C_xy;
         C_xy <= C_xy_lim && last_result != result;
         ++C_xy ) {
      last_result = result;
      ln_p +=   ( log( C_Xy-- ) + log( C_xY-- ) )
              - ( log( C_xy   ) + log( ++C_XY ) );
      result = lnsum( result, ln_p );
   }
   return result;
}

double llr( long long C_x, long long C_y, long long C_xy, long long nn )
{
   long long C_X = nn - C_x;
   long long C_Y = nn - C_y;
   long long C_xY = C_x - C_xy;
   long long C_Xy = C_y - C_xy;
   long long C_XY = C_Y - C_xY;
   double ln_x  = ( C_x  > 0 ) ? log( C_x  ) : 0.0;
   double ln_X  = ( C_X  > 0 ) ? log( C_X  ) : 0.0;
   double ln_y  = ( C_y  > 0 ) ? log( C_y  ) : 0.0;
   double ln_Y  = ( C_Y  > 0 ) ? log( C_Y  ) : 0.0;
   double ln_xy = ( C_xy > 0 ) ? log( C_xy ) : 0.0;
   double ln_xY = ( C_xY > 0 ) ? log( C_xY ) : 0.0;
   double ln_Xy = ( C_Xy > 0 ) ? log( C_Xy ) : 0.0;
   double ln_XY = ( C_XY > 0 ) ? log( C_XY ) : 0.0;
   double ln_nn = (   nn > 0 ) ? log(   nn ) : 0.0;
   return   C_xy * ( - ln_x - ln_y + ln_xy + ln_nn )
          + C_xY * ( - ln_x - ln_Y + ln_xY + ln_nn )
          + C_Xy * ( - ln_X - ln_y + ln_Xy + ln_nn )
          + C_XY * ( - ln_X - ln_Y + ln_XY + ln_nn );
}

double G2( long long C_x, long long C_y, long long C_xy, long long nn )
{
   return 2.0 * llr( C_x, C_y, C_xy, nn );
}

string write_b64( double i )
{
   string result;
   int nd = 0;
   int digit;
   int d[ 20 ];
   if ( i > 0 ) {
      while ( nd < 20 && i > 0 ) {
         digit = fmod( i, 64.0 );
         d[ nd++ ] = digit;
         i = ( i - digit ) / 64.0;
      }
      result.push_back( 'P' + nd );
      while ( nd > 0 ) {
         result.push_back( '0' + d[ --nd ] );
      }
   } else {
      i = -i;
      while ( nd < 20 && i > 0 ) {
         digit = fmod( i, 64.0 );
         d[ nd++ ] = digit;
         i = ( i - digit ) / 64.0;
      }
      result.push_back( 'O' - nd );
      while ( nd > 0 ) {
         result.push_back( 'o' - d[ --nd ] );
      }
   }
   return result;
}



int main( int argc, char* argv[] )
{
   printCopyright(2005, "sigprune_fet");
   getArgs( argc, argv );

   iSafeMagicStream istr( infile );
   oSafeMagicStream ostr( outfile );

   Uint lineno = 0;
   string line;
   string line_copy;

   ssize_t read;

   int i, j;
   vector< char * > field;
   double minus_ln_2 = -log( 2.0 );
   long long last_nn = -1;
   double minus_ln_nn = 0.0;

   while (    getline( istr, line )
           && (    lineno++ < num_lines
                || num_lines == 0 ) ) {
      line_copy = line;
      read = line.length();
      field.resize( 0 );
      field.push_back( &line[ 0 ] );
      j = 0;
      for ( i = 0; i < read; ++i ) {
         if (    line[ i ] == '\t'
              || line[ i ] == '\n' ) {
            line[ i ] = '\0';
            field.push_back( &line[ i + 1 ] );
         }
      }
      long long C_fr_en = atoi( field[ 1 ] );
      long long C_fr    = atoi( field[ 2 ] );
      long long C_en    = atoi( field[ 3 ] );
      long long nn      = atoi( field[ 4 ] );
      if (    0 > C_fr_en
           || C_fr_en > C_fr
           || C_fr_en > C_en
           || C_fr > nn
           || C_en > nn ) {
         cout << "Illegal contingincy table" << endl ;
         exit( 1 );
      }
      if ( nn != last_nn ) {
         minus_ln_nn = -log( nn );
         last_nn = nn;
      }
      Uint flag;
      double p_value;

// Anti-linked
      if (   (double) C_fr_en * (double) nn
           < (double) C_fr * (double) C_en ) {
         flag = 0;
         p_value = minus_ln_2;
      }

// 1-1-1 's
      else if ( C_fr_en == 1 && C_fr == 1 && C_en == 1 ) {
         flag = 2;
         p_value = minus_ln_nn;
      }

// Associated less strongly than 1-1-1's
      else {
         p_value = ln_p_value( C_fr, C_en, C_fr_en, nn );
         if ( p_value > minus_ln_nn ) {
            flag = 1;
         }

// Associated more strongly than 1-1-1's
         else {
            flag = 3;
         }
      }

      if ( flag >= flag_threshold ) {
         ostr << write_b64( floor( p_value * 10000000.0 + 0.5 ) ) << '\t';
         ostr << p_value << '\t';
         ostr << flag << '\t';
         ostr << line_copy << '\n';
      }
   }
}
