/**
 * @author Eric Joanis
 * @file tppt_config.h  Read and write TPPT configuration files
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#ifndef TPPT_CONFIG_H
#define TPPT_CONFIG_H

#include "tpt_typedefs.h"
#include "str_utils.h"

namespace ugdiss {

namespace TPPTConfig {

   static const string MagicNumber_v2 = "TPPT v2, format augmented by Eric Joanis (NRC) from Uli Germann's original TPPT format.";

   const char* code_book_magic_number_v2 = "TPPT Codebook format v2 ";

   void write(const string& filename,
      uint32_t third_col_count, uint32_t fourth_col_count,
      uint32_t num_counts, bool has_alignments)
   {
      ofstream configFile(filename.c_str());
      if (configFile.fail())
         cerr << efatal << "Unable to open config file '" << filename << "' for writing."
              << exit_1;
      configFile
         << MagicNumber_v2 << endl
         << "ThirdColumnCount=" << third_col_count << endl
         << "FourthColumnCount=" << fourth_col_count << endl
         << "NumCounts=" << num_counts << endl
         << "HasAlignment=" << has_alignments << endl
         ;
      configFile.close();
   }

   /**
    * Read a TPPT config file.
    * @return the TPPT version number: 1 is original (no config file found),
    *         2 is v2, with support for 4th column, counts and alignments.
    */
   uint32_t read(const string& filename,
      uint32_t &third_col_count, uint32_t &fourth_col_count,
      uint32_t &num_counts, bool &has_alignments)
   {
      third_col_count = fourth_col_count = num_counts = 0;
      has_alignments = false;

      ifstream configFile(filename.c_str());
      if (configFile.fail()) {
         return 1;
      }

      string line;
      if (getline(configFile, line) && line == MagicNumber_v2) {
         // We're reading a TPPT v2 config file
         vector<string> tokens;
         while (getline(configFile, line)) {
            splitZ(line, tokens, "=");
            if (tokens.size() != 2)
               cerr << efatal << "Invalid line in TPPT config file " << filename
                    << ": " << line << exit_1;
            if (tokens[0] == "ThirdColumnCount")
               third_col_count = conv<uint32_t>(tokens[1]);
            else if (tokens[0] == "FourthColumnCount")
               fourth_col_count = conv<uint32_t>(tokens[1]);
            else if (tokens[0] == "NumCounts")
               num_counts = conv<uint32_t>(tokens[1]);
            else if (tokens[0] == "HasAlignment")
               has_alignments = conv<bool>(tokens[1]);
            else
               cerr << efatal << "Unknown identifier in config file " << filename
                    << ": " << tokens[0] << exit_1;
         }
         return 2;
      }

      cerr << efatal << "Invalid TPPT config file " << filename << exit_1;
      return 0;
   }

} // namespace TPPTConfig
} // namespace ugdiss


#endif // TPPT_CONFIG_H
