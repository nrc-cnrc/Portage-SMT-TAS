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
 * Copyright 2006-2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006-2011, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <boost/math/special_functions/gamma.hpp>

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

typedef long double quad;

quad lnfact( double x )
{
   return lgamma( (quad) x + 1.0 );
}

double ln_hyper( double C_xy, double C_x, double C_y, double nn )
{
   quad result =
      - lnfact( C_xy )
      + lnfact( C_x )
      - lnfact( C_x - C_xy )
      + lnfact( C_y )
      - lnfact( C_y - C_xy )
      + lnfact( nn - C_x )
      - lnfact( nn - C_x - C_y + C_xy )
      + lnfact( nn - C_y )
      - lnfact( nn );
   return result;
}

double lnsum( double ln_x, double ln_y )
{
   if ( ln_x < ln_y ) swap( ln_x, ln_y );
   double del = ln_y - ln_x;
   double eps = exp( del );
   if ( del < 10.0 ) return ln_x + log( 1.0 + eps );
   double last_result = 0.0;
   double result = ln_x;
   double power = eps;
   double i = 1.0;
   while ( result != last_result ) {
      last_result = result;
      result = last_result + power / i;
      power *= -eps;
      ++i;
   }
   return result;
//   return ln_x + log( exp( ln_y - ln_x ) + 1.0 );
}

double ln_p_value( double C_xy, double C_x, double C_y, double nn )
{
   double ln_p = ln_hyper( C_xy, C_x, C_y, nn );
   double result = ln_p;
   double C_xY = C_x - C_xy;
   double C_Xy = C_y - C_xy;
   double C_XY = nn - C_x - C_y + C_xy;
   double C_xy_lim = ( ( C_x < C_y ) ? C_x : C_y );
   double last_result = 0.0;
   for ( ++C_xy;
         C_xy <= C_xy_lim && result != last_result;
         ++C_xy ) {
      last_result = result;
      ln_p +=   ( log( C_Xy-- ) + log( C_xY-- ) )
              - ( log( C_xy   ) + log( ++C_XY ) );
      result = lnsum( result, ln_p );
   }
   return result;
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
   printCopyright(2006, "sigprune_fet");
   getArgs( argc, argv );

   iSafeMagicStream istr( infile );
   oSafeMagicStream ostr( outfile );

   Uint lineno = 0;
   string line;
   string line_copy;

   ssize_t read;

   int i, j;
   vector< char * > field;

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
      double C_fr_en = strtod( field[ 1 ], 0 );
      double C_fr    = strtod( field[ 2 ], 0 );
      double C_en    = strtod( field[ 3 ], 0 );
      double nn      = strtod( field[ 4 ], 0 );
      if (    0.0 > C_fr_en
           || C_fr_en > C_fr
           || C_fr_en > C_en
           || C_fr > nn
           || C_en > nn ) {
         error(ETFatal, "Illegal contingency table\n%d : %d : %d : %d : %d", lineno, C_fr_en, C_fr, C_en, nn);
         cerr << "Illegal contingency table" << endl ;
         cerr << lineno
              << ": " << C_fr_en
              << ": " << C_fr
              << ": " << C_en
              << ": " << nn << endl;
         exit( 1 );
      }
      double minus_ln_nn = -log( nn );
      Uint flag;
      double p_value;

// Anti-linked
      if ( C_fr_en * nn < C_fr * C_en ) {
         flag = 0;
         p_value = -log( 2.0 );
      }

// 1-1-1 's
      else if ( C_fr_en == 1.0 && C_fr == 1.0 && C_en == 1.0 ) {
         flag = 2;
         p_value = minus_ln_nn;
      }

// Associated less strongly than 1-1-1's
      else {
         p_value = ln_p_value( C_fr_en, C_fr, C_en, nn );
         if ( p_value > minus_ln_nn ) {
            flag = 1;
         }

// Associated more strongly than 1-1-1's
         else {
            flag = 3;
         }
      }

      if ( flag >= flag_threshold ) {
         double s_p_value = floor( p_value * 1.0e8 + 0.5 );
         ostr << write_b64( s_p_value ) << '\t';
         ostr << setprecision( 14 );
         ostr << s_p_value * 1.0e-8 << '\t';
         ostr << flag << '\t';
         ostr << line_copy << '\n';
      }
   }
}
