/**
 * @author George Foster
 * @file ibm.cc  Implementation of GizaAlignmentFile.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <cmath>
#include <numeric>
#include <iostream>
#include <file_utils.h>
#include <errors.h>
#include "ibm.h"

using namespace Portage;

/*--------------------------------------------------------------------------------
GizaAlignmentFile
--------------------------------------------------------------------------------*/

GizaAlignmentFile::GizaAlignmentFile(string& filename) :
  in(filename)
{
  if (!in)
    error(ETFatal, "GizaAlignmentFile: Failed to open alignment file "+filename);
  sent_count = 0;
}

GizaAlignmentFile::~GizaAlignmentFile() {}

void
GizaAlignmentFile::align(const vector<string>& src, const vector<string>& tgt,
                         vector<Uint>& tgt_al, bool twist,
                         vector<double>* tgt_al_probs) {
  sent_count++;

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
    error(ETFatal, "GizaAlignmentFile (sent %d): Unexpectedly reached end of file", sent_count);
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


/*--------------------------------------------------------------------------------
IBM1
--------------------------------------------------------------------------------*/



void IBM1::write(const string& ttable_file) const
{
   oSafeMagicStream out(ttable_file);
   tt.write(out);
}

void IBM1::add(const vector<string>& src_toks, const vector<string>& tgt_toks)
{
   tt.add(src_toks, tgt_toks);
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

void IBM1::count(const vector<string>& src_toks, const vector<string>& tgt_toks)
{
   vector<int> offsets(src_toks.size());

   for (Uint i = 0; i < tgt_toks.size(); ++i) {

      double sum = 0.0;
      Uint tindex = tt.targetIndex(tgt_toks[i]);
      if (tindex == tt.numTargetWords()) continue;

      for (Uint j = 0; j < src_toks.size(); ++j) {
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_toks[j]);
         offsets[j] = tt.targetOffset(tindex, src_distn);
         if (offsets[j] != -1)
            sum += src_distn[offsets[j]].second;
      }

      for (Uint j = 0; j < src_toks.size(); ++j) {
         if (offsets[j] == -1) continue;
         Uint src_index = tt.sourceIndex(src_toks[j]);
         counts[src_index][offsets[j]] +=
            tt.getSourceDistn(src_index)[offsets[j]].second / sum;
      }

      if (sum != 0.0) {
         logprob += log(sum / src_toks.size());
         ++num_toks;
      }
   }
}

pair<double,Uint> IBM1::estimate(double pruning_threshold)
{
   for (Uint i = 0; i < tt.numSourceWords(); ++i) {
      double sum = 0.0;
      for (Uint j = 0; j < counts[i].size(); ++j)
         sum += counts[i][j];
      for (Uint j = 0; j < counts[i].size(); ++j)
         tt.prob(i, j) = sum ? counts[i][j] / sum : 1.0 / sum;
   }
   Uint size = tt.prune(pruning_threshold);

   return make_pair(exp(-logprob / num_toks),size);
}

// is spos1 closer to tpos than spos2?
bool IBM1::closer(Uint spos1, Uint spos2, Uint slen, Uint tpos, Uint tlen, bool twist)
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

double IBM1::logpr(const vector<string>& src_toks, const vector<string>& tgt_toks,
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
  for (vector<string>::const_iterator itr=tgt_toks.begin(); itr!=tgt_toks.end(); itr++)
    probs.push_back( IBM1::pr(vector<string>(1,src_tok), *itr) );
  double tp = *max_element(probs.begin(), probs.end());
  double lp = tp == 0 ? log(smooth) : log(tp);
  return lp;
}

void IBM1::align(const vector<string>& src, const vector<string>& tgt, vector<Uint>& tgt_al, bool twist,
                 vector<double>* tgt_al_probs)
{
   tgt_al.resize(tgt.size());
   if (tgt_al_probs)
      tgt_al_probs->resize(tgt.size());

   for (Uint i = 0; i < tgt.size(); ++i) {

      double max_pr = -1.0;
      tgt_al[i] = src.size();   // this value means unaligned

      for (Uint j = 0; j < src.size(); ++j) {
         double pr = tt.getProb(src[j], tgt[i]);
         if (pr > max_pr ||
             pr == max_pr && pr != -1 && closer(j, tgt_al[i], src.size(), i, tgt.size(),twist)) {
            max_pr = pr;
            tgt_al[i] = j;
            if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
         }
      }
      if (useImplicitNulls && tt.getProb(nullWord(), tgt[i]) > max_pr) {
         tgt_al[i] = src.size();
         if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
      }
   }
}


/*--------------------------------------------------------------------------------
IBM2
--------------------------------------------------------------------------------*/

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


IBM2::IBM2(Uint max_slen, Uint max_tlen, Uint backoff_size) :
   max_slen(max_slen), max_tlen(max_tlen), backoff_size(backoff_size)
{
   pos_counts = backoff_counts = NULL;
   createProbTables();
   initProbTables();
}


IBM2::IBM2(const string& ttable_file, Uint dummy, Uint max_slen, Uint max_tlen, Uint backoff_size) :
   IBM1(ttable_file),
   max_slen(max_slen), max_tlen(max_tlen), backoff_size(backoff_size)
{
   pos_counts = backoff_counts = NULL;
   createProbTables();
   initProbTables();
}

IBM2::IBM2(const string& ttable_file) : IBM1(ttable_file)
{
   pos_counts = backoff_counts = NULL;

   string pos_file = posParamFileName(ttable_file);

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

void IBM2::write(const string& ttable_file) const
{
   IBM1::write(ttable_file);

   string pos_file = posParamFileName(ttable_file);
   oSafeMagicStream out(pos_file);

   out << max_slen << " " << max_tlen << " " << backoff_size << endl;
   for (Uint tlen = 1; tlen <= max_tlen; ++tlen)
      for (Uint tpos = 0; tpos < tlen; ++tpos)
         for (Uint slen = 1; slen <= max_slen; ++slen) {
            Uint os = posOffset(tpos, tlen, slen);
            for (Uint j = 0; j < slen; ++j)
               out << pos_probs[os+j] << " ";
            out << pendl;
         }

   for (Uint i = 0; i < backoff_size; ++i) {
      for (Uint j = 0; j < backoff_size; ++j)
         out << backoff_probs[i * backoff_size + j] << " ";
      out << pendl;
   }
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

void IBM2::count(const vector<string>& src, const vector<string>& tgt)
{
   vector<int> offsets(src.size());

   for (Uint i = 0; i < tgt.size(); ++i) {

      float* pos_distn = posDistn(i, tgt.size(), src.size());
      float* back_distn = getBackoffDistn(i, tgt.size(), src.size());

      double totpr = 0.0, back_totpr = 0.0;
      Uint tindex = tt.targetIndex(tgt[i]);
      if (tindex == tt.numTargetWords()) continue;

      for (Uint j = 0; j < src.size(); ++j) {
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src[j]);
         offsets[j] = tt.targetOffset(tindex, src_distn);
         if (offsets[j] != -1) {
            totpr += src_distn[offsets[j]].second * pos_distn[j];
            back_totpr += src_distn[offsets[j]].second * back_distn[j];
         }
      }

      Uint trat = backoff_size * i / tgt.size();
      for (Uint j = 0; j < src.size(); ++j) {
         if (offsets[j] == -1) continue;
         Uint src_index = tt.sourceIndex(src[j]);
         const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
         float c = src_distn[offsets[j]].second * pos_distn[j] / totpr;
         float back_c = src_distn[offsets[j]].second * back_distn[j] / back_totpr;
         counts[src_index][offsets[j]] += c;
         if (tgt.size() <= max_tlen && src.size() <= max_slen)
            pos_counts[posOffset(i, tgt.size(), src.size()) + j] += c;
         backoff_counts[backoff_size * trat + backoffSrcOffset(j, src.size())] += back_c;
      }

      if (totpr != 0.0) {
         logprob += log(totpr);
         ++num_toks;
      }
   }
}

pair<double,Uint> IBM2::estimate(double pruning_threshold)
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
         backoff_probs[backoff_size * i + j] = sum ? row[j] / sum : 1.0 / backoff_size;
      }
   }

   return IBM1::estimate(pruning_threshold);
}

double IBM2::pr(const vector<string>& src_toks, const string& tgt_tok,
                Uint tpos, Uint tlen, vector<double>* probs)
{
   Uint base = useImplicitNulls ? 1 : 0;
   if (probs) (*probs).assign(src_toks.size() + base, 0.0);

   Uint tindex = tt.targetIndex(tgt_tok);
   if (tindex == tt.numTargetWords())
      return 0.0;

   float* pos_distn = useImplicitNulls ?
      posDistn(tpos, tlen, src_toks.size()+1) : posDistn(tpos, tlen, src_toks.size());

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
   error(ETFatal, "IBM2::pr(src,tgt,probs) cannot be implemented - missing required parameters");
   return 0.0;
}

double IBM2::logpr(const vector<string>& src_toks, const vector<string>& tgt_toks,
                   double smooth)
{
   double lp = 0, logsmooth = log(smooth);
   for (Uint i = 0; i < tgt_toks.size(); ++i) {
      double tp = pr(src_toks, tgt_toks[i], i, tgt_toks.size());
      lp += tp == 0 ? logsmooth : log(tp);
   }
   return lp;
}

double IBM2::minlogpr(const vector<string>& src_toks, const vector<string>& tgt_toks,
                      const int i, bool inv, double smooth)
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
    for (vector<string>::const_iterator itr=tgt_toks.begin(); itr!=tgt_toks.end(); itr++)
      probs.push_back( pr(vector<string>(1,src_tok), *itr, i, tgt_toks.size()) );
    double tp = *max_element(probs.begin(), probs.end());
    lp = tp == 0 ? log(smooth) : log(tp);
  }
  return lp;
}


void IBM2::align(const vector<string>& src, const vector<string>& tgt, vector<Uint>& tgt_al, bool twist,
                 vector<double>* tgt_al_probs)
{
   tgt_al.resize(tgt.size());
   if (tgt_al_probs)
      tgt_al_probs->resize(tgt.size());

   for (Uint i = 0; i < tgt.size(); ++i) {

      double max_pr = -1.0;
      tgt_al[i] = src.size();   // this value means unaligned

      float* pos_distn = useImplicitNulls ?
         posDistn(i, tgt.size(), src.size()+1) : posDistn(i, tgt.size(), src.size());

      for (Uint j = 0; j < src.size(); ++j) {
         double pos_pr = useImplicitNulls ? pos_distn[j+1] : pos_distn[j];
         double pr = tt.getProb(src[j], tgt[i]) * pos_pr;
         if (pr > max_pr) {
            max_pr = pr;
            tgt_al[i] = j;
            if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
         }
      }
      if (useImplicitNulls && tt.getProb(nullWord(), tgt[i]) * pos_distn[0] > max_pr) {
         tgt_al[i] = src.size();
         if (tgt_al_probs) (*tgt_al_probs)[i] = max_pr;
      }
   }
}

