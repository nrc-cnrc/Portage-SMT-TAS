// This file is derivative work from Ulrich Germann's Tightly Packed Tries
// package (TPTs and related software).
//
// Original Copyright:
// Copyright 2005-2009 Ulrich Germann; all rights reserved.
// Under licence to NRC.
//
// Copyright for modifications:
// Technologies langagieres interactives / Interactive Language Technologies
// Inst. de technologie de l'information / Institute for Information Technology
// Conseil national de recherches Canada / National Research Council Canada
// Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
// Copyright 2008-2010, Her Majesty in Right of Canada


// (c) 2007,2008 Ulrich Germann
/** 
 * @author Ulrich Germann
 * @file ptable.encode-scores.cc
 * @brief Preparatory step in converting phrase tables to TPPT:
 * encodes phrase scores.
 *
 * Second step in textpt2tppt conversion. Normally called via textpt2tppt.sh.
 */

// YET TO BE DONE: 
// - allow truncation of scores to get smaller, coarser code books
// - proper help message, command line interface, etc

#include <map>
#include <tr1/unordered_map>
#include <ctime>
#include <cmath>
#include <sstream>
#include <limits>  // numeric_limits<float>::min & numeric_limits<float>::max

#if IN_PORTAGE
#include "file_utils.h"
#include "tm_entry.h"
using namespace Portage;
#else
#include <boost/iostreams/stream.hpp>
#include "ugStream.h"
#endif

#include <boost/program_options.hpp>

//#define DEBUG_TPT

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "tpt_encodingschemes.h"
#include "tppt_config.h"

static char base_help_message[] = "\n\
ptable.encode-scores [options] [-i] TEXTPT_FILE [-o] OUTPUT_BASE_NAME\n\
\n\
  Encode the phrase scores for a text phrase table file TEXTPT_FILE.\n\
\n\
  The following files are produced:\n\
  OUTPUT_BASE_NAME.scr is an intermediate file containing a score id for each\n\
  score in the phrase table.\n\
  OUTPUT_BASE_NAME.cbk is the codebook file for decoding the scores.\n\
\n\
  This is the second step in the conversion of text phrase tables to tightly\n\
  packed phrase tables (TPPT).\n\
  This program is normally called via textpt2tppt.sh.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
   return os << base_help_message << options_help.str();
}

namespace po  = boost::program_options;
namespace bio = boost::iostreams;

using namespace std;
using namespace ugdiss;

const char * const COLSEP = " ||| ";

string iFileName;
string oBaseName;
//string truncation;
// vector<size_t> truncactionBits; // add truncation later (maybe)

// If the phrase table has new fields (4th column, count and/or alignment), we
// need the use the TPPT v2 format.
bool using_v2 = false;

void
interpret_args(int ac, char* av[])
{
   po::variables_map vm;
   po::options_description o("Options");
   o.add_options()
      ("help,h",  "print this message")
      ("quiet,q", "don't print progress information")
      ("input,i",  po::value<string>(&iFileName), "input file")
      ("output,o", po::value<string>(&oBaseName), "base name for output files")
//      ("truncate,x", po::value<string>(&truncation),
//       "how many bits to mask out for truncation (max. 23)")
      ;
   options_help << o;
   po::positional_options_description a;
   a.add("input",1);
   a.add("output",1);

   try {
      po::store(po::command_line_parser(ac,av).options(o).positional(a).run(), vm);
      po::notify(vm);
   } catch(std::exception& e) {
      cerr << efatal << e.what() << endl << help_message << exit_1;
   }


   if (vm.count("help")) {
      cerr << help_message << endl;
      exit(0);
   }

   if (!iFileName.size())
      cerr << efatal << "TEXTPT_FILE required." << endl << help_message << exit_1;

   if (!oBaseName.size())
      cerr << efatal << "OUTPUT_BASE_NAME required." << endl << help_message
           << exit_1;
}

void
count_scores_per_line(TMEntry& entry, uint32_t& third_col_count, uint32_t& adir_scores, bool& has_alignment, uint32_t& num_counts)
{
   third_col_count = entry.ThirdCount();
   adir_scores = entry.FourthCount();
   double v[third_col_count];
   const char *a;
   char *c;
   entry.parseThird(v, &a, &c);
   if (third_col_count > 0 && v[third_col_count-1] > 2.7179 && v[third_col_count-1] < 2.7181)
      --third_col_count; // moses-style phrase table
   has_alignment = (a != NULL);
   vector<uint32_t> counts;
   num_counts = c ? split(c, counts, ",") : 0;
}

// Convert a double to a number than can be represented by a float without
// becoming infinity or zero.  Information can be lost!
float trim_double_to_float(double x)
{
   if (x > 0) {
      if (x < numeric_limits<float>::min()) return numeric_limits<float>::min();
      if (x > numeric_limits<float>::max()) return numeric_limits<float>::max();
   }
   else if (x < 0) {
      if (-x < numeric_limits<float>::min()) return -numeric_limits<float>::min();
      if (-x > numeric_limits<float>::max()) return -numeric_limits<float>::max();
   }
   return static_cast<float>(x);
}

void
process_line(TMEntry& entry, ostream& tmp,
             uint32_t third_col_scores, uint32_t num_counts, bool has_alignments)
{
   assert(third_col_scores <= entry.ThirdCount());
   double v[entry.ThirdCount()];
   const char *al;
   char *c;
   entry.parseThird(v, &al, &c);
   for (uint32_t i = 0; i < third_col_scores; ++i) {
      float s = trim_double_to_float(v[i]);
      tmp.write(reinterpret_cast<char*>(&s),sizeof(s));
   }
   double a[entry.FourthCount()];
   entry.parseFourth(a);
   for (uint32_t i = 0; i < entry.FourthCount(); ++i) {
      float s = trim_double_to_float(a[i]);
      tmp.write(reinterpret_cast<char*>(&s),sizeof(s));
   }
   if (num_counts) {
      vector<uint32_t> counts;
      if (c) split(c, counts, ",");
      if (counts.size() > num_counts)
         cerr << efatal << "Too many counts on line " << entry.LineNo()
              << " in file " << entry.File() << exit_1;
      if (counts.size() < num_counts)
         counts.resize(num_counts, 0);
      for (uint32_t i = 0; i < num_counts; ++i) {
         uint32_t count = (i < counts.size() ? counts[i] : 0);
         tmp.write(reinterpret_cast<char*>(&count),sizeof(count));
      }
   }
   if (has_alignments) {
      // TODO: handle the alignment string here.
      assert(false);
   }
}

//typedef tr1::unordered_map<float, uint32_t> scores_map_t;
//typedef map<float, uint32_t> scores_map_t; // use to reproduce Uli's original output
//vector<uint32_t> ids;  // use to reproduce Uli's original sort order.

// Count score occurrences, writing preliminary score IDs to the .scr file.
// Fill the scrs and cnts vectors, ordered by id, with scores and their
// corresponding counts, respectively.
template <class ScoreT>
void
count_scores(bio::mapped_file &scr_file, uint32_t which_scr, uint32_t how_many_scr,
             size_t fields_per_line,
             vector<ScoreT>& scrs, vector<uint32_t>& cnts)
{
   assert(sizeof(ScoreT) == 4);
   assert(which_scr + how_many_scr <= fields_per_line);

   // No real need for custom allocation when we're using TCMalloc: with
   // TCMalloc, there's no memory overhead, and it's only about 25% slower
   // (whereas with older mallocs, there is a large memory overhead and a much
   // more significant time cost).  Plus, I (EJJ) could not make the
   // scores_allocator work correctly with the key type being a template
   // parameter.
   typedef tr1::unordered_map<ScoreT, uint32_t> scores_map_t;

   ScoreT* scr_end = reinterpret_cast<ScoreT*>(scr_file.data() + scr_file.size());
   ScoreT* p = reinterpret_cast<ScoreT*>(scr_file.data());
   size_t num_lines = (scr_end - p)/fields_per_line;
   // Benchmarks show this is a good heuristic reserve() size, and it helps performance.
   cnts.reserve((num_lines + 7) / 8);
   scrs.reserve((num_lines + 7) / 8);

   scores_map_t scores;  // for map or default unordered_map scores_map_t

   p += which_scr;
   while (p < scr_end)
   {
      for (uint32_t i = 0; i < how_many_scr; ++i) {
         typename scores_map_t::value_type new_score(*(p+i), scores.size());
         TPT_DBG(cerr << "inserting: " << new_score.first << " " << new_score.second << endl);
         typename scores_map_t::iterator score_it = scores.insert(new_score).first;
         assert(score_it->second <= cnts.size());
         if (score_it->second == cnts.size()) {
            cnts.push_back(0);
            scrs.push_back(new_score.first);
         }
         ++cnts[score_it->second];
         // Convert the score in the file to its preliminary score id.
         *reinterpret_cast<uint32_t*>(p+i) = score_it->second;
      }
      p += fields_per_line;
   }
   TPT_DBG(cerr << "cnts: size = " << cnts.size() << ", capacity = " << cnts.capacity() << endl);
   //  sleep(5);           // maximum space used at this point

   // Code for using map<ScoreT, uint32_t> to reproduce Uli's original output:
   // Use global ids vector to reproduce Uli's original sort order.
   //scrs.resize(cnts.size());
   //ids.resize(cnts.size());
   //uint i = 0;
   //for (typename scores_map_t::const_iterator m = scores->begin(); m != scores->end(); m++)
   //{
   //   scrs[m->second] = m->first;
   //   ids[i++] = m->second;
   //}
   //delete scores;
}

struct byFreq
{
   vector<uint32_t>& cnts;
   byFreq(vector<uint32_t>& cnts) : cnts(cnts) {}
   bool operator()(uint32_t const& A, uint32_t const& B)
   {
      return cnts[A] > cnts[B];
   }
};

template <class ScoreT> const char* type2string() { assert(false); return ""; }
template <> const char* type2string<float>() { return "float   "; }
template <> const char* type2string<uint32_t>() { return "uint32_t"; }

// Write the codebook for a scores column to the .cbk file and convert cnts to
// a remapping vector mapping the preliminary score IDs to final score IDs.
template <class ScoreT>
void
mkCodeBook(ostream& codebook, vector<ScoreT>& scrs, vector<uint32_t>& cnts)
{
   assert(sizeof(ScoreT) == 4);
   TPT_DBG(cerr << "mkCodeBook: scrs.size = " << scrs.size() << endl);
   vector<uint32_t>& remap = cnts;  // cnts doubles as the remap vector.

   // Note: comment out the next 3 lines when reproducing Uli's original output.
   vector<uint32_t> ids(scrs.size());
   for (size_t i = 0; i < scrs.size(); ++i)
      ids[i] = i;
   sort(ids.begin(), ids.end(), byFreq(cnts));

   vector<uint32_t> bits_needed(max(2, int(ceil(log2(ids.size()))))+1, 0);
   for (size_t k = 0; k < scrs.size(); k++)
   {
      size_t bn = max(2, int(ceil(log2(k+1))));
      bits_needed[bn] += cnts[ids[k]];
   }
   for (size_t k = 0; k < remap.size(); k++)
      remap[ids[k]] = k;       // remap the ids.
   TPT_DBG(cerr << "  bits_needed.size = " << bits_needed.size() << endl);

   // determine the best encoding scheme
   Escheme best = best_scheme(bits_needed,5);
   vector<uint32_t> const& blocks = best.blockSizes;

   TPT_DBG(cerr << "  scrs.size = " << scrs.size() << endl);
   uint32_t x = scrs.size(); // number of distinct scores in this column
   if (using_v2) codebook.write(type2string<ScoreT>(), 8);
   codebook.write(reinterpret_cast<char*>(&x), 4);
   TPT_DBG(cerr << "  blocks.size = " << blocks.size() << endl);
   x = blocks.size(); // number of bit blocks used to encode score IDs
   codebook.write(reinterpret_cast<char*>(&x), 4);
   for (size_t k = 0; k < blocks.size(); k++)
   {
      TPT_DBG(cerr << "  blocks[" << k << "] = " << blocks[k] << endl);
      codebook.write(reinterpret_cast<char const*>(&(blocks[k])), 4); // bit block sizes
   }
   for (size_t k = 0; k < scrs.size(); k++)
      codebook.write(reinterpret_cast<char const*>(&scrs[ids[k]]), sizeof(ScoreT));
}

// Remap the preliminary score IDs for a scores column in the .scr file to
// the final score IDs.
void
remap_ids(bio::mapped_file& scr_file, size_t which_scr, uint32_t how_many_scr,
          size_t fields_per_line, vector<uint32_t>& remap)
{
   uint32_t* p = reinterpret_cast<uint32_t*>(scr_file.data()) + which_scr;
   uint32_t* scr_end = reinterpret_cast<uint32_t*>(scr_file.data() + scr_file.size());
   while (p < scr_end)
   {
      for (uint32_t i = 0; i < how_many_scr; ++i) {
         assert(*(p+i) < remap.size());
         *(p+i) = remap[*(p+i)];
      }
      p += fields_per_line;
   }
}

int MAIN(argc, argv)
{
   cerr << "ptable.encode-scores starting." << endl;

   interpret_args(argc,(char **)argv);
   string line;

   // Read the phrase table, storing the scores into the .scr file for fast access.
   string scrName(oBaseName+".scr");
   ofstream tmpFile(scrName.c_str());
   if (tmpFile.fail())
      cerr << efatal << "Unable to open preliminary score file '" << scrName << "' for writing."
           << exit_1;

#if IN_PORTAGE
   iSafeMagicStream in(iFileName);
#else
   filtering_istream in;
   open_input_stream(iFileName,in);
#endif

   cerr << "Reading phrase table." << endl;
   time_t start_time(time(NULL));
   TMEntry init_entry(iFileName);
   uint32_t third_col_scores(0), adir_scores(0), num_counts(0);
   bool has_alignments(false);
   vector<string> initial_lines;
   // how many lines we look at to decide the number of count fields, and
   // whether there are alignments.
   const uint32_t header_size = 100;

   for (uint32_t i = 0; i < header_size; ++i) {
      if (!getline(in, line)) break;
      initial_lines.push_back(line);
      init_entry.newline(line);
      uint32_t third, adir, counts;
      bool al;
      count_scores_per_line(init_entry, third, adir, al, counts);
      third_col_scores = max(third, third_col_scores);
      adir_scores = max(adir, adir_scores);
      has_alignments = has_alignments || al;
      num_counts = max(counts, num_counts);
   }

   if (has_alignments) {
      cerr << ewarn << "The alignment field is not yet supported in the convertion from text phrase tables to TPPTs.  Ignoring all a= fields found in " << iFileName << endl;
      has_alignments = false;
   }

   TMEntry entry(iFileName);
   for (uint32_t i = 0; i < initial_lines.size(); ++i) {
      entry.newline(initial_lines[i]);
      process_line(entry, tmpFile, third_col_scores, num_counts, has_alignments);
   }

   if (initial_lines.size() == header_size) {
      while (getline(in, line)) {
         entry.newline(line);
         if (entry.LineNo() % 10000000 == 0)
            cerr << "Reading phrase table: " << entry.LineNo()/1000000 << "M lines read (..."
                 << (time(NULL) - start_time) << "s)" << endl;
         process_line(entry, tmpFile, third_col_scores, num_counts, has_alignments);
      }
   }
   tmpFile.close();
   cerr << "Read " << entry.LineNo() << " lines in " << (time(NULL) - start_time) << " seconds." << endl;

   // Write the config file for 1) ptable.assemble to be able to interpret its
   // input files, in particular the .scr file, and 2) for TPPT interpretation
   // code to know how to interpret the TPPT.
   if (has_alignments || num_counts || adir_scores) {
      using_v2 = true;
      TPPTConfig::write(oBaseName+".config", third_col_scores, adir_scores,
         num_counts, false /* alignments are not implemented yet... */);
   }

   // Open the .scr (memory-mapped, read/write) and .cbk (write) files
   bio::mapped_file scr;
   try {
      scr.open(scrName, ios::in|ios::out);
      if (!scr.is_open()) throw std::exception();
   } catch(std::exception& e) {
      cerr << efatal << "Unable to open final score file '" << scrName << "' for read/write."
           << exit_1;
   }
   uint32_t scrs_per_line = third_col_scores + adir_scores;
   size_t expected_size = entry.LineNo() * (scrs_per_line+num_counts) * sizeof(uint32_t);
   if (scr.size() != expected_size)
      cerr << efatal << "Incorrect temporary score file size: " << scr.size()
           << "; expected " << expected_size
           << exit_1;

   string cbkName(oBaseName+".cbk");
   ofstream cbk(cbkName.c_str());
   if (cbk.fail())
      cerr << efatal << "Unable to open codebook file '" << cbkName << "' for writing."
           << exit_1;

   if (using_v2) {
      using namespace TPPTConfig;
      // start the new code book file format with 1) 0, so that old code doesn't
      // try to read past what it thinks is the header, and 2) a magic number for
      // easier future changes to the file format.  The new format writes the type
      // (float or uint32_t) before each code book.
      uint32_t zero = 0;
      cbk.write(reinterpret_cast<char*>(&zero), 4);
      cbk.write(code_book_magic_number_v2, strlen(code_book_magic_number_v2));
      assert(strlen(code_book_magic_number_v2) % 4 == 0);
   }

   uint32_t numBooks = scrs_per_line + (num_counts > 0 ? 1 : 0);
   cbk.write(reinterpret_cast<char*>(&numBooks),4); // number of code books.

   // For each score field (column) in the .scr file:
   // 1. Count score occurrences, writing preliminary score IDs to the .scr file.
   // 2. Write its codebook to the .cbk file and get remapping vector mapping
   //    the preliminary score IDs to final score IDs.
   // 3. Remap the preliminary score IDs in the .scr file to the final score IDs.
   for (size_t i = 0; i < scrs_per_line; i++)
   {
      start_time = time(NULL);
      cerr << "Counting scores for code book " << i << "." << endl;
      vector<uint32_t> cnts;
      vector<float> scrs;
      count_scores(scr, i, 1, scrs_per_line+num_counts, scrs, cnts);
      cerr << "Writing code book " << i << "." << endl;
      vector<uint32_t>& remap = cnts;  // cnts doubles as remap vector
      mkCodeBook(cbk, scrs, cnts);
      cerr << "Remapping score ids for code book " << i << "." << endl;
      remap_ids(scr, i, 1, scrs_per_line+num_counts, remap);
      cerr << "Done code book " << i << " in " << (time(NULL) - start_time) << " seconds." << endl;
   }

   // For all the count fields, use just one code book.
   if (num_counts > 0) {
      start_time = time(NULL);
      const Uint i = scrs_per_line;
      cerr << "Counting counts for code book" << i << "." << endl;
      vector<uint32_t> cnts;
      vector<uint32_t> scrs;
      count_scores(scr, i, num_counts, scrs_per_line+num_counts, scrs, cnts);
      cerr << "Writing code book " << i << "." << endl;
      vector<uint32_t>& remap = cnts;  // cnts doubles as remap vector
      mkCodeBook(cbk, scrs, cnts);
      cerr << "Remapping count ids for code book " << i << "." << endl;
      remap_ids(scr, i, num_counts, scrs_per_line+num_counts, remap);
      cerr << "Done code book " << i << " in " << (time(NULL) - start_time) << " seconds." << endl;
   }

   cerr << "ptable.encode-scores finished." << endl;
}
END_MAIN
