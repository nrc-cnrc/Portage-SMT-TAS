/**
 * @author George Foster / Eric Joanis
 * @file ibm.cc  Implementation of GizaAlignmentFile, IBM1 and IBM2.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <cmath>
#include <numeric>
#include <iostream>
#include "binio.h"
#include "errors.h"
#include "ibm.h"
#include "tmp_val.h"
#include "hmm_aligner.h"

using namespace Portage;

/*----------------------------------------------------------------------------
Giza2AlignmentFile
----------------------------------------------------------------------------*/

Giza2AlignmentFile::Giza2AlignmentFile(string& filename)
   : GizaAlignmentFile()
{
   sent_count = 0;
   p_in = new iMagicStream(filename);
   if (!p_in)
      error(ETFatal, "GizaAlignmentFile: Failed to open alignment file "
                     +filename);
}

Giza2AlignmentFile::Giza2AlignmentFile(istream* in)
   : GizaAlignmentFile()
   , p_in(in)
{
   sent_count = 0;
   assert(in != NULL);
}

Giza2AlignmentFile::~Giza2AlignmentFile()
{
   delete p_in;
}

void
Giza2AlignmentFile::align(const vector<string>& src, const vector<string>& tgt,
                         vector<Uint>& tgt_al, bool twist,
                         vector<double>* tgt_al_probs)
{
   // How many fields are expected in the alignment description line aka first line.
   static const Uint NUMBER_OF_EXPECTED_FIELD = 14;
   /*
   # Sentence pair (1) source length 11 target length 16 alignment score : 3.63487e-24
   m. hulchanski est l' un des plus éminents spécialistes du canada en matière de logement .
   NULL ({ 14 }) dr. ({ 1 }) hulchanski ({ 2 }) is ({ 3 }) one ({ 4 5 }) of ({ 6 }) canada ({ 11 }) 's ({ }) foremost ({ 7 8 9 10 12 13 }) housing ({ 15 }) experts ({ }) . ({ 16 })
   */

   ++sent_count;

   tgt_al.clear();
   tgt_al.resize(tgt.size(), src.size());
   if (tgt_al_probs) {
      tgt_al_probs->clear();
      tgt_al_probs->resize(tgt.size(), src.size());
   }

   string line;

   // First line in "# Sentence pair (1) source length 11 target length 16 alignment score : 3.63487e-24"
   vector<string> field;
   if (!getline(*p_in, line))
      error(ETFatal, "Giza2AlignmentFile (sent %d): Unexpectedly reached end of file",
            sent_count);
   split(line, field);
   if (field.size() != NUMBER_OF_EXPECTED_FIELD || !isPrefix("# Sentence pair (", line.c_str()))
      error(ETFatal, "Giza2AlignmentFile (sent %d): Expected \"# Sentence pair (%d)....\", got \"%s\"",
            sent_count, sent_count, line.c_str());

   // Check sentence id?
   // Check source length
   Uint sentenceLength = 0;
   if (!conv(field[6], sentenceLength) or sentenceLength != src.size()) {
      copy(src.begin(), src.end(), ostream_iterator<string>(cerr, ":")); cerr << endl;
      error(ETWarn, "Giza2AlignmentFile (send %d): Expected source length %d, got %d", sent_count, src.size(), sentenceLength);
   }
   // Check target length
   if (!conv(field[9], sentenceLength) or sentenceLength != tgt.size())
      error(ETWarn, "Giza2AlignmentFile (send %d): Expected target length %d, got %d", sent_count, tgt.size(), sentenceLength);


   // Read target
   if (!getline(*p_in, line))
      error(ETFatal, "Giza2AlignmentFile (send %d): Corrupted file, there is no target.", sent_count);


   // Read source sentence that contain alignments.
   if (!getline(*p_in, line))
      error(ETFatal, "Giza2AlignmentFile (send %d): Corrupted file, there is no source.", sent_count);
   field.clear();
   split(line, field);
   // We initialize src_index to -1 since the source sentence in GIZA-v2's
   // format starts with a NULL token which is not a word that is part of the
   // original sentence.
   int src_index = -1;
   for (vector<string>::const_iterator token(field.begin()); token!=field.end(); ++token) {
      if (*token == "({") {
         ++token;
         while (*token != "})") {
            Uint tgt_index = 0;
            if (!conv(*token, tgt_index))
               error(ETFatal, "Giza2AlignmentFile (send %d): Unable to convert to Uint (%s).", sent_count, token->c_str());
            tgt_index -= 1;  // Make the index a 0-based index.
            if (tgt_index < tgt_al.size())
               // Are we processing the NULL token?
               tgt_al[tgt_index] = (src_index == -1 ? src.size() : src_index);
            else
               error(ETFatal, "Giza2AlignmentFile (send %d): Error tgt_index (%d, %d)", sent_count, tgt_index, tgt_al.size());
            ++token;
         }
         ++src_index;
      }
   }

   if ((Uint)src_index != src.size())
      error(ETFatal, "Giza2AlignmentFile (sent %d): Error src_index (%d, %d)", sent_count, src_index, src.size());
}


/*----------------------------------------------------------------------------
GizaAlignmentFile
----------------------------------------------------------------------------*/

GizaAlignmentFile::GizaAlignmentFile()
   : sent_count(0)
{}

GizaAlignmentFile::GizaAlignmentFile(string& filename)
   : in(filename)
   , sent_count(0)
{
   if (!in)
      error(ETFatal, "GizaAlignmentFile: Failed to open alignment file "
                     +filename);
}

GizaAlignmentFile::~GizaAlignmentFile() {}

void
GizaAlignmentFile::align(const vector<string>& src, const vector<string>& tgt,
                         vector<Uint>& tgt_al, bool twist,
                         vector<double>* tgt_al_probs) {
   ++sent_count;

   tgt_al.clear();
   tgt_al.resize(tgt.size(), 0);
   if (tgt_al_probs) {
      tgt_al_probs->clear();
      tgt_al_probs->resize(tgt.size(), 0);
   }

   string line;

   // First line in "SENT ID"
   vector<string> field;
   if (!getline(in, line))
      error(ETFatal, "GizaAlignmentFile (sent %d): Unexpectedly reached end of file",
            sent_count);
   split(line, field);
   if (field.size() < 2 || field[0] != "SENT:")
      error(ETFatal, "GizaAlignmentFile (sent %d): Expected \"SENT ID\", got \"%s\"",
            sent_count, line.c_str());
   // string sent_id = field[1];

   // Subsequent lines are "[S|P] SRC TGT"
   vector<Uint> pos;
   while(getline(in, line) && !line.empty()) {
      field.clear();
      split(line, field, " ", 2);
      if (field.size() != 2 || !(field[0] == "P" || field[0] == "S"))
         error(ETFatal, "GizaAlignmentFile (sent %d): Expected \"[S|P] SRC TGT\", got \"%s\"",
               sent_count, line.c_str());

      pos.clear();
      split(field[1], pos);
      if (pos.size() != 2)
         error(ETFatal, "GizaAlignmentFile (sent %d): Expected \"[S|P] SRC TGT\", got \"%s\"",
               sent_count, line.c_str());
      if (pos[0] >= src.size())
         error(ETFatal, "GizaAlignmentFile (sent %d): src position %d exceeds src sent size %d",
               sent_count, pos[0], src.size());
      if (pos[1] >= tgt.size())
         error(ETFatal, "GizaAlignmentFile (sent %d): tgt position %d exceeds tgt sent size %d",
               sent_count, pos[1], tgt.size());
      tgt_al[pos[1]] = pos[0];
   }
}

/*----------------------------------------------------------------------------
IBM1
----------------------------------------------------------------------------*/


void IBM1::createIBMModelPair(IBM1*& ibm_1g2, IBM1*& ibm_2g1,
      const string& ibm_1g2_filename, const string& ibm_2g1_filename,
      string ibmtype, bool verbose)
{
   if (ibm_1g2_filename.empty() || ibm_2g1_filename.empty())
      error(ETFatal, "IBM models in both directions are required");

   if (ibmtype == "") {
      if (check_if_exists(HMMAligner::distParamFileName(ibm_1g2_filename)) &&
          check_if_exists(HMMAligner::distParamFileName(ibm_2g1_filename)))
         ibmtype = "hmm";
      else if (check_if_exists(IBM2::posParamFileName(ibm_1g2_filename)) &&
               check_if_exists(IBM2::posParamFileName(ibm_2g1_filename)))
         ibmtype = "2";
      else
         ibmtype = "1";
   }

   string namepair = ibm_1g2_filename + " + " + ibm_2g1_filename;

   ibm_1g2 = ibm_2g1 = NULL;
   if (ibmtype == "hmm") {
      if (verbose) cerr << "Loading HMM models " + namepair << endl;
      ibm_1g2 = new HMMAligner(ibm_1g2_filename);
      ibm_2g1 = new HMMAligner(ibm_2g1_filename);
   } else if (ibmtype == "1") {
      if (verbose) cerr << "Loading IBM1 models " + namepair << endl;
      ibm_1g2 = new IBM1(ibm_1g2_filename);
      ibm_2g1 = new IBM1(ibm_2g1_filename);
   } else if (ibmtype == "2") {
      if (verbose) cerr << "Loading IBM2 models " + namepair << endl;
      ibm_1g2 = new IBM2(ibm_1g2_filename);
      ibm_2g1 = new IBM2(ibm_2g1_filename);
   } else {
      error(ETFatal, "Invalid ibmtype specification: %s", ibmtype.c_str());
   }
}

void IBM1::write(const string& ttable_file, bool bin_ttable) const
{
   oSafeMagicStream out(ttable_file);
   if ( bin_ttable )
      tt.write_bin(out);
   else
      tt.write(out);
}

void IBM1::writeBinCounts(const string& count_file) const
{
   using namespace BinIO;
   oSafeMagicStream os(count_file);
   // Header
   os << "NRC PORTAGE IBM1 count file v1.0" << endl;
   // Global figures
   os << num_toks << ":";
   writebin(os, logprob);
   // Counts
   for ( Uint i(0); i < counts.size(); ++i ) {
      os << i << ":";
      writebin(os, counts[i]);
   }
   // Footer
   os << "End of NRC PORTAGE IBM1 count file v1.0" << endl;
}

void IBM1::readAddBinCounts(const string& count_file)
{
   using namespace BinIO;
   iSafeMagicStream is(count_file);

   // Header
   string line;
   if ( !getline(is, line) )
      error(ETFatal, "No input in bin IBM1 count file %s", count_file.c_str());
   if ( line != "NRC PORTAGE IBM1 count file v1.0" )
      error(ETFatal, "File %s doesn't look like a binary IBM1 count file",
            count_file.c_str());

   // Global figures
   Uint num_toks_read(0);
   char c;
   is >> num_toks_read >> c;
   if ( c != ':' )
      error(ETFatal, "Bad bin IBM1 file format in %s at beginning",
            count_file.c_str());
   num_toks += num_toks_read;
   double logprob_read(0);
   assert(sizeof(logprob_read) == sizeof(logprob));
   readbin(is, logprob_read);
   logprob += logprob_read;

   // Counts
   vector<float> count_line;
   for ( Uint i(0); i < counts.size(); ++i ) {
      Uint i_read(0);
      is >> i_read >> c;
      if ( i_read != i || c != ':' )
         error(ETFatal, "Bad bin IBM1 file format in %s at i=%d",
               count_file.c_str(), i);
      readbin(is, count_line);
      if ( count_line.size() != counts[i].size() )
         error(ETFatal, "Format error in IBM1 file %s at i=%d: expected %d counts, got %d",
               count_file.c_str(), i, counts[i].size(), count_line.size());
      for ( Uint j(0); j < count_line.size(); ++j )
         counts[i][j] += count_line[j];
   }

   // Footer
   if ( !getline(is, line) )
      error(ETFatal, "Missing footer in bin IBM1 count file %s",
            count_file.c_str());
   if ( line != "End of NRC PORTAGE IBM1 count file v1.0" )
      error(ETFatal, "Bad footer in bin IBM1 count file %s",
            count_file.c_str());
}

void IBM1::add(vector<string>& src_toks, const vector<string>& tgt_toks,
               bool use_null)
{
   if ( use_null ) src_toks.push_back(nullWord());
   tt.add(src_toks, tgt_toks);
   if ( use_null ) src_toks.pop_back();
}

void IBM1::initCounts()
{
   counts.resize(tt.numSourceWords());
   for (Uint i = 0; i < tt.numSourceWords(); ++i) {
      counts[i].resize(tt.getSourceDistn(i).size());
      counts[i].assign(tt.getSourceDistn(i).size(), (float)0.0);
   }
   num_toks = 0;
   logprob = 0.0;
}

void IBM1::count(const vector<string>& src_toks,
                 const vector<string>& tgt_toks,
                 bool use_null)
{
   const Uint base = (use_null || useImplicitNulls) ? 1 : 0;
   const Uint src_size = src_toks.size() + base;
   vector<int> offsets(src_size);

   for (Uint i = 0; i < tgt_toks.size(); ++i) {

      double sum = 0.0;
      const Uint tindex = tt.targetIndex(tgt_toks[i]);
      if (tindex == tt.numTargetWords()) continue;

      for (Uint j = 0; j < src_size; ++j) {
         const TTable::SrcDistn& src_distn =
            tt.getSourceDistn(j < base ? nullWord() : src_toks[j-base]);
         offsets[j] = tt.targetOffset(tindex, src_distn);
         if (offsets[j] != -1)
            sum += src_distn[offsets[j]].second;
      }

      for (Uint j = 0; j < src_size; ++j) {
         if (offsets[j] == -1) continue;
         const Uint src_index =
            tt.sourceIndex(j < base ? nullWord() : src_toks[j-base]);
         counts[src_index][offsets[j]] +=
            tt.getSourceDistn(src_index)[offsets[j]].second / sum;
      }

      if (sum != 0.0) {
         logprob += log(sum / src_size);
         ++num_toks;
      }
   }
}

void IBM1::count_symmetrized(const vector<string>& src_toks,
                             const vector<string>& tgt_toks,
                             bool use_null, IBM1* r)
{
   use_null = use_null || useImplicitNulls;

   if ( (! src_toks.empty() && src_toks[0] == nullWord()) ||
        (! tgt_toks.empty() && tgt_toks[0] == nullWord()) )
      error(ETWarn, "Using explicit nulls with IBM1::count_symmetrized() will "
                    "have incorrect effects on the models.");

   // Posteriors for tgt as the observed sequence, src as the hidden one.
   // posteriors[i][j] has p(linking tgt[i] to src[j]).
   // If use_null, posteriors[i][src_toks.size()] has p(null aligning tgt[i]).
   vector<vector<double> > posteriors;
   double log_pr;
   {
      tmp_val<bool> tmp_useImplicitNulls(useImplicitNulls, use_null);
      log_pr = IBM1::linkPosteriors(src_toks, tgt_toks, posteriors);
   }

   // Posteriors for src as the observed sequence, tgt as the hidden one.
   // r_posteriors[j][i] has p_r(linking src[j] to tgt[i]).
   // If use_null, r_posteriors[j][tgt_toks.size()] has p_r(null al. src[j]).
   vector<vector<double> > r_posteriors;
   double r_log_pr;
   {
      tmp_val<bool> tmp_useImplicitNulls(r->useImplicitNulls, use_null);
      r_log_pr = r->IBM1::linkPosteriors(tgt_toks, src_toks, r_posteriors);
   }

   // Update this model's counts using the products of posteriors.
   IBM1::count_sym_helper(src_toks, tgt_toks, use_null,
                          posteriors, r_posteriors);
   logprob += log_pr;

   // Update r's counts using the products of posteriors
   r->IBM1::count_sym_helper(tgt_toks, src_toks, use_null,
                             r_posteriors, posteriors);
   r->logprob += r_log_pr;
}

void IBM1::count_sym_helper(const vector<string>& src_toks,
                            const vector<string>& tgt_toks,
                            bool use_null,
                            const vector<vector<double> >& posteriors,
                            const vector<vector<double> >& r_posteriors)
{
   assert(posteriors.size() == tgt_toks.size());
   assert(posteriors.empty() ||
          posteriors[0].size() == src_toks.size() + (use_null?1:0));
   assert(r_posteriors.size() == src_toks.size());
   assert(r_posteriors.empty() ||
          r_posteriors[0].size() == tgt_toks.size() + (use_null?1:0));

   for (Uint i = 0; i < tgt_toks.size(); ++i) {

      const Uint tindex = tt.targetIndex(tgt_toks[i]);
      if (tindex == tt.numTargetWords()) continue;

      for ( Uint j = 0; j < src_toks.size(); ++j ) {
         const Uint src_index = tt.sourceIndex(src_toks[j]);
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
         const int offset = tt.targetOffset(tindex, src_distn);
         if (offset != -1)
            counts[src_index][offset] += posteriors[i][j] * r_posteriors[j][i];
      }
      if ( use_null ) {
         const Uint src_index = tt.sourceIndex(nullWord());
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
         const int offset = tt.targetOffset(tindex, src_distn);
         if (offset != -1) {
            double r_nullprob = 1;
            for ( Uint j_prime = 0; j_prime < src_toks.size(); ++j_prime )
               r_nullprob *= (1 - r_posteriors[j_prime][i]);
            counts[src_index][offset] +=
               posteriors[i][src_toks.size()] * r_nullprob;
         }
      }

      ++num_toks;
   }
}

pair<double,Uint> IBM1::estimate(double pruning_threshold,
                                 double null_pruning_threshold)
{
   Uint zero_counts = 0;
   for (Uint i = 0; i < tt.numSourceWords(); ++i) {
      double sum = 0.0;
      for (Uint j = 0; j < counts[i].size(); ++j)
         sum += counts[i][j];
      if (sum == 0.0)
         ++zero_counts;
      else
         for (Uint j = 0; j < counts[i].size(); ++j)
            tt.prob(i, j) = counts[i][j] / sum;
   }

   if ( zero_counts )
      error(ETWarn, "IBM1::estimate(): Zero counts for %u out of %u "
            "source words, kept previous distribution for each of them.",
            zero_counts, tt.numSourceWords());

   Uint size = tt.prune(pruning_threshold, null_pruning_threshold, nullWord());

   return make_pair(exp(-logprob / num_toks),size);
}

// is spos1 closer to tpos than spos2?
bool IBM1::closer(Uint spos1, Uint spos2, Uint slen,
                  Uint tpos, Uint tlen, bool twist)
{
   if (twist) {
      spos1 = slen - spos1 - 1;
      spos2 = slen - spos2 - 1;
   }
   double srel1 = spos1 / (double) slen;
   double srel2 = spos2 / (double) slen;
   double trel = tpos / (double) tlen;
   return abs(srel1 - trel) < abs(srel2 - trel);
}

double IBM1::pr(const vector<string>& src_toks, const string& tgt_tok,
                vector<double>* probs)
{
   Uint base = useImplicitNulls ? 1 : 0;
   if (probs) (*probs).assign(src_toks.size() + base, 0.0);

   Uint tindex = tt.targetIndex(tgt_tok);
   if (tindex == tt.numTargetWords())
      return 0.0;

   double p = 0.0;
   for (Uint i = 0; i < src_toks.size(); ++i) {
      const TTable::SrcDistn& distn = tt.getSourceDistn(src_toks[i]);
      int offset = tt.targetOffset(tindex, distn);
      if (offset != -1) {
         p += distn[offset].second;
         if (probs) (*probs)[base+i] = distn[offset].second;
      }
   }

   Uint num_src = src_toks.size();

   if (useImplicitNulls) {
      const TTable::SrcDistn& distn = tt.getSourceDistn(nullWord());
      int offset = tt.targetOffset(tindex, distn);
      if (offset != -1) {
         p += distn[offset].second;
         if (probs) (*probs)[0] = distn[offset].second;
      }
      ++num_src;
   }

   return num_src == 0 ? 0.0 : p / num_src;
}

double IBM1::logpr(const vector<string>& src_toks,
                   const vector<string>& tgt_toks,
                   double smooth)
{
   double lp = 0, logsmooth = log(smooth);
   for (Uint i = 0; i < tgt_toks.size(); ++i) {
      double tp = IBM1::pr(src_toks, tgt_toks[i]);
      lp += tp == 0 ? logsmooth : log(tp);
   }
   return lp;
}

double IBM1::minlogpr(const vector<string>& src_toks, const string& tgt_tok,
                      double smooth)
{
   vector<double> probs;
   IBM1::pr(src_toks, tgt_tok, &probs);
   double tp = *max_element(probs.begin(), probs.end());
   double lp = tp == 0 ? log(smooth) : log(tp);
   return lp;
}

double IBM1::minlogpr(const string& src_tok, const vector<string>& tgt_toks,
                      double smooth)
{
   vector<double> probs;
   for ( vector<string>::const_iterator itr=tgt_toks.begin();
         itr!=tgt_toks.end(); itr++ )
      probs.push_back( IBM1::pr(vector<string>(1,src_tok), *itr) );
   double tp = *max_element(probs.begin(), probs.end());
   double lp = tp == 0 ? log(smooth) : log(tp);
   return lp;
}

void IBM1::align(const vector<string>& src, const vector<string>& tgt,
                 vector<Uint>& tgt_al, bool twist,
                 vector<double>* tgt_al_probs)
{
   tgt_al.resize(tgt.size());
   if (tgt_al_probs)
      tgt_al_probs->resize(tgt.size());

   for (Uint i = 0; i < tgt.size(); ++i) {

      double max_pr = -1.0;
      tgt_al[i] = src.size();   // this value means unaligned

      for (Uint j = 0; j < src.size(); ++j) {
         const double pr = tt.getProb(src[j], tgt[i],
               (j==0 && src[j]==nullWord() ? 1e-10 : 1e-10));
         if ( pr > max_pr ||
              (pr == max_pr && pr != -1 &&
                 closer(j, tgt_al[i], src.size(), i, tgt.size(),twist))) {
            max_pr = pr;
            tgt_al[i] = j;
            if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
         }
      }
      if (useImplicitNulls && tt.getProb(nullWord(), tgt[i], 1e-10) > max_pr) {
         tgt_al[i] = src.size();
         if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
      }
   }
}

double IBM1::linkPosteriors(
   const vector<string>& src, const vector<string>& tgt,
   vector<vector<double> >& posteriors)
{
   const Uint I = tgt.size();
   const Uint J = useImplicitNulls ? src.size() : src.size() - 1;

   // resize and initialize posteriors with 0.0 in each cell.
   posteriors.assign(I, vector<double>(J+1, 0.0));
   assert(posteriors.size() == I);
   if ( I > 0 ) {
      assert(posteriors[0].size() == J + 1);
      assert(posteriors[I-1][J] == 0.0);
   }

   double log_pr = 0;

   double numerators[J+1];
   for (Uint i = 0; i < I; ++i ) {
      double sum(0);
      for (Uint j = 0; j < src.size(); ++j ) {
         const double lex_pr = tt.getProb(src[j], tgt[i],
               (j==0 && src[j]==nullWord() ? 1e-10 : 1e-10));
         sum += numerators[j] = lex_pr;
      }
      if ( useImplicitNulls ) {
         const double lex_pr = tt.getProb(nullWord(), tgt[i], 1e-10);
         sum += numerators[src.size()] = lex_pr;
      }
      if ( sum == 0 ) {
         // no data, not even for null alignment! use uniform
         for (Uint j = 0; j <= J; ++j)
            posteriors[i][j] = 1.0/(J+1);
      } else {
         for (Uint j = 0; j <= J; ++j)
            posteriors[i][j] = numerators[j] / sum;
         log_pr += log(sum / (J+1));
      }
   }

   return log_pr;
}

void IBM1::testReadWriteBinCounts(const string& count_file) const
{
   cerr << "Checking IBM1 read/write bin_counts in " << count_file << endl;
   IBM1::writeBinCounts(count_file);
   IBM1 copy;
   // Do a deep copy of the TTable - slow but OK since this is just testing.
   copy.tt = tt;
   copy.initCounts();
   copy.readAddBinCounts(count_file);
   assert(counts.size() == copy.counts.size());
   for ( Uint i(0); i < counts.size(); ++i ) {
      assert(counts[i].size() == copy.counts[i].size());
      for ( Uint j(0); j < counts[i].size(); ++j )
         if ( counts[i][j] != copy.counts[i][j] )
            error(ETWarn, "Different counts[%d][%d] orig=%g copy=%g",
                  i, j, counts[i][j], copy.counts[i][j]);
   }

   copy.readAddBinCounts(count_file);
   assert(counts.size() == copy.counts.size());
   for ( Uint i(0); i < counts.size(); ++i ) {
      assert(counts[i].size() == copy.counts[i].size());
      for ( Uint j(0); j < counts[i].size(); ++j )
         if ( counts[i][j] + counts[i][j] != copy.counts[i][j] )
            error(ETWarn, "Different doubled counts[%d][%d] orig=%g copy=%g",
                  i, j, counts[i][j], copy.counts[i][j]);
   }
   cerr << "Done checking IBM1 read/write bin_counts in " << count_file << endl;
}


/*----------------------------------------------------------------------------
IBM2
----------------------------------------------------------------------------*/

void IBM2::createProbTables()
{
   sblock_size = max_slen * (max_slen + 1) / 2;
   npos_params = max_tlen * (max_tlen + 1) * sblock_size / 2;
   pos_probs = new float[npos_params];

   backoff_probs = new float[backoff_size * backoff_size];
}

void IBM2::initProbTables()
{
   for (Uint tlen = 1; tlen <= max_tlen; ++tlen)
      for (Uint tpos = 0; tpos < tlen; ++tpos)
         for (Uint slen = 1; slen <= max_slen; ++slen)
            fill_n(pos_probs + posOffset(tpos, tlen, slen), slen, 1.0 / slen);

   for (Uint i = 0; i < backoff_size; ++i) {
      float* row = backoff_probs + i * backoff_size;
      fill(row, row+backoff_size, 1.0 / backoff_size);
   }
}

// get a backoff distribution for given conditioning variables
float* IBM2::getBackoffDistn(Uint tpos, Uint tlen, Uint slen)
{
   // fill backoff_distn with values from backoff probs
   if (backoff_distn.size() < slen) backoff_distn.resize(slen);
   Uint trat = backoff_size * tpos / tlen;
   double sum = 0.0;
   for (Uint spos = 0; spos < slen; ++spos) {
      float p = backoff_probs[backoff_size * trat + backoffSrcOffset(spos,slen)];
      backoff_distn[spos] = p;
      sum += p;
   }

   // renormalize
   for (Uint spos = 0; spos < slen; ++spos)
      backoff_distn[spos] /= sum;

   return &backoff_distn[0];
}


IBM2::IBM2(Uint max_slen, Uint max_tlen, Uint backoff_size)
   : max_slen(max_slen)
   , max_tlen(max_tlen)
   , backoff_size(backoff_size)
   , pos_probs(NULL)
   , pos_counts(NULL)
   , backoff_probs(NULL)
   , backoff_counts(NULL)
{
   createProbTables();
   initProbTables();
}


IBM2::IBM2(const string& ttable_file, Uint dummy, Uint max_slen, Uint max_tlen,
           Uint backoff_size)
   : IBM1(ttable_file)
   , max_slen(max_slen)
   , max_tlen(max_tlen)
   , backoff_size(backoff_size)
   , pos_probs(NULL)
   , pos_counts(NULL)
   , backoff_probs(NULL)
   , backoff_counts(NULL)
{
   createProbTables();
   initProbTables();
}

void IBM2::read(const string& pos_file)
{
   iSafeMagicStream ifs(pos_file);

   ifs >> max_slen;
   ifs >> max_tlen;
   ifs >> backoff_size;

   createProbTables();

   if ( ifs.eof() )
      error(ETFatal, "Unexpected end of file in %s", pos_file.c_str());

   for (Uint tlen = 1; tlen <= max_tlen; ++tlen)
      for (Uint tpos = 0; tpos < tlen; ++tpos)
         for (Uint slen = 1; slen <= max_slen; ++slen) {
            double sum = 0.0;
            Uint os = posOffset(tpos, tlen, slen);
            for (Uint j = 0; j < slen; ++j) {
               ifs >> pos_probs[os+j];
               sum += pos_probs[os+j];
            }
            if (abs(sum - 1.0) > .05)
               error(ETWarn, "non-normalized distribution for tpos=%d, tlen=%d, slen=%d",
                     tpos, tlen, slen);
         }

   if ( ifs.eof() )
      error(ETFatal, "Unexpected end of file in %s", pos_file.c_str());

   for (Uint i = 0; i < backoff_size; ++i)
      for (Uint j = 0; j < backoff_size; ++j)
         ifs >> backoff_probs[i * backoff_size + j];
}

IBM2::IBM2(const string& ttable_file)
   : IBM1(ttable_file)
   , pos_probs(NULL)
   , pos_counts(NULL)
   , backoff_probs(NULL)
   , backoff_counts(NULL)
{
   read(posParamFileName(ttable_file));
}

IBM2::IBM2(void* bogus, const string& pos_file)
   : IBM1()
   , pos_probs(NULL)
   , pos_counts(NULL)
   , backoff_probs(NULL)
   , backoff_counts(NULL)
{
   read(pos_file);
}

IBM2::IBM2()
   : pos_probs(NULL)
   , pos_counts(NULL)
   , backoff_probs(NULL)
   , backoff_counts(NULL)
{}

IBM2::IBM2(const IBM2& that)
   : pos_probs(NULL)
   , pos_counts(NULL)
   , backoff_probs(NULL)
   , backoff_counts(NULL)
{}

IBM2::~IBM2() {
   if (pos_probs) delete[] pos_probs;
   if (pos_counts) delete[] pos_counts;
   if (backoff_probs) delete[] backoff_probs;
   if (backoff_counts) delete[] backoff_counts;
}

void IBM2::write(const string& ttable_file, bool bin_ttable) const
{
   IBM1::write(ttable_file, bin_ttable);

   string pos_file = posParamFileName(ttable_file);
   oSafeMagicStream out(pos_file);

   out << max_slen << " " << max_tlen << " " << backoff_size << endl;
   for (Uint tlen = 1; tlen <= max_tlen; ++tlen)
      for (Uint tpos = 0; tpos < tlen; ++tpos)
         for (Uint slen = 1; slen <= max_slen; ++slen) {
            Uint os = posOffset(tpos, tlen, slen);
            for (Uint j = 0; j < slen; ++j)
               out << pos_probs[os+j] << " ";
            out << nf_endl;
         }

   for (Uint i = 0; i < backoff_size; ++i) {
      for (Uint j = 0; j < backoff_size; ++j)
         out << backoff_probs[i * backoff_size + j] << " ";
      out << nf_endl;
   }
}

void IBM2::writeBinCounts(const string& count_file) const
{
   using namespace BinIO;
   IBM1::writeBinCounts(count_file);
   string ibm2_count_file(addExtension(count_file, ".ibm2"));
   oSafeMagicStream os(ibm2_count_file);
   // "Magic number"
   os << "NRC PORTAGE IBM2 count file v1.0" << endl;
   // parameters
   os << max_slen << " " << max_tlen << " " << backoff_size
      << " " << sblock_size << " " << npos_params << ":";
   // pos counts
   assert(pos_counts);
   writebin(os, npos_params);
   writebin(os, pos_counts, npos_params);
   // backoff counts
   assert(backoff_counts);
   Uint backoff_counts_size = backoff_size * backoff_size;
   writebin(os, backoff_counts_size);
   writebin(os, backoff_counts, backoff_counts_size);
   // footer
   os << "End of NRC PORTAGE IBM2 count file v1.0" << endl;
}

void IBM2::readAddBinCounts(const string& count_file)
{
   using namespace BinIO;
   IBM1::readAddBinCounts(count_file);
   string ibm2_count_file(addExtension(count_file, ".ibm2"));
   iSafeMagicStream is(ibm2_count_file);
   string line;

   // "Magic number"
   if ( ! getline(is, line) )
      error(ETFatal, "No input in bin IBM2 count file %s",
            ibm2_count_file.c_str());
   if ( line != "NRC PORTAGE IBM2 count file v1.0" )
      error(ETFatal, "File %s doesn't look like a binary IBM2 count file",
            ibm2_count_file.c_str());

   // parameters
   Uint max_slen_read(0), max_tlen_read(0), backoff_size_read(0),
        sblock_size_read(0), npos_params_read(0);
   char c;
   is >> max_slen_read >> max_tlen_read >> backoff_size_read
      >> sblock_size_read >> npos_params_read >> c;
   if ( max_slen_read != max_slen || max_tlen_read != max_tlen ||
        backoff_size_read != backoff_size || sblock_size_read != sblock_size ||
        npos_params_read != npos_params )
      error(ETFatal, "Parameter mismatch in IBM2 bin count file %s: "
            "expected/got is shown: max_slen %d/%d max_tlen %d/%d "
            "backoff_size %d/%d, sblock_size %d/%d npos_params %d/%d",
            ibm2_count_file.c_str(),
            max_slen, max_slen_read, max_tlen, max_tlen_read,
            backoff_size, backoff_size_read, sblock_size, sblock_size_read,
            npos_params, npos_params_read);
   if ( c != ':' )
      error(ETFatal, "Format error in IBM2 bin count file %s",
            ibm2_count_file.c_str());

   // pos counts
   assert(pos_counts);
   Uint pos_counts_read_size(0);
   readbin(is, pos_counts_read_size);
   if ( pos_counts_read_size != npos_params )
      error(ETFatal, "Bad pos_counts size in IBM2 bin count file %s: got %d, expected %d",
            ibm2_count_file.c_str(), pos_counts_read_size, npos_params);
   float* pos_counts_read = new float[npos_params];
   assert(sizeof(pos_counts_read[0]) == sizeof(pos_counts[0]));
   readbin(is, pos_counts_read, npos_params);
   for ( Uint i(0); i < npos_params; ++i )
      pos_counts[i] += pos_counts_read[i];
   delete [] pos_counts_read;

   // backoff counts
   assert(backoff_counts);
   Uint backoff_counts_size = backoff_size * backoff_size;
   Uint backoff_counts_read_size(0);
   readbin(is, backoff_counts_read_size);
   if ( backoff_counts_read_size != backoff_counts_size )
      error(ETFatal, "Bad backoff_counts size in IBM2 bin count file %s: got %d, expected %d",
            ibm2_count_file.c_str(), backoff_counts_read_size,
            backoff_counts_size);
   float* backoff_counts_read = new float[backoff_counts_size];
   assert(sizeof(backoff_counts_read[0]) == sizeof(backoff_counts[0]));
   readbin(is, backoff_counts_read, backoff_counts_size);
   for ( Uint i(0); i < backoff_counts_size; ++i )
      backoff_counts[i] += backoff_counts_read[i];
   delete [] backoff_counts_read;

   // footer
   if ( ! getline(is, line) )
      error(ETFatal, "No footer in bin IBM2 count file %s",
            ibm2_count_file.c_str());
   if ( line != "End of NRC PORTAGE IBM2 count file v1.0" )
      error(ETWarn, "Bad footer (%s) in bin IBM2 count file %s",
            line.c_str(),
            ibm2_count_file.c_str());
}

void IBM2::initCounts()
{
   if (!pos_counts)
      pos_counts = new float[npos_params];
   fill_n(pos_counts, npos_params, 0.0);

   if (!backoff_counts)
      backoff_counts = new float[backoff_size * backoff_size];
   fill_n(backoff_counts, backoff_size * backoff_size, 0.0);

   IBM1::initCounts();
}

void IBM2::count(const vector<string>& src,
                 const vector<string>& tgt,
                 bool use_null)
{
   const Uint base = (use_null || useImplicitNulls) ? 1 : 0;
   const Uint src_size = src.size() + base;
   vector<int> offsets(src_size);

   for (Uint i = 0; i < tgt.size(); ++i) {

      float* pos_distn = posDistn(i, tgt.size(), src_size);
      float* back_distn = getBackoffDistn(i, tgt.size(), src_size);

      double totpr = 0.0, back_totpr = 0.0;
      Uint tindex = tt.targetIndex(tgt[i]);
      if (tindex == tt.numTargetWords()) continue;

      for (Uint j = 0; j < src_size; ++j) {
         const TTable::SrcDistn& src_distn =
            tt.getSourceDistn(j < base ? nullWord() : src[j-base]);
         offsets[j] = tt.targetOffset(tindex, src_distn);
         if (offsets[j] != -1) {
            totpr += src_distn[offsets[j]].second * pos_distn[j];
            back_totpr += src_distn[offsets[j]].second * back_distn[j];
         }
      }

      const Uint trat = backoff_size * i / tgt.size();
      for (Uint j = 0; j < src_size; ++j) {
         if (offsets[j] == -1) continue;
         const Uint src_index = tt.sourceIndex(j < base ? nullWord() : src[j-base]);
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
         const float c = src_distn[offsets[j]].second * pos_distn[j] / totpr;
         const float back_c = src_distn[offsets[j]].second * back_distn[j] / back_totpr;
         counts[src_index][offsets[j]] += c;
         if (tgt.size() <= max_tlen && src_size <= max_slen)
            pos_counts[posOffset(i, tgt.size(), src_size) + j] += c;
         backoff_counts[backoff_size * trat + backoffSrcOffset(j, src_size)] += back_c;
      }

      if (totpr != 0.0) {
         logprob += log(totpr);
         ++num_toks;
      }
   }
}

void IBM2::count_symmetrized(const vector<string>& src_toks,
                             const vector<string>& tgt_toks,
                             bool use_null, IBM1* r_ibm1)
{
   use_null = use_null || useImplicitNulls;
   IBM2* r = dynamic_cast<IBM2*>(r_ibm1);
   assert(r);

   if ( (! src_toks.empty() && src_toks[0] == nullWord()) ||
        (! tgt_toks.empty() && tgt_toks[0] == nullWord()) )
      error(ETWarn, "Using explicit nulls with IBM2::count_symmetrized() will "
                    "have incorrect effects on the models.");

   // Posteriors for tgt as the observed sequence, src as the hidden one.
   // posteriors[i][j] has p(linking tgt[i] to src[j]).
   // If use_null, posteriors[i][src_toks.size()] has p(null aligning tgt[i]).
   vector<vector<double> > posteriors;
   double log_pr;
   {
      tmp_val<bool> tmp_useImplicitNulls(useImplicitNulls, use_null);
      log_pr = IBM2::linkPosteriors(src_toks, tgt_toks, posteriors);
   }

   // Posteriors for src as the observed sequence, tgt as the hidden one.
   // r_posteriors[j][i] has p_r(linking src[j] to tgt[i]).
   // If use_null, r_posteriors[j][tgt_toks.size()] has p_r(null al. src[j]).
   vector<vector<double> > r_posteriors;
   double r_log_pr;
   {
      tmp_val<bool> tmp_useImplicitNulls(r->useImplicitNulls, use_null);
      r_log_pr = r->IBM2::linkPosteriors(tgt_toks, src_toks, r_posteriors);
   }

   // Update this model's counts using the products of posteriors.
   IBM2::count_sym_helper(src_toks, tgt_toks, use_null,
                          posteriors, r_posteriors);
   logprob += log_pr;

   // Update r's counts using the products of posteriors
   r->IBM2::count_sym_helper(tgt_toks, src_toks, use_null,
                             r_posteriors, posteriors);
   r->logprob += r_log_pr;
}

void IBM2::count_sym_helper(const vector<string>& src_toks,
                            const vector<string>& tgt_toks,
                            bool use_null,
                            const vector<vector<double> >& posteriors,
                            const vector<vector<double> >& r_posteriors)
{
   assert(posteriors.size() == tgt_toks.size());
   assert(posteriors.empty() ||
          posteriors[0].size() == src_toks.size() + (use_null?1:0));
   assert(r_posteriors.size() == src_toks.size());
   assert(r_posteriors.empty() ||
          r_posteriors[0].size() == tgt_toks.size() + (use_null?1:0));

   const Uint base = use_null ? 1 : 0;
   const Uint src_size = src_toks.size() + base;

   for (Uint i = 0; i < tgt_toks.size(); ++i) {

      Uint tindex = tt.targetIndex(tgt_toks[i]);
      if (tindex == tt.numTargetWords()) continue;

      const Uint trat = backoff_size * i / tgt_toks.size();
      for ( Uint j = 0; j < src_toks.size(); ++j ) {
         const Uint src_index = tt.sourceIndex(src_toks[j]);
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
         const int offset = tt.targetOffset(tindex, src_distn);
         const float c = posteriors[i][j] * r_posteriors[j][i];
         if (offset != -1)
            counts[src_index][offset] += c;
         if ( tgt_toks.size() <= max_tlen && src_size <= max_slen )
            pos_counts[posOffset(i, tgt_toks.size(), src_size) + j + base] += c;
         backoff_counts[backoff_size * trat + backoffSrcOffset(j+base, src_size)] += c;
      }
      if ( use_null ) {
         const Uint src_index = tt.sourceIndex(nullWord());
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
         const int offset = tt.targetOffset(tindex, src_distn);
         if (offset != -1 || posteriors[i][src_toks.size()] != 0.0) {
            double r_nullprob = 1;
            for ( Uint j_prime = 0; j_prime < src_toks.size(); ++j_prime )
               r_nullprob *= (1 - r_posteriors[j_prime][i]);
            const float c = posteriors[i][src_toks.size()] * r_nullprob;
            if ( offset != -1 )
               counts[src_index][offset] += c;
            if ( tgt_toks.size() <= max_tlen && src_size <= max_slen )
               pos_counts[posOffset(i, tgt_toks.size(), src_size) + 0] += c;
            backoff_counts[backoff_size * trat + backoffSrcOffset(0, src_size)] += c;
         }
      }

      ++num_toks;
   }
}

pair<double,Uint> IBM2::estimate(double pruning_threshold, 
                                 double null_pruning_threshold)
{
   for (Uint tlen = 1; tlen <= max_tlen; ++tlen)
      for (Uint tpos = 0; tpos < tlen; ++tpos)
         for (Uint slen = 1; slen <= max_slen; ++slen) {
            Uint os = posOffset(tpos, tlen, slen);
            double sum = accumulate(pos_counts+os, pos_counts+os+slen, 0.0);
            for (Uint j = 0; j < slen; ++j)
               pos_probs[os+j] = sum ? pos_counts[os+j] / sum : 1.0 / slen;
         }

   for (Uint i = 0; i < backoff_size; ++i) {
      float* row = backoff_counts + i * backoff_size;
      double sum = accumulate(row, row+backoff_size, 0.0);
      for (Uint j = 0; j < backoff_size; ++j) {
         backoff_probs[backoff_size * i + j] = sum
            ? row[j] / sum
            : 1.0 / backoff_size;
      }
   }

   return IBM1::estimate(pruning_threshold, null_pruning_threshold);
}

double IBM2::pr(const vector<string>& src_toks, const string& tgt_tok,
                Uint tpos, Uint tlen, vector<double>* probs)
{
   Uint base = useImplicitNulls ? 1 : 0;
   if (probs) (*probs).assign(src_toks.size() + base, 0.0);

   Uint tindex = tt.targetIndex(tgt_tok);
   if (tindex == tt.numTargetWords())
      return 0.0;

   float* pos_distn = useImplicitNulls
      ? posDistn(tpos, tlen, src_toks.size()+1)
      : posDistn(tpos, tlen, src_toks.size());

   double p = 0.0;
   for (Uint i = 0; i < src_toks.size(); ++i) {
      const TTable::SrcDistn& distn = tt.getSourceDistn(src_toks[i]);
      int offset = tt.targetOffset(tindex, distn);
      double pos_pr = useImplicitNulls ? pos_distn[i+1] : pos_distn[i];
      if (offset != -1) {
         p += distn[offset].second * pos_pr;
         if (probs) (*probs)[base+i] = distn[offset].second;
      }
   }
   if (useImplicitNulls) {
      const TTable::SrcDistn& distn = tt.getSourceDistn(nullWord());
      int offset = tt.targetOffset(tindex, distn);
      if (offset != -1) {
         p += distn[offset].second * pos_distn[0];
         if (probs) (*probs)[0] = distn[offset].second;
      }
   }

   return p;
}

double IBM2::pr(const vector<string>& src_toks, const string& tgt_tok,
                vector<double>* probs) {
   error(ETFatal, "IBM2::pr(src,tgt,probs) cannot be implemented - "
                  "missing required parameters");
   return 0.0;
}

double IBM2::logpr(const vector<string>& src_toks,
                   const vector<string>& tgt_toks,
                   double smooth)
{
   double lp = 0, logsmooth = log(smooth);
   for (Uint i = 0; i < tgt_toks.size(); ++i) {
      double tp = pr(src_toks, tgt_toks[i], i, tgt_toks.size());
      lp += tp == 0 ? logsmooth : log(tp);
   }
   return lp;
}

double IBM2::minlogpr(const vector<string>& src_toks,
                      const vector<string>& tgt_toks,
                      int i, bool inv, double smooth)
{
   vector<double> probs;
   double lp = 0;
   if (!inv) {
      pr(src_toks, tgt_toks[i], i, tgt_toks.size(), &probs);
      double tp = *max_element(probs.begin(), probs.end());
      lp = tp == 0 ? log(smooth) : log(tp);
   }
   else {
      string src_tok = src_toks[i];
      for ( vector<string>::const_iterator itr=tgt_toks.begin();
            itr!=tgt_toks.end(); itr++ )
         probs.push_back(
            pr(vector<string>(1,src_tok), *itr, i, tgt_toks.size()) );
      double tp = *max_element(probs.begin(), probs.end());
      lp = tp == 0 ? log(smooth) : log(tp);
   }
   return lp;
}


void IBM2::align(const vector<string>& src, const vector<string>& tgt,
                 vector<Uint>& tgt_al, bool twist,
                 vector<double>* tgt_al_probs)
{
   tgt_al.resize(tgt.size());
   if (tgt_al_probs)
      tgt_al_probs->resize(tgt.size());

   for (Uint i = 0; i < tgt.size(); ++i) {

      double max_pr = -1.0;
      tgt_al[i] = src.size();   // this value means unaligned

      float* pos_distn = useImplicitNulls ?
         posDistn(i, tgt.size(), src.size()+1) :
         posDistn(i, tgt.size(), src.size());

      for (Uint j = 0; j < src.size(); ++j) {
         const double pos_pr = useImplicitNulls ? pos_distn[j+1] : pos_distn[j];
         const double pr = pos_pr * tt.getProb(src[j], tgt[i],
               (j==0 && src[j]==nullWord() ? 1e-10 : 1e-10));
         if (pr > max_pr) {
            max_pr = pr;
            tgt_al[i] = j;
            if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
         }
      }
      if ( useImplicitNulls ) {
         const double pr = tt.getProb(nullWord(), tgt[i], 1e-10) * pos_distn[0];
         if ( pr > max_pr ) {
            max_pr = pr;
            tgt_al[i] = src.size();
            if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
         }
      }
   }
}

double IBM2::linkPosteriors(
   const vector<string>& src, const vector<string>& tgt,
   vector<vector<double> >& posteriors)
{
   const Uint I = tgt.size();
   const Uint J = useImplicitNulls ? src.size() : src.size() - 1;
   const Uint pos_distn_base = useImplicitNulls ? 1 : 0;

   // resize and initialize posteriors with 0.0 in each cell.
   posteriors.assign(I, vector<double>(J+1, 0.0));
   assert(posteriors.size() == I);
   if ( I > 0 ) {
      assert(posteriors[0].size() == J + 1);
      assert(posteriors[I-1][J] == 0.0);
   }

   double log_pr = 0;

   for (Uint i = 0; i < I; ++i ) {

      const float* const pos_distn = posDistn(i, tgt.size(), J+1);
      double numerators[J+1];
      double sum(0);

      for (Uint j = 0; j < src.size(); ++j ) {
         const double pos_pr = pos_distn[j+pos_distn_base];
         const double lex_pr = tt.getProb(src[j], tgt[i],
               (j==0 && src[j]==nullWord() ? 1e-10 : 1e-10));
         sum += numerators[j] = lex_pr * pos_pr;
      }
      if ( useImplicitNulls ) {
         const double lex_pr = tt.getProb(nullWord(), tgt[i], 1e-10);
         sum += numerators[src.size()] = lex_pr * pos_distn[0];
      }
      if ( sum > 0.0 )
         log_pr += log(sum / (J+1));
      if ( sum == 0 ) {
         // no data, not even for null alignment! use uniform lexical
         // probabilities, relying only on positional probabilities.
         for ( Uint j = 0; j < src.size(); ++j )
            sum += numerators[j] = pos_distn[j+pos_distn_base];
         if ( useImplicitNulls )
            sum += numerators[src.size()] = pos_distn[0];
      }
      if ( sum == 0 ) {
         // nothing gives, we have to go to plain uniform
         for (Uint j = 0; j <= J; ++j)
            posteriors[i][j] = 1.0/(J+1);
      } else {
         for (Uint j = 0; j <= J; ++j)
            posteriors[i][j] = numerators[j] / sum;
      }
   }

   return log_pr;
}

void IBM2::testReadWriteBinCounts(const string& count_file) const
{
   IBM1::testReadWriteBinCounts(count_file);
   if ( !pos_counts || !backoff_counts )
      return;

   cerr << "Checking IBM2 read/write bin_counts in " << count_file << endl;
   writeBinCounts(count_file);
   // Do a deep copy of the model - slow but OK since this is just for testing.
   IBM2 copy(*this);
   copy.pos_probs = copy.pos_counts = NULL;
   copy.backoff_probs = copy.backoff_counts = NULL;
   copy.initCounts();
   copy.readAddBinCounts(count_file);

   for ( Uint i(0); i < npos_params; ++i ) {
      if ( pos_counts[i] != copy.pos_counts[i] )
         error(ETWarn, "Different pos_counts[%d] orig=%g copy=%g",
               i, pos_counts[i], copy.pos_counts[i]);
   }
   Uint backoff_counts_size = backoff_size * backoff_size;
   for ( Uint i(0); i < backoff_counts_size; ++i ) {
      if ( backoff_counts[i] != copy.backoff_counts[i] )
         error(ETWarn, "Different backoff_counts[%d] orig=%g copy=%g",
               i, backoff_counts[i], copy.backoff_counts[i]);
   }

   copy.readAddBinCounts(count_file);

   for ( Uint i(0); i < npos_params; ++i ) {
      if ( pos_counts[i] + pos_counts[i] != copy.pos_counts[i] )
         error(ETWarn, "Different double pos_counts[%d] orig=%g copy=%g",
               i, pos_counts[i], copy.pos_counts[i]);
   }
   for ( Uint i(0); i < backoff_counts_size; ++i ) {
      if ( backoff_counts[i] + backoff_counts[i] != copy.backoff_counts[i] )
         error(ETWarn, "Different double backoff_counts[%d] orig=%g copy=%g",
               i, backoff_counts[i], copy.backoff_counts[i]);
   }

   cerr << "Done checking IBM2 read/write bin_counts in " << count_file << endl;
}

