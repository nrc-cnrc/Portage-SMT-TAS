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
#include <cstring>
#include <sstream>
#include <limits>  // numeric_limits<float>::min & numeric_limits<float>::max
#include <locale>

#if IN_PORTAGE
#include "file_utils.h"
#include "tm_entry.h"
#include "word_align_io.h"
#include "tp_alignment.h"
#define DO_HUFFMAN_COMPARISON 0
#if DO_HUFFMAN_COMPARISON
#include "huffman.h"
#endif
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

/// The AlignmentCountHandler accepts the alignment and the count field from
/// phrase table, and rejects anything else.
struct AlignmentCountHandler {
   const char* alignment;
   const char* count;
   AlignmentCountHandler() : alignment(NULL), count(NULL) {}
   bool operator()(const char* name, const char* value) {
      if (strcmp(name, "a") == 0) alignment = value;
      else if (strcmp(name, "c") == 0) count = value;
      else {
         error(ETWarn, "ptable.encode-scores only supports the a= and c= field in phrase tables; %s=%s found but not supported",
               name, value);
         return false;
      }
      return true;
   }
};

void
count_scores_per_line(TMEntry& entry, uint32_t& third_col_count, uint32_t& adir_scores, bool& has_alignment, uint32_t& num_counts)
{
   third_col_count = entry.ThirdCount();
   adir_scores = entry.FourthCount();
   double v[third_col_count];
   AlignmentCountHandler handler;
   entry.parseThird(v, handler);
   if (third_col_count > 0 && v[third_col_count-1] > 2.7179 && v[third_col_count-1] < 2.7181)
      --third_col_count; // moses-style phrase table
   has_alignment = (handler.alignment != NULL);
   vector<double> dummy_counts;
   num_counts = handler.count ? split(handler.count, dummy_counts, ",") : 0;
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

const bool debug_scr_file = false;

void
process_line(TMEntry& entry, ostream& prelimScoresFile,
             ostream& prelimAlignmentFile, size_t& prelimAlignmentPosn,
             uint32_t third_col_scores, uint32_t num_counts,
             bool has_alignments, vector<uint32_t>& alignment_code_distn)
{
   assert(third_col_scores <= entry.ThirdCount());
   double v[entry.ThirdCount()];
   AlignmentCountHandler handler;
   entry.parseThird(v, handler);
   for (uint32_t i = 0; i < third_col_scores; ++i) {
      float s = trim_double_to_float(v[i]);
      prelimScoresFile.write(reinterpret_cast<char*>(&s),sizeof(s));
      if (debug_scr_file)
         fprintf(stderr, "SCR float: %f (\\x%08x)\n", s, *(reinterpret_cast<uint32_t*>(&s)));
   }
   double a[entry.FourthCount()];
   entry.parseFourth(a);
   for (uint32_t i = 0; i < entry.FourthCount(); ++i) {
      float s = trim_double_to_float(a[i]);
      prelimScoresFile.write(reinterpret_cast<char*>(&s),sizeof(s));
      if (debug_scr_file)
         fprintf(stderr, "SCR float: %f (\\x%08x)\n", s, *(reinterpret_cast<uint32_t*>(&s)));
   }
   if (num_counts) {
      vector<uint32_t> counts;

      if (handler.count) {
         // Read counts has double in order to support counts written in scientific notation.
         vector<double> temp_counts;
         split(handler.count, temp_counts, ",");
         if (!temp_counts.empty()) {
            counts.reserve(temp_counts.size());
            // Counts in tppt models are stored as integer thus lets convert our double to integer.
            for (Uint i(0); i<temp_counts.size(); ++i) {
               const uint32_t converted = (uint32_t) temp_counts[i];
               // This doesn't cover all cases but most likely will catch a
               // frational count before the end of the conversion.
               if (fabs(temp_counts[i] - converted) > 0.001)
                  error(ETFatal, "TPPT's count fields cannot be fractional: %f", temp_counts[i]);
               counts.push_back(converted);
            }
         }
      }
      if (counts.size() > num_counts)
         cerr << efatal << "Too many counts on line " << entry.LineNo()
              << " in file " << entry.File() << exit_1;
      if (counts.size() < num_counts)
         counts.resize(num_counts, 0);
      for (uint32_t i = 0; i < num_counts; ++i) {
         uint32_t count = (i < counts.size() ? counts[i] : 0);
         prelimScoresFile.write(reinterpret_cast<char*>(&count),sizeof(count));
         if (debug_scr_file)
            fprintf(stderr, "SCR uint32_t: %u (\\x%08x)\n", count, count);
      }
   }
   if (has_alignments) {
      // The scores file is used to tell us where in the alignment we will find
      // the alignment information for the given phrase pair.
      prelimScoresFile.write(reinterpret_cast<char*>(&prelimAlignmentPosn),sizeof(prelimAlignmentPosn));
      if (debug_scr_file)
         fprintf(stderr, "SCR size_t: %lu (\\x%016lx)\n",
            static_cast<long unsigned int>(prelimAlignmentPosn),
            static_cast<long unsigned int>(prelimAlignmentPosn));

      static vector< vector<Uint> > sets;
      if (handler.alignment && *handler.alignment)
         GreenReader('_')(handler.alignment, sets);
      else
         sets.clear();
      // alignment_links is a vector of 64-bit uint, not 16, to detect and
      // report overflow, and to match AlignmentLink::pack()'s return type:
      static vector<Uint64> alignment_links;
      alignment_links.clear();
      for (Uint i = 0; i < sets.size(); ++i) {
         if (sets[i].empty()) {
            alignment_links.push_back(AlignmentLink().pack());
         } else {
            for (Uint j = 0; j + 1 < sets[i].size(); ++j)
               alignment_links.push_back(AlignmentLink(sets[i][j], false).pack());
            alignment_links.push_back(AlignmentLink(sets[i].back(), true).pack());
         }
      }

      // Temporary encoding, not fully packed: use a short for each encoded
      // link, which can never have value 0, and a short with value 0 to mark
      // the end.  In the final TPPT format, created by ptable.assemble.cc,
      // we'll pack the same information with the smallest number of bits per
      // link we can get away with, typically 4 or 5: 4 bits if the longest
      // phrase in the phrase table has <= 7 tokens, 5 if it has <= 15 tokens.
      size_t al_size = alignment_links.size();
      for (uint16_t i = 0; i < al_size; ++i) {
         uint16_t short_link = alignment_links[i];
         assert(short_link == alignment_links[i]);
         if (short_link >= alignment_code_distn.size()) {
            alignment_code_distn.resize(short_link+1, 0);
         }
         ++alignment_code_distn[short_link];
         prelimAlignmentFile.write(reinterpret_cast<char*>(&short_link),sizeof(short_link));
         ++prelimAlignmentPosn;
      }
      uint16_t end_marker = 0;
      prelimAlignmentFile.write(reinterpret_cast<char*>(&end_marker),sizeof(end_marker));
      ++prelimAlignmentPosn;
   }
}

//typedef tr1::unordered_map<float, uint32_t> scores_map_t;
//typedef map<float, uint32_t> scores_map_t; // use to reproduce Uli's original output
//vector<uint32_t> ids;  // use to reproduce Uli's original sort order.

// Count score occurrences, writing preliminary score IDs to the .scr file.
// Fill the scrs and cnts vectors, ordered by id, with scores and their
// corresponding counts, respectively.
// Returns the total count, over all scores. (== sum(cnts))
template <class ScoreT>
uint64_t
count_scores(bio::mapped_file &scr_file, uint32_t which_scr, uint32_t how_many_scr,
             size_t fields_per_line,
             vector<ScoreT>& scrs, vector<uint32_t>& cnts)
{
   assert(sizeof(ScoreT) == 4);
   assert(which_scr + how_many_scr <= fields_per_line);

   uint64_t total_count = 0;

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
         ++total_count;
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

   return total_count;
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
mkCodeBook(ostream& codebook, vector<ScoreT>& scrs, vector<uint32_t>& cnts, bool no_sort = false)
{
   #if DO_HUFFMAN_COMPARISON
   // Check how much smaller the Huffman encoding would be.
   { // extra scope so these large structures get deleted as soon as we're done.
      vector<pair<ScoreT,uint32_t> > scores_for_huffman;
      scores_for_huffman.reserve(scrs.size());
      for (Uint i = 0; i < scrs.size(); ++i)
         scores_for_huffman.push_back(make_pair(scrs[i], cnts[i]));
      HuffmanCoder<ScoreT> huffman(scores_for_huffman.begin(), scores_for_huffman.end());
      cerr << "Huffman encoding would have total cost in bits: " << uint64_t(huffman.totalCost()) << endl;
   }
   #endif

   assert(sizeof(ScoreT) == 4);
   TPT_DBG(cerr << "mkCodeBook: scrs.size = " << scrs.size() << endl);
   vector<uint32_t>& remap = cnts;  // cnts doubles as the remap vector.

   // Note: comment out the next 3 lines when reproducing Uli's original output.
   vector<uint32_t> ids(scrs.size());
   for (size_t i = 0; i < scrs.size(); ++i)
      ids[i] = i;
   if (!no_sort)
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
   assert(p);
   uint32_t* scr_end = reinterpret_cast<uint32_t*>(scr_file.data() + scr_file.size());
   assert(scr_end);
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
   cerr.imbue(locale(""));

   interpret_args(argc,(char **)argv);
   string line;

   // Read the phrase table, storing the scores into the .scr file for fast access.
   string scrName(oBaseName+".scr");
   ofstream prelimScoresFile(scrName.c_str());
   if (prelimScoresFile.fail())
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
   const uint32_t header_size = 10000;

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

   ofstream prelimAlignmentFile;
   size_t prelimAlignmentPosn(0);
   if (has_alignments) {
      string alFileName(oBaseName+".aln");
      prelimAlignmentFile.open(alFileName.c_str(), ios_base::out|ios_base::trunc);
      if (prelimAlignmentFile.fail())
      cerr << efatal << "Unable to open preliminary alignment file '" << alFileName << "' for writing."
           << exit_1;
   }

   TMEntry entry(iFileName);
   vector<uint32_t> alignment_code_distn;
   for (uint32_t i = 0; i < initial_lines.size(); ++i) {
      entry.newline(initial_lines[i]);
      process_line(entry, prelimScoresFile, prelimAlignmentFile, prelimAlignmentPosn,
                   third_col_scores, num_counts, has_alignments, alignment_code_distn);
   }

   if (initial_lines.size() == header_size) {
      while (getline(in, line)) {
         entry.newline(line);
         if (entry.LineNo() % 10000000 == 0)
            cerr << "Reading phrase table: " << entry.LineNo()/1000000 << "M lines read (..."
                 << (time(NULL) - start_time) << "s)" << endl;
         process_line(entry, prelimScoresFile, prelimAlignmentFile, prelimAlignmentPosn,
                      third_col_scores, num_counts, has_alignments, alignment_code_distn);
      }
   }
   prelimScoresFile.close();
   if (has_alignments) prelimAlignmentFile.close();
   cerr << "Read " << entry.LineNo() << " lines in " << (time(NULL) - start_time) << " seconds." << endl;

   // Write the config file for 1) ptable.assemble to be able to interpret its
   // input files, in particular the .scr file, and 2) for TPPT interpretation
   // code to know how to interpret the TPPT.
   if (has_alignments || num_counts || adir_scores) {
      using_v2 = true;
      TPPTConfig::write(oBaseName+".config", third_col_scores, adir_scores,
         num_counts, has_alignments);
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
   size_t scrs_per_line = third_col_scores + adir_scores;
   size_t fields_per_line = scrs_per_line + num_counts +
      (has_alignments ? 1 : 0) * (sizeof(prelimAlignmentPosn)/sizeof(uint32_t));
   size_t expected_size = size_t(entry.LineNo()) * fields_per_line * sizeof(uint32_t);
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

   uint32_t numBooks = scrs_per_line + (num_counts > 0 ? 1 : 0) + (has_alignments ? 1 : 0);
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
      uint64_t total_count = count_scores(scr, i, 1, fields_per_line, scrs, cnts);
      cerr << "Writing code book " << i << " to encode " << total_count << " scores with "
           << scrs.size() << " distinct values." << endl;
      vector<uint32_t>& remap = cnts;  // cnts doubles as remap vector
      mkCodeBook(cbk, scrs, cnts);
      cerr << "Remapping score ids for code book " << i << "." << endl;
      remap_ids(scr, i, 1, fields_per_line, remap);
      cerr << "Done code book " << i << " in " << (time(NULL) - start_time) << " seconds." << endl;
   }

   // For all the count fields, use just one code book.
   if (num_counts > 0) {
      start_time = time(NULL);
      const Uint i = scrs_per_line;
      cerr << "Counting counts for code book" << i << "." << endl;
      vector<uint32_t> cnts;
      vector<uint32_t> scrs;
      uint64_t total_count = count_scores(scr, i, num_counts, fields_per_line, scrs, cnts);
      cerr << "Writing code book " << i << " to encode " << total_count << " counts with "
           << scrs.size() << " distinct values." << endl;
      vector<uint32_t>& remap = cnts;  // cnts doubles as remap vector
      mkCodeBook(cbk, scrs, cnts);
      cerr << "Remapping count ids for code book " << i << "." << endl;
      remap_ids(scr, i, num_counts, fields_per_line, remap);
      cerr << "Done code book " << i << " in " << (time(NULL) - start_time) << " seconds." << endl;
   }

   // For the links in the alignment field, we also use one code book.
   // Calculate the number of bits needed per alignment code
   if (has_alignments) {
      start_time = time(NULL);
      assert(alignment_code_distn[0] == 0);
      alignment_code_distn[0] = entry.LineNo(); // We NULL-terminate each alignment string.

      vector<uint32_t> links(alignment_code_distn.size());
      uint32_t total_count(0);
      for (uint32_t i = 0; i < alignment_code_distn.size(); ++i) {
         links[i] = i;
         total_count += alignment_code_distn[i];
      }

      cerr << "Writing code book to encode " << total_count << " alignment links with "
           << links.size() << " distinct values." << endl;
      mkCodeBook(cbk, links, alignment_code_distn, /*no_sort=*/true);
      cerr << "Done alignment link code book in " << (time(NULL) - start_time) << " seconds." << endl;
   }

   cerr << "ptable.encode-scores finished." << endl;
}
END_MAIN
