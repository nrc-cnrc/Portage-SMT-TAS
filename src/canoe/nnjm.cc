/**
 * @author George Foster
 * @file nnjm.cc  BBN-inspired Neural Net Joint Model Feature
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2014, Her Majesty in Right of Canada
 */

#include <boost/unordered_map.hpp>
#include "str_utils.h"
#include "nnjm.h"
#include "alignment_annotation.h"

// To activate use -DNNJM_MEMORY_FOOTPRINT flag at compile time
#ifdef NNJM_MEMORY_FOOTPRINT
#define NNJM_MEMORY_FOOTPRINT_PRINT(expr) expr
#else
#define NNJM_MEMORY_FOOTPRINT_PRINT(expr)
#endif

using namespace Portage;

const char* NNJM::help = "\
Model options:\n\
(All vocabularies must be in the format output by nnjm-genex.py: 3 special\n\
tokens, followed by word entries in the form 'index word', followed by tag\n\
entries in the form 'index <TAG>tag'.)\n\
\n\
[srcwindow] number of words in source window (must be odd)\n\
[ngorder] number of words in target ngram history (includes current)\n\
[srcvoc] file containing vocabulary for source language\n\
[tgtvoc] file containing vocabulary for target history\n\
[outvoc] file containing vocabulary for target predictions\n\
[srcclasses] file containing source class mapping in the form 'word\tindex'\n\
        (for context-dependent tags, used canoe's -srctags option instead)\n\
[tgtclasses] file containing target class mapping in the form 'word\tindex'\n\
[file] file containing saved python model, accessed via nnjm.py\n\
[dump] dump every call (debugging only!)\n\
[nocache] turn off caching (debugging only!)\n\
[format] model format: nrc (pkl), udem (pkl) or native (txt) [native]\n\
[selfnorm] model is self-normalized\n\
[noSrcSentCache] turn off source sentence caching (debugging only!)\n\
[noLookup] turn off tanh and sigmoid table lookup (debugging only!)\n\
";

// These need to remain synched with nnjm-genex.py:
const char* NNJM::tag_prefix = "<TAG>:";
const char* NNJM::keywords[NUMKEYS] = {"<ELID>", "<UNK>", "<BOS>", "<EOS>"};

// helper for arg processing
static void checkArg(const vector<string>& toks, Uint i, Uint n)
{
   assert(i < toks.size());
   if (i + n >= toks.size())
      error(ETFatal, "not enough arguments for command %s in NNJM model",
            toks[i].c_str());
}

Uint NNJM::readVoc(const string& dir, const string& filename, Voc& voc)
{
   Uint tag_count = 0;
   string line;
   iSafeMagicStream is(adjustRelativePath(dir, filename));

   while (getline(is, line)) {
      vector<string> toks = split<string>(line); // num word
      if (toks.size() != 2)
         error(ETFatal, "expected 'num word' entries in <%s>", 
               filename.c_str());
      if (voc.add(toks[1].c_str()) != conv<Uint>(toks[0]))
         error(ETFatal, 
               "expected entries in <%s> to be consecutively numbered, starting with 0",
               filename.c_str());
      if (isPrefix(tag_prefix, toks[1].c_str()))
         ++tag_count;
   }
   for (Uint i = 0; i < NUMKEYS; ++i)
      if (voc.index(keywords[i]) != i)
         error(ETFatal, "voc file <%s> is missing mandatory keyword %s at index %d",
               filename.c_str(), keywords[i], i);
   return tag_count;
}

NNJM::NNJM(BasicModelGenerator* bmg, const string& arg, bool arg_is_filename) :
   bmg(bmg),
   srcwindow(11),
   ngorder(4),
   have_src_tags(false),
   have_tgt_tags(false),
   dump(false),
   caching(true),
   srcSentCaching(true),
   cache_hits(0),
   cache_misses(0),
   nnjm_wrap(NULL),
   format(native)
{
   time_t start;
   time(&start);

   string str, line;
   const string& spec = arg_is_filename ? gulpFile(arg.c_str(), str) : arg;
   string dir = arg_is_filename ? DirName(arg) : ".";
   std::vector<string> nnjm_files;

   bool useLookup = true;

   cerr << "Loading NNJM model" << (arg_is_filename?" from file " + arg:"") <<  endl;
     
   vector<string> toks = split<string>(spec);
   for (Uint i = 0; i < toks.size(); ++i) {
      if (toks[i] == "[dump]") {
         dump = true;
      } else if (toks[i] == "[noLookup]") {
         useLookup = false;
      } else if (toks[i] == "[nocache]") {
         caching = false;
      } else if (toks[i] == "[noSrcSentCache]") {
         srcSentCaching = false;
      } else if (toks[i] == "[srcwindow]") {
         checkArg(toks, i++, 1);
         srcwindow = conv<Uint>(toks[i]);
         if (srcwindow % 2 == 0)
            error(ETFatal, "NNJM [srcwindow] parameter must be an odd number");
      } else if (toks[i] == "[ngorder]") {
         checkArg(toks, i++, 1);
         ngorder = conv<Uint>(toks[i]);
         if (ngorder == 0)
            error(ETFatal, "NNJM [ngorder] parameter must be > 0");
      } else if (toks[i] == "[srcvoc]") {
         checkArg(toks, i++, 1);
         have_src_tags = readVoc(dir, toks[i], srcvoc);
      } else if (toks[i] == "[tgtvoc]") {
         checkArg(toks, i++, 1);
         have_tgt_tags = readVoc(dir, toks[i], tgtvoc) || have_tgt_tags;
      } else if (toks[i] == "[outvoc]") {
         checkArg(toks, i++, 1);
         have_tgt_tags = readVoc(dir, toks[i], outvoc) || have_tgt_tags;
      } else if (toks[i] == "[srcclasses]") {
         checkArg(toks, i++, 1);
         iSafeMagicStream is(adjustRelativePath(dir, toks[i]));
         while (getline(is, line)) {
            vector<string> sc = split<string>(line); // word\tclass
            if (sc.size() != 2)
               error(ETFatal, "expected 'word\ttag' entries in <%s>", toks[i].c_str());
            srctags[sc[0]] = sc[1];
         }
      } else if (toks[i] == "[tgtclasses]") {
         checkArg(toks, i++, 1);
         iSafeMagicStream is(adjustRelativePath(dir, toks[i]));
         while (getline(is, line)) {
            vector<string> tc = split<string>(line); // word\tclass
            if (tc.size() != 2)
               error(ETFatal, "expected 'word\ttag' entries in <%s>", toks[i].c_str());
            tgttags[tc[0]] = tc[1];
         }
      } else if (toks[i] == "[file]") {
         checkArg(toks, i++, 1);
         vector<string> hashsplit;
         split(toks[i],hashsplit,"#");
         if(hashsplit.size()!=2) {
            error(ETFatal, "expected nnjm file in format filename#index, instead of <%s>",toks[i].c_str());
         }
         if(!isdigit(hashsplit[1][0]))
            error(ETFatal, "expected digit after # in <%s>",toks[i].c_str());
         const Uint iIndex = conv<Uint>(hashsplit[1]);
         while(nnjm_files.size()<=iIndex) nnjm_files.push_back("");
         if(nnjm_files[iIndex]!="")
            error(ETFatal, "error, repeated file at index %d, saw both %s and %s",
                  iIndex, nnjm_files[iIndex].c_str(), hashsplit[0].c_str());
         nnjm_files[iIndex] = hashsplit[0];
      } else if (toks[i] == "[format]") {
         checkArg(toks, i++, 1);
         if (toks[i] == "nrc") format = nrc;
         else if (toks[i] == "udem") format = udem;
         else if (toks[i] == "native") format = native;
         else error(ETFatal, "unknown format in NNJM model: %s", toks[i].c_str());
      } else if (toks[i] == "[selfnorm]") {
         selfnorm = true;
      } else
         error(ETFatal, "unexpected token in NNJM model: %s", toks[i].c_str());
   }
   if (srcvoc.empty()) {
      string kw = join(keywords, keywords+NUMKEYS, "/");
      error(ETWarn, "no NNJM [srcvoc] provided: will map all source words/contexts to %s", 
            kw.c_str());
      for (Uint i = 0; i < NUMKEYS; ++i)
         srcvoc.add(keywords[i]);
   }
   if (tgtvoc.empty()) {
      string kw = join(keywords, keywords+NUMKEYS, "/");
      error(ETWarn, "no NNJM [tgtvoc] provided: will map all target contexts to %s", 
            kw.c_str());
      for (Uint i = 0; i < NUMKEYS; ++i)
         tgtvoc.add(keywords[i]);
   }
   if (outvoc.empty()) {
      error(ETWarn, "no NNJM [outvoc] provided: will map all target words to %s or %s", 
            keywords[UNK], keywords[EOS]);
      for (Uint i = 0; i < NUMKEYS; ++i)
         outvoc.add(keywords[i]);
   }
   if (have_tgt_tags && tgttags.empty()) {
      error(ETFatal, "this NNJM requires a non-empty [tgttags] file");
   }
   tgt_pad.resize(ngorder-1, BOS);
   tgt_pad_fut.resize(ngorder-1, ELID);
   
   if (selfnorm && format!=native) {
      error(ETFatal, "only [format] native models can [selfnorm]");
   }

   // Ensures fall-back behaviour of everyone returning 0
   while(nnjm_wraps.size()<ngorder) {
      nnjm_wraps.push_back(NULL);
   }

   if (nnjm_files.empty() || nnjm_files[0] == "") {
      error(ETWarn, "[file] parameter not supplied for NNJM %s - all scores will be 0",
            arg_is_filename ? arg.c_str() : "<>");
   } else if(nnjm_files.size()>ngorder) {
      error(ETFatal, "expected max #index to be between 0 and %d, got %d",ngorder-1,nnjm_files.size()-1);
   }
   else{
      for(Uint i=0;i<ngorder;i++) {
         string nnjm_file = "";
         if(i<nnjm_files.size()) nnjm_file = nnjm_files[i];
         if(!nnjm_file.empty()) {
            cerr << "Loading NNJM wrapper at index " << i << " from " << nnjm_file << endl;
            nnjm_wraps[i] = getNNJM(dir, nnjm_file, useLookup);
         } else {
            cerr << "No NNJM wrapper at index " << i << "; model will always return 0" << endl;
         }
      }
      nnjm_wrap = nnjm_wraps[0];
   }

   cerr << "Loaded nnjm's model in " << difftime(time(NULL), start) << " seconds." << endl;
}

void NNJM::updateIndexMap(const Voc& ind_voc, vector<Uint>& ind_map)
{
   if (ind_map.size() == bmg->get_voc().size())
      return;                   // want this to happen right away, for speed
   const Voc& voc = bmg->get_voc();
   const Uint b = ind_map.size();
   assert(voc.size() > b);
   ind_map.resize(voc.size(), UNK); // init with default mapping
   for (Uint i = b; i < voc.size(); ++i) {
      Uint id = ind_voc.index(voc.word(i));
      if (id < ind_voc.size())
         ind_map[i] = id;   // word is in ind_voc
      else {
         string w = voc.word(i);
         map<string,string>::iterator p = tgttags.find(w);
         if (p != tgttags.end()) {
            w = tag_prefix + p->second;
            id = ind_voc.index(w.c_str());
            if (id < ind_voc.size())
               ind_map[i] = id;  // word's tag is in ind_voc
         }
      }
   }
}

void NNJM::finalizeInitialization() 
{
   tgtind_map.clear();
   outind_map.clear();
   // reserve extra 1000 for future updates
   tgtind_map.reserve(bmg->get_voc().size() + 1000);
   outind_map.reserve(bmg->get_voc().size() + 1000);
   updateIndexMap(tgtvoc, tgtind_map);
   updateIndexMap(outvoc, outind_map);
}

void NNJM::newSrcSent(const newSrcSentInfo& info)
{
   NNJM_MEMORY_FOOTPRINT_PRINT(cerr << score_cache.getStats() << endl;)
   score_cache.clear();
   vector<string> src_sent_tags;
   if (have_src_tags && !info.src_sent.empty()) {
      if (!srctags.empty()) {
         src_sent_tags.reserve(info.src_sent.size());
         for (Uint i = 0; i < info.src_sent.size(); ++i) {
            map<string,string>::iterator p = srctags.find(info.src_sent[i]);
            src_sent_tags.push_back(p == srctags.end() ? string("<unk>") : p->second);
         }
      } else if (!info.src_sent_tags.empty()) {
         src_sent_tags = info.src_sent_tags;
      } else {
         error(ETFatal, "you need to specify source tags via the canoe -srctags option with this NNJM model");
      }
   }
   if (src_sent_tags.size() != info.src_sent.size())
      error(ETFatal, "number of tags doesn't match number of tokens for input sentence %d",
            info.external_src_sent_id);
   src_pad.assign(srcwindow/2, BOS);
   for (Uint i = 0; i < info.src_sent.size(); ++i) {
      Uint ind = srcvoc.index(info.src_sent[i].c_str());
      if (ind == srcvoc.size()) { // word not in voc
         const string s = tag_prefix + src_sent_tags[i];
         ind = srcvoc.index(s.c_str());
         if (ind == srcvoc.size()) // tag not in voc
            ind = UNK;
      }
      src_pad.push_back(ind);
   }
   src_pad.insert(src_pad.end(), srcwindow/2+1, EOS);

   if (nnjm_wrap && srcSentCaching) {
      nnjm_wrap->newSrcSent(src_pad, srcwindow);
   }

   updateIndexMap(tgtvoc, tgtind_map);
   updateIndexMap(outvoc, outind_map);
}

vector<Uchar>* NNJM::getSposMap(const PhraseInfo& pi)
{
   static Uint unal_phrasepairs = 0, total_phrasepairs = 0; // for alignment check
   Uint src_len = pi.src_words.size();
   Uint tgt_len = pi.phrase.size();
   if (total_phrasepairs <= 100) ++total_phrasepairs;
   if (src_len > max_srcphrase_len)
      error(ETFatal, "maximum permissible source phrase length for NNJMs is %d", 
            max_srcphrase_len);

   // find cached tgt pos -> src pos map for this phrase pair, or make new cache entry

   vector<Uchar>* spos = NULL;  // tgt pos -> src pos
   AlignmentAnnotation* al = AlignmentAnnotation::get(pi.annotations);
   if (al) {
      Uint al_id = al->getAlignmentID();
      Cache::iterator res = align_cache.find(CacheKey(src_len, tgt_len, al_id));
      if (res == align_cache.end()) {
         const vector< vector<Uint> >& sets = *al->getAlignmentSets(al_id, src_len); // src pos -> links
         assert(sets.size() == src_len || sets.size() == src_len+1);
         vector< vector<Uint> > sp_sets(tgt_len); // tgt pos -> links
         for (Uint i = 0; i < src_len; ++i)
            for (Uint j = 0; j < sets[i].size(); ++j) {
               if (sets[i][j] < tgt_len) sp_sets[sets[i][j]].push_back(i);
               else assert(sets[i][j] == tgt_len);
            }
         res = align_cache.insert(make_pair(CacheKey(src_len, tgt_len, al_id), vector<Uchar>())).first;
         spos = &res->second;
         spos->resize(tgt_len);
         for (Uint ii = tgt_len; ii > 0; --ii) { // reverse iter for rtward attachment
            Uint i = ii-1;
            if (sp_sets[i].size()) { // alignments exist for this tgt position
               sort(sp_sets[i].begin(), sp_sets[i].end());
               (*spos)[i] = sp_sets[i][(sp_sets[i].size()-1)/2]; // centre link, round down
            } else
               (*spos)[i] = i+1 < tgt_len ? (*spos)[i+1] : src_len-1;
         }
      }
      spos = &res->second;
      assert(spos->size() == tgt_len);
   } else if (total_phrasepairs <= 100) {
      ++unal_phrasepairs;
      // fatal error rather than warning?
      if (total_phrasepairs == 100 && unal_phrasepairs > 50)
         error(ETWarn, "phrase table appears not to contain alignment info - NNJM may not work properly");
   }
   return spos;
}

double NNJM::score(const PartialTranslation& pt) 
{
   vector<Uchar>* spos = getSposMap(*pt.lastPhrase);  // tgt pos -> src pos
   const Uint tgt_len = pt.lastPhrase->phrase.size();
   tgt_pad.resize(ngorder-1);   // 1st ngorder-1 positions are <BOS>
   pt.getLastWords(tgt_pad, tgt_len + ngorder-1); // append h,w
   // remap indexes for the words before current tgt phrase, if any
   for (Uint i = ngorder-1; i < tgt_pad.size()-tgt_len; ++i)
      tgt_pad[i] = tgtind_map[tgt_pad[i]];

   // call logprob for every position in tgt phrase
   vector<Uint>::iterator tw = tgt_pad.end() - tgt_len;
   vector<Uint>::iterator th = tw - ngorder + 1;
   double s = 0.0;
   for (Uint i = 0; i < tgt_len; ++i) {
      const Uint sp = pt.lastPhrase->src_words.start + 
         (spos ? (*spos)[i] : congruentPos(i, tgt_len, pt.lastPhrase->src_words.size()));
      const Uint w = outind_map[*tw]; // predicted word
      *tw = tgtind_map[*tw];    // set up for use as history on next iter
      if (dump)
         cerr << "NNJM::s(" << pt.lastPhrase->src_words << ", " << bmg->getStringPhrase(pt.lastPhrase->phrase) << ") ";
      s += logprob(sp, src_pad.begin()+sp, src_pad.begin()+sp+srcwindow, th++, tw++, w);
   }
   // handle end-of-sentence
   if(pt.sourceWordsNotCovered.empty() && !bmg->c->nosent) {
      const Uint sp = src_pad.size() - srcwindow;
      if (dump)
         cerr << "NNJM::s(" << pt.lastPhrase->src_words << ", " << "<EOS>" << ") ";
      s += logprob(sp, src_pad.begin()+sp, src_pad.begin()+sp+srcwindow, th++, tw++, EOS);
   }

   return s;
}

double NNJM::precomputeFutureScore(const PhraseInfo& pi)
{
   updateIndexMap(tgtvoc, tgtind_map); // needed?
   updateIndexMap(outvoc, outind_map); // needed?
 
   vector<Uchar>* spos = getSposMap(pi); // tgt pos -> src pos
   const Uint tgt_len = pi.phrase.size();
   tgt_pad_fut.resize(ngorder-1, ELID); // 1st ngorder-1 positions are <ELID>
   tgt_pad_fut.reserve(ngorder-1+tgt_len);
   copy(pi.phrase.begin(), pi.phrase.end(), back_inserter(tgt_pad_fut));

   vector<Uint>::iterator tw = tgt_pad_fut.end() - tgt_len;
   vector<Uint>::iterator th = tw - ngorder + 1;

   double s = 0.0;
   Uint elid = ngorder-1;
   for (Uint i = 0; i < tgt_len; ++i) {
      const Uint sp = pi.src_words.start + 
         (spos ? (*spos)[i] : congruentPos(i, tgt_len, pi.src_words.size()));
      const Uint w = outind_map[*tw]; // predicted word
      *tw = tgtind_map[*tw];    // set up for use as history on next iter
      if (dump)
         cerr << "NNJM::pFS(" << pi.src_words << ", " << bmg->getStringPhrase(pi.phrase) << ") ";
      s += elidLogprob(elid, sp, src_pad.begin()+sp, src_pad.begin()+sp+srcwindow, th++, tw++, w);
      if(elid!=0) elid--;
   }
   return s;
}

void NNJM::dumpContext(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w, double score)
{
   assert(hist_end - hist_beg == ngorder-1);
   assert(src_end - src_beg == srcwindow);
   cerr << "NNJM: ";
   for (VUI p = src_beg; p != src_end; ++p) {
      assert(*p < srcvoc.size());
      cerr << srcvoc.word(*p) << ' ';
   }
   cerr << "/ ";
   for (VUI p = hist_beg; p != hist_end; ++p) {
      assert(*p < tgtvoc.size());
      cerr << tgtvoc.word(*p) << ' ';
   }
   assert(w < outvoc.size());
   cerr << "/ " << outvoc.word(w) << " : " << score;
   if (nnjm_wrap && caching) cerr << " (hits=" << cache_hits << ")";
   if (nnjm_wrap && caching) cerr << " (misses=" << cache_misses << ")";
   cerr << endl;
}
