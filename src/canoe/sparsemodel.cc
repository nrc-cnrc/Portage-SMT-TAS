/**
 * @author George Foster
 * @file sparsemodel.cc Represent large feature sets, not all of which are
 * active in a given context.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <set>
#include <limits>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <sstream>
#include "str_utils.h"
#include "file_utils.h"
#include "sparsemodel.h"
#include "alignment_annotation.h"
#include "count_annotation.h"
#include "casemap_strings.h"

using namespace std;
using namespace Portage;

/*-----------------------------------------------------------------------------
  SparseModel
  ---------------------------------------------------------------------------*/
SparseModel::IWordClassesMapping::IWordClassesMapping(Uint unknown)
: num_clust_id(0)
, unknown(unknown)
{ }

SparseModel::IWordClassesMapping::~IWordClassesMapping()
{ }

Uint SparseModel::IWordClassesMapping::numClustIds() const {
   return num_clust_id;
}


SparseModel::WordClassesTextMapper::WordClassesTextMapper(const string& filename, Voc& voc, Uint unknown)
: IWordClassesMapping(unknown)
{
   uint max_clust_id = 0;
   // TODO: WHY? are we creating wl and not using map instead?
   Word2ClassMap* wl = &(this->word2class_map);
   iSafeMagicStream is(filename);
   string line;
   vector<string> toks;
   while (getline(is, line)) {
      if(splitZ(line,toks)!=2)
         error(ETFatal, "mkcls file " + filename + " poorly formatted");
      // TODO: do we really need to add all words from the map to the voc?
      // ANSWER: in load-first mode we are missing the target sentence vocabulary.
      const Uint voc_id = voc.add(toks[0].c_str());
      //const Uint voc_id = voc.index(toks[0].c_str());
      Uint clus_id = 0;
      stringstream ss(toks[1]);
      ss >> clus_id;
      //assert(clus_id!=(Uint)0); // CAC: Opting to fail silently; happens when
                                  // the text includes '1$','2$','3$' or '4$'
                                  // May want to include a warning.
      assert(wl->find(voc_id) == wl->end());
      (*wl)[voc_id] = clus_id;
      if(clus_id > max_clust_id) max_clust_id = clus_id;
   }
   num_clust_id = max_clust_id + 1; // Max is actually one more than the max
}

Uint SparseModel::WordClassesTextMapper::operator()(Uint voc_id) const
{
   Word2ClassMap::const_iterator p = word2class_map.find(voc_id);
   return p == word2class_map.end() ? unknown : p->second;
}


SparseModel::WordClassesMemoryMappedMapper::WordClassesMemoryMappedMapper(const string& filename, const Voc& voc, Uint unknown)
: IWordClassesMapping(unknown)
, word2class_map(filename)
, voc(voc)
{
   // TODO:
   // - load MMMap
   // - find the high class id.
   num_clust_id = 0;
   for (MMMap::const_value_iterator it(word2class_map.vbegin()); it!=word2class_map.vend(); ++it) {
      const Uint classId = conv<Uint>(*it);
      if (classId > num_clust_id)
         num_clust_id = classId;
   }
   num_clust_id += 1;
}

Uint SparseModel::WordClassesMemoryMappedMapper::operator()(Uint voc_id) const
{
   // TODO: what to do if the word is not in the vocabulary?
   if (voc_id >= voc.size()) return unknown;

   MMMap::const_iterator it = word2class_map.find(voc.word(voc_id));
   if (it == word2class_map.end()) return unknown;

   return conv<Uint>(it.getValue());
}


SparseModel::IWordClassesMapping* SparseModel::getWordClassesMapper(const string& filename, Voc& voc, Uint unknown) {
   string magicNumber;
   iSafeMagicStream is(filename);
   if (!getline(is, magicNumber))
      error(ETFatal, "Empty classfile %s", filename.c_str());

   IWordClassesMapping* mapper = NULL;
   if (magicNumber == MMMap::version1)
      error(ETFatal, "SparseModels require MMmap >= 2.0.");
   else if (magicNumber == MMMap::version2)
      mapper = new WordClassesMemoryMappedMapper(filename, voc, unknown);
   else
      mapper = new WordClassesTextMapper(filename, voc, unknown);

   assert(mapper != NULL);
   return mapper;
}


/*-----------------------------------------------------------------------------
  ClusterMap
  ---------------------------------------------------------------------------*/
SparseModel::ClusterMap::ClusterMap(SparseModel& m, const string& filename)
: word2class_map(getWordClassesMapper(adjustRelativePath(m.path, filename), *(m.voc)))
{
   assert(word2class_map != NULL);
}

Uint SparseModel::ClusterMap::phraseId(Uint startLex, Uint endLex, Uint len)
{
   // 0..max                          : [sw]
   // max..max+(max*max)              : [sw fw]
   // max+(max*max)..max+2(max*max)   : [sw .. fw]
   const Uint max = this->numClustIds();
   const Uint start = this->clusterId(startLex);
   if(len==1)
      return start;
   else {
      const Uint end = this->clusterId(endLex);
      Uint multi = 0;
      if(len>2) multi = 1;
      return max + start + (max * end) + (max * max * multi);
   }
}

Uint SparseModel::ClusterMap::clusterId(Uint voc_id)
{
   // This function is called often during decoding.
   assert(word2class_map != NULL);
   return (*word2class_map)(voc_id);
}


Uint SparseModel::ClusterMap::numClustIds()
{
   assert(word2class_map != NULL);
   return word2class_map->numClustIds();
}

Uint SparseModel::ClusterMap::numPhraseIds()
{
   const Uint max = this->numClustIds();
   return max + 2 * (max * max);
}

string SparseModel::ClusterMap::phraseStr(Uint phraseId)
{
   Uint event = phraseId;
   const Uint max = this->numClustIds();
   ostringstream tmpstr;
   tmpstr << "[";
   if(event < max) {
      tmpstr << event;
   }
   else {
      event -= max;
      const Uint multi = event / (max*max);
      event = event % (max*max);
      const Uint end = event / max;
      const Uint start = event % max;
      tmpstr << start;
      tmpstr << (multi==(Uint)1 ? ".." : " ");
      tmpstr << end;
   }
   tmpstr << "]";
   return tmpstr.str();
}

void SparseModel::ClusterMap::replaceVoc(Voc* oldvoc, Voc* newvoc)
{
   assert(false);
   /*
   Word2ClassMap old_map(word2class_map);
   word2class_map.clear();
   for (Uint i = 0; i < oldvoc->size(); ++i)
      word2class_map[newvoc->add(oldvoc->word(i))] = old_map[i];
   */
}

/*-----------------------------------------------------------------------------
  DistortionMaster
  ---------------------------------------------------------------------------*/

// Master singleton to index and track Distortion singletons

class DistortionMaster {
   
private:
   Uint m_sigIndex;
   vector<SparseModel::Distortion*> m_activeDistortions;
   static DistortionMaster* s_instance;
   DistortionMaster() {
      m_sigIndex = 0;
   } // Singleton idiom
public:
   static DistortionMaster* instance();
   Uint add(SparseModel::Distortion* dist);
   Uint maxSig() const;
   const vector<SparseModel::Distortion*>& active() const;
};

DistortionMaster* DistortionMaster::s_instance=0;

DistortionMaster* DistortionMaster::instance() {
   if(!s_instance) s_instance = new DistortionMaster;
   return s_instance;
}

Uint DistortionMaster::add(SparseModel::Distortion* dist)
{
   m_activeDistortions.push_back(dist);
   const Uint toRet = m_sigIndex;
   m_sigIndex += dist->sigSize();
   return toRet;
}

Uint DistortionMaster::maxSig() const {
   return m_sigIndex;
}

const vector<SparseModel::Distortion*>& DistortionMaster::active() const {
   return this->m_activeDistortions;
}


/*-----------------------------------------------------------------------------
  FwdHierDistortion
  ---------------------------------------------------------------------------*/

// Helper singleton to encapsulate distortion decisions
class FwdHierDistortion : public SparseModel::Distortion {
private:
   vector<Uint> m;
   vector<Uint> s;
   vector<Uint> d;
   Uint offset;
   vector<SparseModel::EventTemplate*> m_clients;
   FwdHierDistortion()
   {
      m.push_back((Uint)0); 
      s.push_back((Uint)1); 
      d.push_back((Uint)2);
      offset = DistortionMaster::instance()->add(this);
   } // Singleton idiom
   static FwdHierDistortion* s_instance;
   Uint sig(const Range& shift,
            const ShiftReducer* stack) const;
public :
   static FwdHierDistortion* instance(SparseModel::EventTemplate* t);
   
   virtual Uint numTypes() const;
   virtual const vector<Uint>& type(const Range& shift,
                                    const ShiftReducer* stack) const;
   virtual const vector<Uint>& type(const PartialTranslation& pt) const;
   virtual Uint sigSize() const;
   virtual Uint sig(const PartialTranslation& pt) const;
   virtual string typeString(Uint type) const;
   virtual string name() const {return "FwdHierDistortion";}
   virtual const vector<SparseModel::EventTemplate*>& clients() const {return m_clients;}
};

FwdHierDistortion* FwdHierDistortion::s_instance=0;

FwdHierDistortion* FwdHierDistortion::instance(SparseModel::EventTemplate* t) {
   if(!s_instance) s_instance = new FwdHierDistortion;
   s_instance->m_clients.push_back(t);
   return s_instance;
}

Uint FwdHierDistortion::numTypes() const {
   return 3;
}

Uint FwdHierDistortion::sigSize() const {
   return 3;
}

Uint FwdHierDistortion::sig(const PartialTranslation& pt) const {
   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   return this->sig(pt.lastPhrase->src_words, pt.back->shiftReduce);
}

Uint FwdHierDistortion::sig(const Range& shift,
                            const ShiftReducer* stack) const {
   if(stack==NULL) {
      if(shift.start!=0)
         error(ETFatal, "Expecting [0,*) when stack==NULL");
      return 0+offset; // M
   }

   if(shift.start == stack->end())
      return 0+offset; // M
   else if(shift.end == stack->start())
      return 1+offset; // S
   else
      return 2+offset; // D

}

const vector<Uint>& FwdHierDistortion::type(const Range& shift,
                                            const ShiftReducer* stack) const {
   const Uint u = this->sig(shift,stack) - offset;
   if(u==0) return m;
   else if(u==1) return s;
   else if(u==2) return d;
   else { assert(false); return m; }
}

const vector<Uint>& FwdHierDistortion::type(const PartialTranslation& pt) const {

   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   // EJJ TODO: test if cache for last type query could save CPU time
   // Probably not, because we can at a higher level now.
   return this->type(pt.lastPhrase->src_words, pt.back->shiftReduce);
}

string FwdHierDistortion::typeString(Uint type) const {
   assert(type < this->numTypes());
   if(type==0) return "M";
   else if(type==1) return "S";
   else return "D";
}

/*-----------------------------------------------------------------------------
  FwdLRSplitHierDistortion
  ---------------------------------------------------------------------------*/

// Helper singleton to encapsulate distortion decisions
class FwdLRSplitHierDistortion : public SparseModel::Distortion {
private:
   vector<Uint> m;
   vector<Uint> s;
   vector<Uint> dl;
   vector<Uint> dr;
   Uint offset;
   vector<SparseModel::EventTemplate*> m_clients;
   FwdLRSplitHierDistortion(); // Private constructor for singleton
   Uint sig(const Range& shift,
            const ShiftReducer* stack) const;

   static FwdLRSplitHierDistortion* s_instance;
public :
   static FwdLRSplitHierDistortion* instance(SparseModel::EventTemplate* t);
   virtual Uint numTypes() const;
   virtual const vector<Uint>& type(const Range& shift,
                                    const ShiftReducer* stack) const;
   virtual const vector<Uint>& type(const PartialTranslation& pt) const;
   virtual Uint sigSize() const;
   virtual Uint sig(const PartialTranslation& pt) const;
   virtual string typeString(Uint type) const;
   virtual string name() const {return "FwdLRSplitHierDistortion";}
   virtual const vector<SparseModel::EventTemplate*>& clients() const {return m_clients;}
};

FwdLRSplitHierDistortion::FwdLRSplitHierDistortion()
{
   m.push_back((Uint)0);
   s.push_back((Uint)1);
   dl.push_back((Uint)2); dl.push_back((Uint)3);
   dr.push_back((Uint)2); dr.push_back((Uint)4);
   offset = DistortionMaster::instance()->add(this);
} // Singleton idiom


FwdLRSplitHierDistortion* FwdLRSplitHierDistortion::s_instance=0;

FwdLRSplitHierDistortion* FwdLRSplitHierDistortion::instance(SparseModel::EventTemplate* t) {
   if(!s_instance) s_instance = new FwdLRSplitHierDistortion;
   s_instance->m_clients.push_back(t);
   return s_instance;
}

Uint FwdLRSplitHierDistortion::numTypes() const {
   return 5;
}

Uint FwdLRSplitHierDistortion::sig(const Range& shift,
                                   const ShiftReducer* stack) const {
   if(stack==NULL) {
      if(shift.start!=0)
         error(ETFatal, "Expecting [0,*) when stack==NULL");
      return 0+offset; // M
   }

   if (shift.start == stack->end())
      return 0+offset; //M
   else if (shift.end == stack->start())
      return 1+offset; //S
   else {
      if(shift.end < stack->start())
         return 2+offset; //DL
      else if(shift.start > stack->end())
         return 3+offset; //DR
      else {
         assert(false); // Should be impossible
         return 0;
      }
   }
}

Uint FwdLRSplitHierDistortion::sigSize() const {
   return 4;
}

Uint FwdLRSplitHierDistortion::sig(const PartialTranslation& pt) const {
   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   return this->sig(pt.lastPhrase->src_words, pt.back->shiftReduce);
}

const vector<Uint>& FwdLRSplitHierDistortion::type(const Range& shift,
                                                   const ShiftReducer* stack) const {
   const Uint u = this->sig(shift,stack) - offset;
   if(u==0) return m;
   else if(u==1) return s;
   else if(u==2) return dl;
   else if(u==3) return dr;
   else {
      cerr << "Error: " << u << " unrecognized with offset " << offset << endl;
      assert(false);
      return m;
   }
}

const vector<Uint>& FwdLRSplitHierDistortion::type(const PartialTranslation& pt) const {

   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   return this->type(pt.lastPhrase->src_words, pt.back->shiftReduce);
}

string FwdLRSplitHierDistortion::typeString(Uint type) const {
   assert(type < this->numTypes());
   if(type==0) return "M";
   else if(type==1) return "S";
   else if(type==2) return "D";
   else if(type==3) return "DL";
   else return "DR";
}

/*-----------------------------------------------------------------------------
  SparseModel
  ---------------------------------------------------------------------------*/

map<Uint,Uint>* SparseModel::readSubVoc(const string& filename)
{
   map<Uint,Uint>* const wl = new map<Uint,Uint>();
   const string fullname =  adjustRelativePath(path, filename);
   iSafeMagicStream is(fullname);
   string line;
   Uint id = 0;
   while (getline(is, line)) {
      const Uint voc_id = voc->add(line.c_str());
      assert(wl->find(voc_id) == wl->end());
      (*wl)[voc_id] = id++;
   }
   return wl;
}

Uint SparseModel::vocId(map<Uint,Uint>* subvoc, Uint subvoc_id)
{
   assert(subvoc_id < subvoc->size());
   // need to do a linear search through subvoc list for the entry that has
   // subvoc_id as its value (ugh)
   for (map<Uint,Uint>::iterator p = subvoc->begin(); p != subvoc->end(); ++p)
      if (p->second == subvoc_id) {
         assert(p->first < voc->size());
         return p->first;
      }
   assert(false); // should have found subvoc_id somewhere!
   return 0;
}

void SparseModel::createEventTemplates(const string& spec)
{
   // parse string spec

   vector<string> toks;
   split(spec, toks); // may need something better, to preserve whitespace
   vector<string> cmds, args;
   for (Uint i = 0; i < toks.size(); ++i) {
      const string& tok = toks[i];
      if (!tok.empty() && tok[0] == '[' && tok[tok.size()-1] == ']') {
         cmds.push_back(tok.substr(1, tok.size()-2));
         args.push_back("");
      } else {
         if (args.size() == 0)
            error(ETFatal, 
                  "arguments before 1st feature name in feature template spec");
         args.back() += tok + " ";
      }
   }
   // create models

   for (Uint i = 0; i < cmds.size(); ++i) {
      string arg = args[i];
      trim(arg);

      if (cmds[i] == "TgtContextWordAtPos")
         event_templates.push_back(new TgtContextWordAtPos(*this, arg));
      else if (cmds[i] == "CurTgtPhraseWordAtPos")
         event_templates.push_back(new CurTgtPhraseWordAtPos(*this, arg));
      else if (cmds[i] == "CurSrcPhraseWordAtPos")
         event_templates.push_back(new CurSrcPhraseWordAtPos(*this, arg));
      else if (cmds[i] == "PrevTgtPhraseWordAtPos")
         event_templates.push_back(new PrevTgtPhraseWordAtPos(*this, arg));
      else if (cmds[i] == "PrevSrcPhraseWordAtPos")
         event_templates.push_back(new PrevSrcPhraseWordAtPos(*this, arg));
      else if (cmds[i] == "CurTgtPhraseContainsWord")
         event_templates.push_back(new CurTgtPhraseContainsWord(*this, arg));
      else if (cmds[i] == "CurSrcPhraseContainsWord")
         event_templates.push_back(new CurSrcPhraseContainsWord(*this, arg));
      else if (cmds[i] == "PrevTgtPhraseContainsWord")
         event_templates.push_back(new PrevTgtPhraseContainsWord(*this, arg));
      else if (cmds[i] == "PrevSrcPhraseContainsWord")
         event_templates.push_back(new PrevSrcPhraseContainsWord(*this, arg));
      else if (cmds[i] == "TgtNgramCountBin")
         event_templates.push_back(new TgtNgramCountBin(*this, arg));
      else if (cmds[i] == "PhrasePairLengthBin")
         event_templates.push_back(new PhrasePairLengthBin(*this, arg));
      else if (cmds[i] == "PhrasePairAllBiLengthBins")
         event_templates.push_back(new PhrasePairAllBiLengthBins(*this, arg));
      else if (cmds[i] == "PhrasePairAllTgtLengthBins")
         event_templates.push_back(new PhrasePairAllTgtLengthBins(*this, arg));
      else if (cmds[i] == "PhrasePairAllSrcLengthBins")
         event_templates.push_back(new PhrasePairAllSrcLengthBins(*this, arg));
      else if (cmds[i] == "PhrasePairTotalLengthBin")
         event_templates.push_back(new PhrasePairTotalLengthBin(*this, arg));
      else if (cmds[i] == "LexUnalTgtAny")
         event_templates.push_back(new LexUnalTgtAny(*this, arg));
      else if (cmds[i] == "LexUnalSrcAny")
         event_templates.push_back(new LexUnalSrcAny(*this, arg));
      else if (cmds[i] == "AlignedWordPair")
         event_templates.push_back(new AlignedWordPair(*this, arg));
      else if (cmds[i] == "PhrasePairCountBin")
         event_templates.push_back(new PhrasePairCountBin(*this, arg));
      else if (cmds[i] == "PhrasePairCountMultiBin")
         event_templates.push_back(new PhrasePairCountMultiBin(*this, arg));
      else if (cmds[i] == "PhrasePairCountsMultiBin")
         event_templates.push_back(new PhrasePairCountsMultiBin(*this, arg));
      else if (cmds[i] == "RarestTgtWordCountBin")
         event_templates.push_back(new RarestTgtWordCountBin(*this, arg));
      else if (cmds[i] == "AvgTgtWordScoreBin")
         event_templates.push_back(new AvgTgtWordScoreBin(*this, arg));
      else if (cmds[i] == "MinTgtWordScoreBin")
         event_templates.push_back(new MinTgtWordScoreBin(*this, arg));
      else if (cmds[i] == "DistCurrentSrcPhrase")
         event_templates.push_back(new DistCurrentSrcPhrase(*this, arg));
      else if (cmds[i] == "DistCurrentSrcFirstWord")
         event_templates.push_back(new DistCurrentSrcFirstWord(*this, arg));
      else if (cmds[i] == "DistCurrentSrcLastWord")
         event_templates.push_back(new DistCurrentSrcLastWord(*this, arg));
      else if (cmds[i] == "DistCurrentTgtFirstWord")
         event_templates.push_back(new DistCurrentTgtFirstWord(*this, arg));
      else if (cmds[i] == "DistCurrentTgtLastWord")
         event_templates.push_back(new DistCurrentTgtLastWord(*this, arg));
      else if (cmds[i] == "DistReducedSrcFirstWord")
         event_templates.push_back(new DistReducedSrcFirstWord(*this, arg));
      else if (cmds[i] == "DistReducedSrcLastWord")
         event_templates.push_back(new DistReducedSrcLastWord(*this, arg));
      else if (cmds[i] == "DistReducedTgtLastWord")
         event_templates.push_back(new DistReducedTgtLastWord(*this, arg));
      else if (cmds[i] == "DistGapSrcWord")
         event_templates.push_back(new DistGapSrcWord(*this, arg));
      else if (cmds[i] == "DistTopSrcFirstWord")
         event_templates.push_back(new DistTopSrcFirstWord(*this, arg));
      else if (cmds[i] == "DistTopSrcLastWord")
         event_templates.push_back(new DistTopSrcLastWord(*this, arg));
      else if (cmds[i] == "DistPrevTgtWord")
         event_templates.push_back(new DistPrevTgtWord(*this, arg));
      else if (cmds[i] == "LmUnigram")
         event_templates.push_back(new LmUnigram(*this, arg));
      else if (cmds[i] == "LmBigram")
         event_templates.push_back(new LmBigram(*this, arg));
      else if (cmds[i] == "InQuotePos")
         event_templates.push_back(new InQuotePos(*this, arg));
      else if (cmds[i] == "PhrasePairContextMatch")
         event_templates.push_back(new PhrasePairContextMatch(*this, arg));
      else if (cmds[i] == "PhrasePairSubCorpRank")
         event_templates.push_back(new PhrasePairSubCorpRank(*this, arg));
      else
         error(ETFatal, "unknown EventTemplate class: " + cmds[i]);
      if (verbose > 1)
         cerr << "created template " << cmds[i] << " with arg: '" << arg << "'" << endl;

   }
   // create event_template_implies matrix

   const Uint n = event_templates.size();
   event_template_implies.resize(n);
   for (Uint i = 0; i < n; ++i) {
      event_template_implies[i].resize(n);
      for (Uint j = 0; j < n; ++j)
         if (i == j || event_templates[i]->implies(*event_templates[j]))
            event_template_implies[i][j] = true;
   }
}

string SparseModel::describeAllEventTemplates()
{
   static char ret[] = "\n\
Event template inventory:\n\
\n\
(Templates marked with 'A' require a= alignment information in phrasetable, those\n\
marked with 'C' require c= count information, and those marked with X require\n\
custom information in the c= field.\n\
\n\
[TgtContextWordAtPos] -n        -- nth word before current target phrase\n\
[CurTgtPhraseWordAtPos] n       -- nth word in cur tgt phrase (from end if n < 0)\n\
[CurSrcPhraseWordAtPos] n       -- nth word in cur src phrase (from end if n < 0)\n\
[PrevTgtPhraseWordAtPos] n      -- nth word in prev tgt phrase (from end if n < 0)\n\
[PrevSrcPhraseWordAtPos] n      -- nth word in prev src phrase (from end if n < 0)\n\
[CurTgtPhraseContainsWord]      -- cur tgt phrase contains word\n\
[CurSrcPhraseContainsWord]      -- cur src phrase contains word\n\
[PrevTgtPhraseContainsWord]     -- prev tgt phrase contains word\n\
[PrevSrcPhraseContainsWord]     -- prev src phrase contains word\n\
[TgtNgramCountBin] cnts m l h n -- m-gram ending at pos n in cur tgt phrase\n\
                                   has count in [l,h] in SRILM counts file cnts.\n\
                                   Valid pos values: -1 for end, -2 for prev, etc.\n\
[PhrasePairLengthBin] a b c d   -- cur phrase has srclen in [a,b], tgtlen in [c,d]\n\
[PhrasePairAllBiLengthBins] a b -- all length bins, max srclen a, max tgtlen b\n\
[PhrasePairAllTgtLengthBins] a  -- all tgt length bins, max tgtlen a\n\
[PhrasePairAllSrcLengthBins] a  -- all src length bins, max srclen a\n\
[PhrasePairTotalLengthBin] len  -- cur phrase has srclen+tgtlen = len\n\
[LexUnalTgtAny] f               -A tgt word in file f is unaligned in cur phrase\n\
[LexUnalSrcAny] f               -A src word in file f is unaligned in cur phrase\n\
[AlignedWordPair] svoc tvoc     -A src word in file svoc is aligned 1-1 with tgt\n\
                                   word in tvoc or other word, or is not 1-1\n\
[PhrasePairCountBin] min max    -C current phrase pair has joint count in [min,max]\n\
[PhrasePairCountMultiBin] bins  -C current phrase pair has joint count in some bin\n\
                                   in file bins ([low high] count pairs) - one \n\
                                   feature per bin\n\
[PhrasePairCountsMultiBin] args -C where args is 'bins b e': same as previous, except\n\
                                   applied to each count in 1-based count range [b,e]\n\
                                   in 'c=' field; num-bins * (e-b+1) features total\n\
[RarestTgtWordCountBin] c b     -- rarest tgt word, according to file c (word count\n\
                                   pairs), has count in some bin in file b (low high\n\
                                   pairs)\n\
[AvgTgtWordScoreBin] scores n m -- avg tgt word score, according to file scores (word\n\
                                   score pairs), with real-valued scores binned into\n\
                                   n equal ranges, linearly if m is 'lin', logwise if\n\
                                   m is 'log'\n\
[MinTgtWordScoreBin] scores n m -- same as AvgTgtScoreBin, but use minimum score\n\
[DistCurrentSrcPhrase] cls      -- represents source phrase using clusters provided in\n\
                                   file 'cls' and then tracks HierLDM orientation (MSD)\n\
[DistCurrentSrcFirstWord] cls   -- first source word cluster in current phrase + MSD\n\
[DistCurrentSrcLastWord] cls    -- last source word cluster in current phrase + MSD\n\
[DistCurrentTgtFirstWord] cls   -- first target word cluster in current phrase + MSD\n\
[DistCurrentTgtLastWord] cls    -- last target word cluster in current phrase + MSD\n\
[DistReducedSrcFirstWord] cls   -- first source word cluster in any reduced phrase + MSD\n\
[DistReducedSrcLastWord] cls    -- last source word cluster in any reduced phrase + MSD\n\
[DistReducedTgtLastWord] cls    -- last target word cluster in any reduced phrase + MSD\n\
[DistGapSrcWord] cls            -- one cluster event for each source word between current\n\
                                   phrase and the top of the shift-reduce stack\n\
[DistTopSrcFirstWord] cls       -- first source word cluster of the top of the stack + MSD\n\
[DistTopSrcLastWord] cls        -- last source word cluster of the top of the stack + MSD\n\
[DistPrevTgtWord] cls           -- last target word cluster of previous phrase + MSD\n\
[LmUnigram] cls                 -- unigram language model over provided clusters\n\
[LmBigram] cls                  -- bigram language model over provided clusters\n\
[InQuotePos]                    -- position in quoted text - truecasing only\n\
[PhrasePairContextMatch] args   -X match between current phrase pair and sent-level context\n\
                                   args: tagfile idcol field pweight numbins\n\
                                   see constructor in palminer.h for further doc\n\
[PhrasePairSubCorpRank] n c b-e -X out of n sub-corpus probs in c'th column, those\n\
                                   that have ranks in range [b-e] for current phrase pair\n\
";
   return ret;
}

string SparseModel::describeModelStructure() const
{
   ostringstream oss;
   oss << "SparseModel learning parameters: " << endl; 
   oss <<  "   " << (learn_conjunctions ? "" : "not ") 
       << "learning conjunctions" << endl;
   oss <<  "   max features for conjoin = " << max_features_for_conjoin << endl;
   oss <<  "   min freq for conjoin = " << min_freq_for_conjoin << endl;
   oss <<  "   " << (prune0 ? "" : "not ") << "pruning 0-wt features" << endl;

   oss << "SparseModel active templates:" << endl;
   for (Uint i = 0; i < event_templates.size(); ++i)
      oss << event_templates[i]->description() << endl;
   return oss.str();
}

string SparseModel::describeActiveEventTemplateImplications() const
{
   string ret("Template implications:\n");
   for (Uint i = 0; i < event_templates.size(); ++i) {
      ret += event_templates[i]->description2() + " ->";
      for (Uint j = 0; j < event_templates.size(); ++j)
         if (event_template_implies[i][j]) 
            ret += " " + event_templates[j]->description2();
      ret += "\n";
   }
   return ret;
}

void SparseModel::dump(const PartialTranslation& context, const PhraseInfo* alt_lastphrase)
{
   cerr << "reference hypothesis:" << endl;

   // compile indicators for source phrase membership, and reversible list of
   // target phrases  
   vector<const PhraseInfo*> phrases;
   vector<Uint> src_align(srcsent.size(), 0);
   Uint ti = 1;
   for (const PartialTranslation* pt = &context; pt->back; pt = pt->back, ++ti) {
      for (Uint i = pt->lastPhrase->src_words.start; i < pt->lastPhrase->src_words.end; ++i)
         src_align[i] = ti;
      phrases.push_back(pt->lastPhrase);
   }

   // write source phrases
   for (Uint i = 0; i < srcsent.size(); ++i) {
      if (src_align[i] && (i == 0 || src_align[i-1] != src_align[i])) {
         if (i > 0 && src_align[i-1] == 0) cerr << ' ';
         cerr << "[" << ti-src_align[i] << ">";
      }
      cerr << ' ' << voc->word(srcsent[i]);
      if (src_align[i] && (i+1 == context.lastPhrase->src_words.end || 
                           src_align[i] != src_align[i+1]))
         cerr << "]";
   }
   cerr << endl;

   // write target phrases
   Uint j = 0;
   for (Uint i = phrases.size(); i > 0; --i)
      cerr << "[" << ++j << "> " << phrase2string(phrases[i-1]->phrase, *voc) << "]";
   cerr << endl;

   // write alt lastphrase
   if (alt_lastphrase) {
      cerr << "model choice for current phrase pair (source position=" 
           << alt_lastphrase->src_words.start+1 << "):" << endl << "[";
      for (Uint i = alt_lastphrase->src_words.start; i < alt_lastphrase->src_words.end; ++i)
         cerr << voc->word(srcsent[i]) << (i+1 == alt_lastphrase->src_words.end ? "" : " ");
      cerr << "] [" << phrase2string(alt_lastphrase->phrase, *voc) << "]" << endl;
   }
}

void SparseModel::dump(Uint template_id, vector<Uint>& event_ids)
{
   if (event_ids.size()) {
      cerr << event_templates[template_id]->description() << " -> ";
      for (Uint i = 0; i < event_ids.size(); ++i)
         cerr << event_templates[template_id]->eventDesc(event_ids[i]) << " ";
      cerr << endl;
   }
}

void SparseModel::check()
{
   // check event->feature mapping

   for (Uint i = 0; i < features.size(); ++i) {
      for (Uint j = 0; j < features[i].events.size(); ++j) {
         EventMap::iterator p = events.find(features[i].events[j]);
         if (p == events.end())
            error(ETFatal, "no events entry found for event %d,%d (%s) from feature %d (%s)",
                  features[i].events[j].tid, features[i].events[j].eid,
                  features[i].events[j].toString(*this).c_str(),
                  i, features[i].toString(*this).c_str());
         if (find(p->second.begin(), p->second.end(), i) == p->second.end())
            error(ETFatal, "event %d,%d (%s) doesn't point to feature %d (%s) as it should",
                  features[i].events[j].tid, features[i].events[j].eid,
                  features[i].events[j].toString(*this).c_str(),
                  i, features[i].toString(*this).c_str());
      }
   }
   cerr << "check: event->feature map ok" << endl;

   // check feature->event mapping

   for (EventMap::iterator p = events.begin(); p != events.end(); ++p) {
      for (Uint i = 0; i < p->second.size(); ++i) {
         if (p->second[i] > features.size())
            error(ETFatal, "event %d,%d (%s) contains non-existent feature index %d",
                  p->first.tid, p->first.eid, p->first.toString(*this).c_str(),
                  p->second[i]);
         vector<Event>& ev = features[p->second[i]].events;
         if (find(ev.begin(), ev.end(), p->first) == ev.end())
            error(ETFatal, "feature %d (%s) doesn't contain event %d,%d (%s) as it should",
                  p->second[i], features[p->second[i]].toString(*this).c_str(),
                  p->first.tid, p->first.eid, p->first.toString(*this).c_str());
      }
   }
   cerr << "check: feature->event map ok" << endl;
}

void SparseModel::compilePartialFeatures()
{
   // build a set of partial features (having conjunctions containing no events
   // that depend only on the current phrase pair)

   typedef set<Feature,LexComp> FSet;
   FSet fset(LexComp(*this));
   for (Uint i = 0; i < features.size(); ++i) {
      Feature f;
      for (Uint j = 0; j < features[i].events.size(); ++j) 
         if (!event_templates[features[i].events[j].tid]->isPure())
            f.events.push_back(features[i].events[j]);
      fset.insert(f);
   }
   // add elements in set to partial_features, and member events to
   // partial_events 

   partial_features.resize(fset.size());
   partial_events.clear();
   Uint i = 0;
   vector<Uint> v(1);
   for (FSet::iterator p = fset.begin(); p != fset.end(); ++p, ++i) {
      v[0] = i;
      partial_features[i] = *p;
      for (Uint j = 0; j < p->events.size(); ++j) {
         pair<EventMap::iterator, bool> r = 
            partial_events.insert(make_pair(p->events[j], v)); 
         if (!r.second) r.first->second.push_back(i);
      }
   }
}

bool SparseModel::conjoin(const vector<Event>& evs, Event event)
{
   // Check that event isn't implied by any events already in the feature, or vv.

   EventMap::iterator pe = events.find(event);
   vector<Uint>* min_feature_list = pe != events.end() ? &pe->second : NULL;
   for (vector<Event>::const_iterator p = evs.begin(); p < evs.end(); ++p) {
      if (p->eid == event.eid &&
          (event_template_implies[p->tid][event.tid] ||
          event_template_implies[event.tid][p->tid]))
         return false;
      EventMap::iterator pe = events.find(*p);
      assert(pe != events.end());
      if (min_feature_list && pe->second.size() < min_feature_list->size())
         min_feature_list = &pe->second;
   }
   // Check to make sure proposed conjunction isn't already a feature: compare
   // with each feature in the list of features for the event with the smallest
   // number of features.  (We only need to do this if <event> belongs to at
   // least one feature already.)

   Feature newf(evs);
   newf.add(event);
   if (min_feature_list) 
      for (Uint i = 0; i < min_feature_list->size(); ++i)
         if (features[(*min_feature_list)[i]] == newf)
            return false;

   // Add new feature to the model

   vector<Uint> v(1, features.size());
   for (vector<Event>::iterator p = newf.events.begin(); 
        p < newf.events.end(); ++p) {
      pair<EventMap::iterator, bool> res = events.insert(make_pair(*p, v));
      if (!res.second) res.first->second.push_back(features.size());
   }
   features.push_back(newf);
   feature_freqs.push_back(0);
   feature_lastex.push_back(0);
   feature_wtsums.push_back(0);

   return true;
}

double SparseModel::tallyFeatures(const PartialTranslation& context,
      vector<Uint>& event_template_ids, EventMap& event_map)
{
   if (event_template_ids.empty()) return 0;

   vector<Event> active_events; // list of events in context
   active_events.reserve(20);
   map<Uint,Uint> potential_features; // feature id -> event count
   vector<Uint> eids;
   for (Uint i = 0; i < event_template_ids.size(); ++i) {
      const Uint event_template_id = event_template_ids[i];
      eids.clear();
      event_templates[event_template_id]->addEvents(context, eids);
      if (verbose > 2) dump(event_template_id, eids);
      for (Uint j = 0; j < eids.size(); ++j) {
         const Event e(event_template_id, eids[j]);
         active_events.push_back(e);
         EventMap::iterator p = event_map.find(e);
         if (p != event_map.end()) {
            for (vector<Uint>::iterator f = p->second.begin();
                 f < p->second.end(); ++f) {
               pair<map<Uint,Uint>::iterator, bool> r =
                  potential_features.insert(make_pair(*f, 0));
               ++r.first->second;
            }
         }
      }
   }
   if (verbose > 2) 
      cerr << active_events.size() << " active events; " 
           << potential_features.size() << " potential features; ";

   Uint feature_count = 0;
   double r = 0;
   for (map<Uint,Uint>::iterator p = potential_features.begin(); 
        p != potential_features.end(); ++p) {
      if (p->second == features[p->first].events.size()) {
         ++feature_count;
         r += features[p->first].weight;
      }
   }
   if (verbose > 2)
      cerr << feature_count << " active features; "
           << r << " score; ";

   return r;
}

void SparseModel::getFeatures(const PartialTranslation& context, 
                              set<Uint>& fset, bool new_atoms, bool new_conj)
{
   vector<Event> active_events; // list of events in context
   // Profiling results: here map is faster than unordered_map.
   map<Uint,Uint> potential_features; // feature id -> event count

   // Make list of active events in context, and build initial list of
   // potentially active features, including new atomic features from new
   // events if new_atoms is true.
   
   vector<Uint> eids;
   for (Uint i = 0; i < event_templates.size(); ++i) {
      eids.clear();
      event_templates[i]->addEvents(context, eids);
      if (verbose > 2) dump(i, eids);
      for (Uint j = 0; j < eids.size(); ++j) {
         const Event e(i, eids[j]);
         active_events.push_back(e);
         EventMap::iterator p = events.find(e);
         if (new_atoms && p == events.end() && event_templates[i]->isDirect()) {
            p = events.insert(make_pair(e, vector<Uint>(1, features.size()))).first;
            features.push_back(Feature(e));   // new atomic feature
            feature_freqs.push_back(0);
            feature_lastex.push_back(0);
            feature_wtsums.push_back(0);
         }
         if (p != events.end()) // record potential features for this event
            for (vector<Uint>::iterator f = p->second.begin(); 
                 f < p->second.end(); ++f) {
               pair < map<Uint,Uint>::iterator, bool> r = 
                  potential_features.insert(make_pair(*f, 0));
               ++r.first->second;
            }
      }
   }
   if (verbose > 2) 
      cerr << active_events.size() << " active events; " 
           << potential_features.size() << " potential features; ";

   // Determine which potential features are active by checking whether they
   // have the right number of triggering events.

   fset.clear();
   for (map<Uint,Uint>::iterator p = potential_features.begin(); 
        p != potential_features.end(); ++p) {
      //assert(p->second <= features[p->first].events.size()); // Expensive assertion
      if (p->second == features[p->first].events.size())
         assert(fset.insert(p->first).second);
   }
   if (verbose > 2) cerr << fset.size() << " active features; ";

   // Create new conjunctive features if permissible.

   const Uint nf = features.size();
   if (new_conj && fset.size() < max_features_for_conjoin)
      for (set<Uint>::iterator p = fset.begin(); p != fset.end(); ++p)
         if (feature_freqs[*p] >= min_freq_for_conjoin)
            for (Uint i = 0; i < active_events.size(); ++i)
               conjoin(features[*p].events, active_events[i]);
   if (verbose > 2) 
      cerr << features.size()-nf << " new conjoined features; " << endl;
   for (Uint i = nf; i < features.size(); ++i)
      fset.insert(i);

   // Dump final feature vector.
   
   if (verbose > 2) {
      for (set<Uint>::iterator p = fset.begin(); p != fset.end(); ++p)
         dumpFeature(*p);
   }
}

void SparseModel::getPartialFeatures(const PartialTranslation& context, 
                                     set<Uint>& fset)
{   
   // Profiling results: here map is faster than unordered_map.
   map<Uint,Uint> potential_features; // feature id -> event count
   
   // Make list of active events in context, and build initial list of
   // potentially active features.
   
   for (Uint i = 0; i < event_templates.size(); ++i) {
      vector<Uint> eids;
      event_templates[i]->addContextEvents(context, eids);
      for (Uint j = 0; j < eids.size(); ++j) {
         const Event e(i, eids[j]);
         EventMap::iterator p = partial_events.find(e);
         if (p != partial_events.end())
            for (Uint j = 0; j < p->second.size(); ++j) {
               pair<map<Uint,Uint>::iterator, bool> r = 
                  potential_features.insert(make_pair(p->second[j],0));
               ++r.first->second;
            }
      }
   }
   // Add to fset all elements from potential_features whose event counts are
   // 'full' 

   fset.clear();
   for (map<Uint,Uint>::iterator p = potential_features.begin(); 
        p != potential_features.end(); ++p)
      if (partial_features[p->first].events.size() == p->second)
         fset.insert(p->first);
}

void SparseModel::populate()
{
   for (Uint i = 0; i < event_templates.size(); ++i) {
      vector<Uint> eids;
      event_templates[i]->populate(eids);
      for (Uint j = 0; j < eids.size(); ++j) {
         const Event e(i, eids[j]);
         EventMap::iterator p = events.find(e);
         if (p == events.end() && event_templates[i]->isDirect()) {
            p = events.insert(make_pair(e, vector<Uint>(1, features.size()))).first;
            features.push_back(Feature(e));   // new atomic feature
            feature_freqs.push_back(0);
            feature_lastex.push_back(0);
            feature_wtsums.push_back(0);
            if (verbose > 2) cerr << event_templates[i]->eventDesc(j) << endl;
         }
      }
   }
}

void SparseModel::learnInit()
{
   exno = 0;
   const Uint nf = features.size();
   feature_freqs.assign(nf, 0);
   feature_lastex.assign(nf, 0);
   feature_wtsums.assign(nf, 0.0);
}

Uint SparseModel::learn(const PartialTranslation* ref, const PartialTranslation* mbest)
{
   ++exno;
   const Uint nf = features.size();
   
   if (verbose > 1) dump(*ref, mbest ? mbest->lastPhrase : NULL);

   if (verbose > 1) cerr << "reference features:" << endl;
   set<Uint> fset_ref;
   getFeatures(*ref, fset_ref, true, learn_conjunctions);

   if (verbose > 1) cerr << "model best features: " << endl;
   set<Uint> fset_mbest;
   if (mbest) {
      if (ref->lastPhrase->src_words == mbest->lastPhrase->src_words &&
          ref->lastPhrase->phrase == mbest->lastPhrase->phrase) {
         if (verbose > 1) cerr << "[same as reference]" << endl;
         fset_mbest = fset_ref;
      } else {
         getFeatures(*mbest, fset_mbest, true, learn_conjunctions);
      }
   } else
      if (verbose > 1) cerr << "[none]" << endl;

   // count feature frequencies & update weights
   Uint intersect_size = 0;
   vector<Uint> res(fset_ref.size() + fset_mbest.size());
   vector<Uint>::iterator e = 
      set_union(fset_ref.begin(), fset_ref.end(), 
                fset_mbest.begin(), fset_mbest.end(), res.begin());
   for (vector<Uint>::iterator p = res.begin(); p != e; ++p) {
      ++feature_freqs[*p];
      const bool in_ref = fset_ref.find(*p) != fset_ref.end();
      const bool in_model = fset_mbest.find(*p) != fset_mbest.end();
      if (in_ref && !in_model) incrWeight(*p, 1.0);
      else if (in_model && !in_ref) incrWeight(*p, -1.0);
      else ++intersect_size;
   }
   if (prune0) pruneZeroWeightFeatures(nf);
   return (e - res.begin()) - intersect_size;
}

string SparseModel::templ_exten = ".templates";
string SparseModel::feats_exten = ".feats.gz";
string SparseModel::wts_exten = ".wts.gz";
string SparseModel::wtsums_exten = ".wtsums.gz";
string SparseModel::freqs_exten = ".freqs.gz";
string SparseModel::voc_exten = ".voc.gz";
string SparseModel::numex_exten = ".numex";
string SparseModel::pfs_flag = "#pfs";
string SparseModel::fire_flag = "#col";

void SparseModel::save(const string& file, bool prune_templates)
{
   // construct template id remapping (identity if prune_templates is false)

   vector<bool> active_templates(event_templates.size(), 
                                 prune_templates ? false : true);
   if (prune_templates) {
      for (Uint i = 0; i < features.size(); ++i)
         for (Uint j = 0; j < features[i].events.size(); ++j)
            active_templates[features[i].events[j].tid] = true;
   }
   vector<Uint> template_map(event_templates.size());  // old tid -> new tid
   Uint tid = 0;
   for (Uint i = 0; i < event_templates.size(); ++i)
      if (active_templates[i]) template_map[i] = tid++;
   
   // save features and weights, re-mapping template ids if appropriate

   const string fname(file + feats_exten);
   const string wname(file + wts_exten);
   oSafeMagicStream osf(fname);
   oSafeMagicStream osw(wname);
   for (Uint i = 0; i < features.size(); ++i) {
      osw << features[i].weight << endl;
      for (Uint j = 0; j < features[i].events.size(); ++j) {
         if (j) osf << ' ';
         osf << template_map[features[i].events[j].tid] << ',' 
             << features[i].events[j].eid;
      }
      osf << endl;
   }

   // save frequencies and weight sums

   averageWeights(true);
   assert(feature_freqs.size() == feature_wtsums.size());
   oSafeMagicStream os(file + freqs_exten);
   oSafeMagicStream oss(file + wtsums_exten);
   for (Uint i = 0; i < feature_freqs.size(); ++i) {
      os << feature_freqs[i] << endl;
      oss << feature_wtsums[i] << endl;
   }
   // save templates, voc, and numex
   
   const string tname(file + templ_exten);
   oSafeMagicStream ost(tname);
   for (Uint i = 0; i < event_templates.size(); ++i)
      if (active_templates[i])
         ost << event_templates[i]->description() << endl;
   voc->write(file + voc_exten);

   oSafeMagicStream osn(file + numex_exten);
   osn << exno << endl;

   if (verbose) 
      cerr << "saved SparseModel to " << file << endl;
}

void SparseModel::ensureTemplateMatch(const SparseModel& m)
{
   assert(event_templates.size() == m.event_templates.size());
   for (Uint i = 0; i < event_templates.size(); ++i)
      assert(event_templates[i]->description2() == 
             m.event_templates[i]->description2());
}

void SparseModel::add(const vector<string>& files, double prune)
{
   // set up feature -> index map for current model

   typedef map<Feature,Uint,LexComp> FMap;
   FMap fmap(LexComp(*this));
   for (Uint i = 0; i < features.size(); ++i)
      fmap[features[i]] = i;

   // add each model in turn

   for (Uint i = 0; i < files.size(); ++i) {

      Uint new_feat_count = 0;

      // load model and have it use current voc
      SparseModel m(files[i], "", 0, voc, false, false, true, false);
      ensureTemplateMatch(m);

      // add m's features
      for (Uint j = 0; j < m.features.size(); ++j) {
         FMap::iterator p = fmap.find(m.features[j]);
         if (p == fmap.end()) {  // new feature
            if (abs(m.features[j].weight) >= prune) {
               fmap[m.features[j]] = features.size();
               features.push_back(m.features[j]);
               feature_freqs.push_back(m.feature_freqs[j]);
               feature_wtsums.push_back(m.feature_wtsums[j]);
               ++new_feat_count;
            }
         }
         else {
            const Uint ind = p->second;
            features[ind].weight += m.features[j].weight;
            feature_freqs[ind] += m.feature_freqs[j];
            feature_wtsums[ind] += m.feature_wtsums[j];
         }
      }
      exno += m.exno;
      if (verbose) cerr << "added model " << files[i] << " - "
                        << new_feat_count << " new features" << endl;
   }
   // rebuild events and update averaging info

   feature_lastex.assign(features.size(), exno);
   rebuildEventMap();
}

void SparseModel::add2(const string& file)
{
   // load model and have it use current voc

   SparseModel m(file, "", 0, voc, false, false, true, false);
   ensureTemplateMatch(m);

   // add events and features from m to current model

   vector<Uint> curf(m.features.size(), 0); // m feat -> current feat + 1
   for (EventMap::iterator p = m.events.begin(); p != m.events.end(); ++p) {

      // add m event if not already there, and look up feature lists

      vector<Uint>& m_feats = p->second;
      vector<Uint>& cur_feats = events[p->first];

      // add each m feature

      Uint cur_orig_numfeats = cur_feats.size();
      for (Uint i = 0; i < m_feats.size(); ++i) {
         const Uint mf = m_feats[i];  // current m feature index
         const Uint cf = curf[mf]; // corresp cur feature index if > 0

         // try to find the m feature in current list

         Uint j = 0;
         for (j = 0; j < cur_orig_numfeats; ++j) {
            if (cf && cf == cur_feats[j]+1)
               // feature already in model and in cur_feats
               break;
            else if (m.features[mf] == features[cur_feats[j]]) {
               // feature content is the same, so map the feature and add wts
               curf[mf] = cur_feats[j]+1;
               features[cur_feats[j]].weight += m.features[mf].weight;
               feature_freqs[cur_feats[j]] += m.feature_freqs[mf];
               feature_wtsums[cur_feats[j]] += m.feature_wtsums[mf];
               break;
            }
         }
         // if not found, add to model if new, then add to cur_feats list
         
         if (j == cur_orig_numfeats) {
            if (cf == 0) {      // new feature
               curf[mf] = features.size()+1;
               features.push_back(Feature(m.features[mf]));
               features.back().weight = m.features[mf].weight;
               feature_freqs.push_back(m.feature_freqs[mf]);
               feature_wtsums.push_back(m.feature_wtsums[mf]);
            }
            cur_feats.push_back(curf[mf]-1);
         }
      }
   }
   // add exno and update lastex info

   exno += m.exno;
   feature_lastex.assign(features.size(), exno);
}

bool SparseModel::compare(const string& file)
{
   const string h = "compare failed: ";

   // load other model with current voc

   SparseModel m(file, "", 0, voc, false, false, false, false);

   // check that event templates match

   if (event_templates.size() != m.event_templates.size()) {
      cerr << h << "different numbers of event templates" << endl;
      return false;
   }
   for (Uint i = 0; i < event_templates.size(); ++i)
      if (event_templates[i]->description2() !=
          m.event_templates[i]->description2()) {
         cerr << h << "template " << i << " differs: " 
              << event_templates[i]->description2() << " - vs - "
              << m.event_templates[i]->description2() << endl;
         return false;
      }

   // check that feature sets match

   if (features.size() != m.features.size()) {
      cerr << h << "different numbers of features" << endl;
      return false;
   }
   vector<Uint> indexes(features.size());
   for (Uint i = 0; i < features.size(); ++i)  indexes[i] = i;
   sort(indexes.begin(), indexes.begin() + features.size(), LexComp(*this));
   vector<Uint> m_indexes(m.features.size());
   for (Uint i = 0; i < m.features.size(); ++i)  m_indexes[i] = i;
   sort(m_indexes.begin(), m_indexes.begin() + m.features.size(), LexComp(m));
   for (Uint i = 0; i < features.size(); ++i)
      if (features[indexes[i]] != m.features[m_indexes[i]]) {
         cerr << h << i << "th feature in canonical order differs: "
              << features[indexes[i]].toString(*this) << " -vs- " 
              << m.features[m_indexes[i]].toString(m) << endl;
         return false;
      }

   return true;
}

void SparseModel::replaceVoc(Voc* oldvoc, Voc* newvoc)
{
   // Update event ids in the feature list using the templates' remapEvent() functions.

   voc = oldvoc;
   for (Uint i = 0; i < features.size(); ++i) {
      vector<Event>::iterator p;      
      for (p = features[i].events.begin(); p < features[i].events.end(); ++p)
         p->eid = event_templates[p->tid]->remapEvent(p->eid, newvoc);
      sort(features[i].events.begin(), features[i].events.end());
   }
   // Inform templates about the change

   for (Uint i = 0; i < event_templates.size(); ++i)
      event_templates[i]->replaceVoc(newvoc);
      
   // Update shared structures: voc, ngcounts_map, and subvoc_map. The latter
   // two structures are just deleted and read in again from file, which can be
   // costly.

   voc = newvoc;
   for (map<string,NgramCounts*>::iterator p = ngcounts_map.begin(); 
        p != ngcounts_map.end(); ++p) {
      delete p->second;
      p->second = new NgramCounts(voc, p->first);
   }
   for (map<string, map<Uint,Uint>*>::iterator p = subvoc_map.begin();
        p != subvoc_map.end(); ++p) {
      delete p->second;
      p->second = readSubVoc(p->first);
   }
   for (map<string, ClusterMap*>::iterator p = file_to_clust.begin();
        p != file_to_clust.end(); ++p) {
      delete p->second;
      p->second = new ClusterMap(*this, p->first);
   }

   // Rebuild map from events to features, for new ids.

   rebuildEventMap();
}

string SparseModel::stripSuffixFlags(const string& modelname, 
                                     bool* had_pfs, string* idname, string* fireval,
                                     string* tag)
{
   if (had_pfs) *had_pfs = false;
   if (idname) *idname = "";
   if (fireval) *fireval = "";
   if (tag) *tag = "";
   string ret = modelname;
   for (string::size_type p = ret.rfind('#'); p != string::npos; p = ret.rfind('#')) {
      string suff = ret.substr(p);
      if (suff == pfs_flag) {
         if (had_pfs) *had_pfs = true;
      } else if (isPrefix(fire_flag.c_str(), suff.c_str())) {
         if (fireval) *fireval = suff;
      } else if (suff.size() >=3 && suff[1] == '_') {
         if (tag) *tag = ret.substr(p+1);
      } else {
         if (idname) *idname = ret.substr(p+1);
      }
      ret.erase(p);
   }
   return ret;
}

string SparseModel::localWtsFilename(const string& modelname, const string& relative_to, const string& suff)
{
   assert(modelname != "");
   string ret;

   if (modelname[0] == '/') {
      ret = "a";
   } else if (relative_to.empty()) {
      ret = "r";
   } else {
      ret = relative_to + "/r";
   }

   for (Uint i = 0; i < modelname.size(); ++i)
      if (modelname[i] == '/') ret += "_";
      else ret += modelname[i];
   for (Uint i = 0; i < suff.size(); ++i)
      if (suff[i] == '#' || suff[i] == ' ' || suff[i] == '/')
         ret += "_";
      else
         ret += suff[i];
   return ret + wts_exten;
}

void SparseModel::readWeights(const string& oname, const string& relative_to,
      vector<float>& weights, bool allow_non_local)
{
   string fireflag, tag;
   const string name = stripSuffixFlags(oname, NULL, NULL, &fireflag, &tag);
   fireflag += tag;
   weights.clear();
   string wtsfile = localWtsFilename(name, relative_to, fireflag);
   if (!check_if_exists(wtsfile)) {
      if (!allow_non_local)
         error(ETFatal, "Local sparse weights file %s required but not found.\nUse -sparse-model-allow-non-local-wts if you want to use the model's original (possibly untuned) sparse weights file.\nOr remove any explicit [weight-sparse] if your model is not actually tuned yet.", wtsfile.c_str());
      else
         error(ETWarn, "Cannot find local SparseModel weights file; using remote one, which might not be tuned.");
      wtsfile = name + wts_exten; // non-local wts file
   }
   iSafeMagicStream istr(wtsfile);
   string line;
   while (getline(istr, line))
      weights.push_back(conv<float>(line));
}

Uint SparseModel::writeWeights(const string& oname, const string& relative_to,
                               vector<float>& weights, Uint os)
{
   string fireflag, tag;
   const string name = stripSuffixFlags(oname, NULL, NULL, &fireflag, &tag);
   fireflag += tag;
   const string origwts = name + wts_exten;
   const string newwts = localWtsFilename(name, relative_to, fireflag);

   // backup existing weights file if this needs doing

   if (origwts == newwts) {
      error(ETWarn, "overwriting model weights; producing backup copy");
      const string backupwts = name + ".bak" + wts_exten;
      if (!check_if_exists(backupwts))
         boost::filesystem::copy_file(origwts, backupwts);
   }

   // count weights in original file, and write that number from <weights> to
   // <newwts> 

   const Uint numwts = countFileLines(origwts.c_str());
   assert(os + numwts <= weights.size());
   oSafeMagicStream ostr(newwts);
   for (Uint i = 0; i < numwts; ++i)
      ostr << weights[os + i] << endl;
   return numwts;
}


SparseModel::SparseModel(const string& ofile, const string& relative_to,
                         Uint verbose, Voc* newvoc,
                         bool local_wts, bool allow_non_local,
                         bool load_opts, bool in_decoder) :
   verbose(verbose), learn_conjunctions(false), max_features_for_conjoin(0), 
   min_freq_for_conjoin(0), prune0(false), precompute_future_score(false), voc(NULL)
{
   time_t start_time = time(NULL);
   // will use 'real' voc to read in EventTemplates that don't depend on stored
   // voc
   voc = newvoc;
   assert(voc);
   
   // get values of suffix #flags, and process

   string fireval, tag;
   const string partial_file = stripSuffixFlags(ofile, &precompute_future_score, &idname, &fireval, &tag);
   string file;
   if (relative_to.empty() || partial_file[0] == '/') {
      file = partial_file;
      description = "SparseModel:" + ofile;
   } else {
      file = relative_to + "/" + partial_file;
      description = "SparseModel:" + relative_to + "/" + ofile;
   }
   path = DirName(file);
   if (fireval != "") {  // format: colN=TAG
      if (idname == "")
         error(ETFatal, "need #ID flag with #colN=TAG flag for SparseModel");
      vector<string> toks;
      if (split(fireval, toks, "=") != 2)
         error(ETFatal, "expecting #colN=TAG flag for SparseModel; got %s", fireval.c_str());
      Uint col = conv<Uint>(toks[0].substr(fire_flag.length()));
      assert(col > 0);
      col--;
      const string tag = toks[1];
      iSafeMagicStream idfile(idname);
      string line;
      bool gotone = false;
      while (getline(idfile, line)) {
         splitZ(line, toks);
         if (col >= toks.size())
            error(ETFatal, "there is no column %d in id file line: %s",
                           col+1, line.c_str());
         fire_on_sent.push_back(toks[col] == tag);
         if (fire_on_sent.back()) gotone = true;
      }
      if (!gotone)
         error(ETWarn, "tag in flag %s failed to match any in ID file %s - SparseModel will NEVER be active",
               fireval.c_str(), idname.c_str());
   }
   fireval += tag;

   // read structure file and saved voc

   const string tname(file + templ_exten);
   string spec;
   gulpFile(tname.c_str(), spec);
   createEventTemplates(spec);
   Voc oldvoc;
   oldvoc.read(file + voc_exten);
   voc = &oldvoc;

   // read features/wts & remap voc

   string wtsfile = file + wts_exten;
   if (local_wts) {
      const string localwts = localWtsFilename(partial_file, relative_to, fireval);
      if (check_if_exists(localwts)) {
         wtsfile = localwts;
         if (verbose) cerr << "local SparseModel weights file found at " << wtsfile << endl;
      } else {
         if (!allow_non_local)
            error(ETFatal, "Local sparse weights file %s required but not found.\nUse -sparse-model-allow-non-local-wts if you want to use the model's original (possibly untuned) sparse weights file.\nOr remove any explicit [weight-sparse] if your model is not actually tuned yet.", localwts.c_str());
         else
            error(ETWarn, "Cannot find local SparseModel weights file; using remote one, which might not be tuned.");
      }
   }
   iSafeMagicStream fstr(file + feats_exten);
   iSafeMagicStream wstr(wtsfile);
   string line, wline;
   vector<string> toks;
   vector<Uint> inds;
   bool all_zeros = true;
   while (getline(fstr, line)) {
      if (!getline(wstr, wline)) 
         error(ETFatal, "weights file too short: %s", wtsfile.c_str());
      features.push_back(Feature());
      features.back().weight = conv<float>(wline);
      all_zeros = all_zeros && features.back().weight == 0.0;
      splitZ(line, toks);       // t,e t,e ...
      for (Uint i = 0; i < toks.size(); ++i) {
         inds.clear();
         assert(split(toks[i], inds, ",") == 2);
         const Uint newid = event_templates[inds[0]]->remapEvent(inds[1], newvoc);
         features.back().events.push_back(Event(inds[0], newid));
      }
      sort(features.back().events.begin(), features.back().events.end());
   }
   voc = newvoc;                // from now on
   if (verbose) {
      cerr << "loaded " << features.size() << " SparseModel features from " << file+feats_exten << endl;
      cerr << "loaded " << features.size() << " SparseModel weights from " << wtsfile <<
         (all_zeros ? " -- all weights are 0" : " -- non-0 weights found") << endl;
   }

   // read optional info if requested

   if (load_opts) {
      {
         iSafeMagicStream str(file + freqs_exten);
         while (getline(str, line))
            feature_freqs.push_back(conv<Uint>(line));
         assert(features.size() == feature_freqs.size());
      }{
         iSafeMagicStream str(file + wtsums_exten);
         while (getline(str, line))
            feature_wtsums.push_back(conv<float>(line));
         assert(features.size() == feature_wtsums.size());
      }{
         iSafeMagicStream str(file + numex_exten);
         str >> exno;
         feature_lastex.assign(features.size(), exno);
      }
   }

   // create event map
 
   rebuildEventMap();
   if (verbose)
      cerr << "created " << events.size() << " SparseModel events" << endl;

//     // CAC: Temporary print to test sparse distortion tracking
//     DistortionMaster* dmaster = DistortionMaster::instance();
//     const vector<SparseModel::Distortion*> sparseDists = dmaster->active();
//     for(Uint i=0;i<sparseDists.size();i++) {
//        SparseModel::Distortion* sparseDist = sparseDists[i];
//        cerr << "used " << sparseDist->name() << " which had " << sparseDist->clients().size() << " clients" << endl;
//        const vector<SparseModel::EventTemplate*> clients = sparseDist->clients();
//        for(Uint j=0;j<clients.size();j++){
//           cerr << "\t" << clients[j]->description() << endl;
//        }
//     }
   
   // compile partials

   if (in_decoder) {
      compilePartialFeatures();
      if (verbose)
         cerr << "compiled " << partial_features.size() 
              << " partial features" << endl;

      buildDecodingEventMaps();
   }

   cerr << "Loaded SparseModel in " << (time(NULL) - start_time) << "s" << endl;
}

void SparseModel::rebuildEventMap()
{
   events.clear();
   vector<Uint> v(1);
   for (Uint i = 0; i < features.size(); ++i) {
      v[0] = i;
      for (Uint j = 0; j < features[i].events.size(); ++j) {
         pair<EventMap::iterator, bool> r = 
            events.insert(make_pair(features[i].events[j], v));
         if (!r.second) r.first->second.push_back(i);
      }
   }
}

void SparseModel::buildDecodingEventMapsHelper(Uint feature_id, EventMap& event_map,
      set<Uint>& template_id_set)
{
   static vector<Uint> v(1);
   v[0] = feature_id;
   for (Uint j = 0; j < features[feature_id].events.size(); ++j) {
      pair<EventMap::iterator, bool> r =
         event_map.insert(make_pair(features[feature_id].events[j], v));
      if (!r.second) r.first->second.push_back(feature_id);
      template_id_set.insert(features[feature_id].events[j].tid);
   }
}


void SparseModel::buildDecodingEventMaps()
{
   pure_event_template_ids.clear();
   pure_events.clear();
   pure_distortion_event_template_ids.clear();
   pure_distortion_events.clear();
   other_event_template_ids.clear();
   other_events.clear();

   set<Uint> pure_template_id_set, pure_distortion_template_id_set, other_template_id_set;
   for (Uint i = 0; i < features.size(); ++i) {
      bool isPure(true), isPureDistortion(true);
      for (Uint j = 0; j < features[i].events.size(); ++j) {
         EventTemplate* const t = event_templates[features[i].events[j].tid];
         isPure = isPure && t->isPure();
         // pure distortion features can consist of conjunctions of pure and pure-distortion events.
         isPureDistortion = isPureDistortion && (t->isPure() || t->isPureDistortion());
      }
      if (isPure)
         buildDecodingEventMapsHelper(i, pure_events, pure_template_id_set);
      else if (isPureDistortion)
         buildDecodingEventMapsHelper(i, pure_distortion_events, pure_distortion_template_id_set);
      else
         buildDecodingEventMapsHelper(i, other_events, other_template_id_set);
   }

   pure_event_template_ids.reserve(pure_template_id_set.size());
   pure_event_template_ids.assign(pure_template_id_set.begin(), pure_template_id_set.end());

   pure_distortion_event_template_ids.reserve(pure_distortion_template_id_set.size());
   pure_distortion_event_template_ids.assign(pure_distortion_template_id_set.begin(), pure_distortion_template_id_set.end());

   other_event_template_ids.reserve(other_template_id_set.size());
   other_event_template_ids.assign(other_template_id_set.begin(), other_template_id_set.end());
}

void SparseModel::scaleWeights(double v)
{
   for (Uint i = 0; i < features.size(); ++i) 
      features[i].weight *= v;
   for (Uint i = 0; i < feature_wtsums.size(); ++i)
      feature_wtsums[i] *= v;
}

void SparseModel::averageWeights(bool sum_only)
{
   assert(feature_wtsums.size() == features.size());
   assert(feature_lastex.size() == features.size());
   for (Uint i = 0; i < features.size(); ++i) {
      feature_wtsums[i] += features[i].weight * (exno - feature_lastex[i]);
      feature_lastex[i] = exno;
      if (exno && !sum_only) features[i].weight = feature_wtsums[i] / exno;
   }
   if (verbose && !sum_only) 
      cerr << "set weights to average values across " 
           << exno << " examples" << endl;
}

void SparseModel::prune(Uint num_features, bool by_freq, bool conj_only)
{
   if (num_features >= features.size()) return;
   assert(feature_freqs.size() == features.size());
   assert(feature_lastex.size() == features.size());
   assert(feature_wtsums.size() == features.size());

   vector<Uint> indexes;
   for (Uint i = 0; i < features.size(); ++i)
      if (!conj_only || features[i].events.size() > 1) indexes.push_back(i);
   partial_sort(indexes.begin(), indexes.begin() + num_features, indexes.end(),
                PruneComp(*this, by_freq));
   vector<Feature> newfeatures(num_features);
   vector<Uint> newfeature_freqs(num_features);
   vector<Uint> newfeature_lastex(num_features);
   vector<float> newfeature_wtsums(num_features);
   for (Uint i = 0; i < num_features; ++i) {
      newfeatures[i] = features[indexes[i]];
      newfeature_freqs[i] = feature_freqs[indexes[i]];
      newfeature_lastex[i] = feature_lastex[indexes[i]];
      newfeature_wtsums[i] = feature_wtsums[indexes[i]];
   }
   const Uint old_nf = features.size();
   const Uint old_ne = events.size();
   features = newfeatures;
   feature_freqs = newfeature_freqs;
   feature_lastex = newfeature_lastex;
   feature_wtsums = newfeature_wtsums;
   
   rebuildEventMap();

   if (verbose) 
      cerr << "pruned features from " << old_nf << " to " << features.size()
           << "; events reduced from " << old_ne << " to " << events.size() 
           << endl;
}

void SparseModel::pruneZeroWeightFeatures(Uint start_index)
{
   if (verbose > 1)
      cerr << "pruning from " << start_index << " to " << features.size() << endl;
   Uint num_retained = 0;
   for (Uint i = start_index; i < features.size(); ++i) {
      if (features[i].weight == 0.0) {  // remove all event references to this feature
         for (Uint j = 0; j < features[i].events.size(); ++j) {
            vector<Uint>& flist = events[features[i].events[j]];
            vector<Uint>::iterator e = remove(flist.begin(), flist.end(), i);
            flist.resize(e - flist.begin());
            if (flist.size() == 0)
               assert(events.erase(features[i].events[j]) == 1);
         }
      } else {   // re-index feature if necessary
         const Uint fi = start_index + num_retained++;  // new index
         if (fi != i) {
            for (Uint j = 0; j < features[i].events.size(); ++j) {
               vector<Uint>& flist = events[features[i].events[j]];
               replace(flist.begin(), flist.end(), i, fi);
            }
            features[fi] = features[i];
            feature_freqs[fi] = feature_freqs[i];
            feature_lastex[fi] = feature_lastex[i];
            feature_wtsums[fi] = feature_wtsums[i];
         }
      }
   }
   features.resize(start_index + num_retained);
   feature_freqs.resize(start_index + num_retained);
   feature_lastex.resize(start_index + num_retained);
   feature_wtsums.resize(start_index + num_retained);

   if (verbose > 1) cerr << num_retained << " features retained" << endl;
}

void SparseModel::dump(ostream& os, Uint start_index, Uint verbose)
{
   if (verbose > 1) {
      for (Uint i = 0; i < features.size(); ++i) {
         if (start_index) os << start_index + i << '\t';
         os << features[i].weight << '\t' << "SparseModel:" << features[i].toString(*this) << endl;
      }
   } else {
      set<Uint> prev_tids;      // template ids
      Uint beg = 0;
      for (Uint i = 0; i <= features.size(); ++i) {
         set<Uint> tids;
         if (i < features.size())
            for (Uint j = 0; j < features[i].events.size(); ++j)
               tids.insert(features[i].events[j].tid);
         if (prev_tids.empty()) prev_tids = tids;
         if (tids != prev_tids) {
            if (start_index) {
               if (i > beg+1) os << start_index+beg << '-' << start_index+i-1 << '\t';
               else os << start_index+beg << '\t';
            }
            double wt = 0.0;
            for (Uint j = beg; j < i; ++j) wt += features[j].weight;
            os << wt << '\t';
            os << "SparseModel:";
            for (set<Uint>::iterator p = prev_tids.begin(); p != prev_tids.end(); ++p) {
               os << event_templates[*p]->description2();
               set<Uint>::iterator n = p; ++n;
               if (n != prev_tids.end()) os << " & ";
            }
            os << endl;
            prev_tids = tids;
            beg = i;
         }
      }
   }
}

/*-----------------------------------------------------------------------------
  SparseModel's DecoderFeature virtuals
  ---------------------------------------------------------------------------*/

void SparseModel::newSrcSent(const newSrcSentInfo& info) {
   ssid = info.external_src_sent_id;
   active = fire_on_sent.empty() || fire_on_sent[ssid];
   srcsent.resize(info.src_sent.size());
   for (Uint i = 0; i < info.src_sent.size(); ++i)
      srcsent[i] = voc->add(info.src_sent[i].c_str());
   for (Uint i = 0; i < event_templates.size(); ++i)
      event_templates[i]->newSrcSent(info);
}

static const bool debug_score_with_caching = false;

namespace Portage {
   struct SparseCache : public PhrasePairAnnotation {
      double pure_score;
      vector<double> distortion_scores;
      vector<bool> dist_score_init;

      virtual PhrasePairAnnotation* clone() const { return new SparseCache(*this); }
      static SparseCache* getOrCreate(const AnnotationList& b, bool* is_new = NULL) {
         return PhrasePairAnnotation::getOrCreate<SparseCache>(name, b, is_new);
      }
      static const string name;
   };
   const string SparseCache::name = "sparse_score_cache";
}

double SparseModel::score(const PartialTranslation& hyp) {
   if (!active) return 0.0;

   bool cache_is_new;
   SparseCache* const cache = SparseCache::getOrCreate(hyp.lastPhrase->annotations, &cache_is_new);
   if (cache_is_new)
      cache->pure_score = tallyFeatures(hyp, pure_event_template_ids, pure_events);
   const double pure_r = cache->pure_score;
   //double pure_r = tallyFeatures(hyp, pure_event_template_ids, pure_events);

   double pure_distortion_r = 0;
   bool got_pure_dist_from_cache = false;
   if (!pure_distortion_event_template_ids.empty()) {
      DistortionMaster* dmaster = DistortionMaster::instance();
      if (dmaster->active().size() == 1) {
         if (cache_is_new) {
            cache->distortion_scores.assign(dmaster->maxSig(), 0);
            cache->dist_score_init.assign(dmaster->maxSig(), false);
         }
         SparseModel::Distortion* const sparseDist = dmaster->active().front();
         const Uint dist_sig = sparseDist->sig(hyp);
         assert(dist_sig < cache->dist_score_init.size());
         if (!cache->dist_score_init[dist_sig]) {
            cache->distortion_scores[dist_sig] =
               tallyFeatures(hyp, pure_distortion_event_template_ids, pure_distortion_events);
            cache->dist_score_init[dist_sig] = true;
         }
         pure_distortion_r = cache->distortion_scores[dist_sig];
         got_pure_dist_from_cache = true;
      }
   }
   if (!got_pure_dist_from_cache)
      pure_distortion_r =
         tallyFeatures(hyp, pure_distortion_event_template_ids, pure_distortion_events);

   const double other_r = tallyFeatures(hyp, other_event_template_ids, other_events);

   const double r = pure_r + pure_distortion_r + other_r;

   if (debug_score_with_caching) {
      assert(pure_r == tallyFeatures(hyp, pure_event_template_ids, pure_events));
      assert(pure_distortion_r ==
         tallyFeatures(hyp, pure_distortion_event_template_ids, pure_distortion_events));

      set<Uint> fset;
      double old_r = 0;
      getFeatures(hyp, fset, false, false);
      for (set<Uint>::iterator p = fset.begin(); p != fset.end(); ++p)
         old_r += features[*p].weight;

      static Uint warn_count = 0;
      if (warn_count < 100 && abs(r-old_r) > 1e-05) {
         error(ETWarn, "Different sparse feature results with and without caching for hyp: %g != %g\n", r, old_r);
         cerr << hyp.lastPhrase->src_words << " | ";
         cerr << displayUintSet(hyp.sourceWordsNotCovered, false) << endl;
         cerr << "\tnum covered words     " << hyp.numSourceWordsCovered << endl;
         if (hyp.isLmContextSizeSet())
            cerr << "\tlm context size       " << hyp.getLmContextSize() << endl;
         hyp.lastPhrase->annotations.display(cerr);
         ++warn_count;
      }
   }

   return r;
}

void SparseModel::prime(const string& file, const string& relative_to)
{
   const string partial_file = stripSuffixFlags(file);
   string full_file;
   if (relative_to.empty() || partial_file[0] == '/') {
      full_file = partial_file;
   } else {
      full_file = relative_to + "/" + partial_file;
   }
   string path = DirName(full_file);

   if (is_directory(path)) {
      string cmd = "cat '" + path + "'/*.mmcls > /dev/null";
      cerr << "\tPriming: " << path << "/*.mmcls" << endl;  // SAM DEBUGGING
      //cerr << "Cmd = " << cmd << endl;
      int rc = ::system(cmd.c_str());
      FOR_ASSERT(rc);
   }
}

// The returned value is the sum of partial_feature indexes, which is not a
// unique signature for sets of indexes, but satisfies the requirement for this
// function.

Uint SparseModel::computeRecombHash(const PartialTranslation &pt)
{
   if (!active) return 0.0;
   Uint h = 0;
   set<Uint> fset;
   getPartialFeatures(pt, fset);
   for (set<Uint>::iterator p = fset.begin(); p != fset.end(); ++p)
      h += *p;
   return h;
}

bool SparseModel::isRecombinable(const PartialTranslation &pt1,
                                 const PartialTranslation &pt2)
{
   if (!active) return true;
   set<Uint> fset1, fset2;
   getPartialFeatures(pt1, fset1);
   getPartialFeatures(pt2, fset2);
   return fset1 == fset2;
}

// Not very efficient, since we poll all features then discard the non-pure
// ones, but hopefully efficiency isn't too crucial here.

double SparseModel::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   if (!precompute_future_score || !active)
      return 0.0;

   static PartialTranslation dummy((Uint)0);

   PartialTranslation pt(&dummy, &phrase_info);
   double r = 0.0;
   set<Uint> fset;
   getFeatures(pt, fset, false, false);
   for (set<Uint>::iterator p = fset.begin(); p != fset.end(); ++p) {
      const Feature& f = features[*p];
      bool pure = true;
      for (Uint i = 0; i < f.events.size(); ++i) {
         pure = pure && event_templates[f.events[i].tid]->isPure();
         if (!pure) break;
      }
      if (pure) r += features[*p].weight;
   }
   return r;
}

/*-----------------------------------------------------------------------------
  Event Templates
  ---------------------------------------------------------------------------*/

// TgtContextWordAtPos

TgtContextWordAtPos::TgtContextWordAtPos(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   string a(args);
   const int p = conv<int>(trim(a));
   if (p >= 0) 
      error(ETFatal, "%s: position argument must be < 0", name().c_str());
   pos = Uint(-p);
}

void TgtContextWordAtPos::addEvents(const PartialTranslation& pt,
                                    vector<Uint>& events)
{
   if (pt.back == NULL) return; // ? happens when called from isRecombinable
   Uint hlen = 0;
   const PartialTranslation* c = pt.back;
   for (; c->back; c = c->back) {
      hlen += c->lastPhrase->phrase.size();
      if (hlen >= pos)
         break;
   }
   if (c->back) events.push_back(c->lastPhrase->phrase[hlen-pos]);
}

// PhraseWordAtPos

PhraseWordAtPos::PhraseWordAtPos(SparseModel& m, const string& args, 
                                 const string& cname) :
   EventTemplate(m, args)
{
   string a(args);
   pos = conv<int>(trim(a));
   if (pos == 0)
      error(ETFatal, "%s: position argument must be non-zero", cname.c_str());
}

void PhraseWordAtPos::addEvents(vector<Uint>::const_iterator b, 
                                vector<Uint>::const_iterator e, 
                                vector<Uint>& events)
{
   if (pos > 0 && b + pos <= e)
      events.push_back(*(b+pos-1));
   else if (pos < 0 && b - pos <= e)
      events.push_back(*(e+pos));
}

//  PhraseContainsWord

void PhraseContainsWord::addEvents(vector<Uint>::const_iterator b,
                                   vector<Uint>::const_iterator e,
                                   vector<Uint>& events)
{
   set<Uint> words(b, e);
   events.assign(words.begin(), words.end());
}

// TgtNgramCountBin

const char* TgtNgramCountBin::bos = "<s>";
const char* TgtNgramCountBin::eos = "</s>";

TgtNgramCountBin::TgtNgramCountBin(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   // EJJ TODO (or not): enforce LM context requirements here somehow.
   // We know we always have at least one word, but that's not always enough
   // for this event. However, we currently has no plans to fix this problem
   // since the TgtNgramCountBin approach to language modelling has been shown
   // not to work.
   vector<string> toks;
   if (split(args, toks) != 5)
      error(ETFatal, "expected 5 argument values for TgtNgramCountBin args");
   n = conv<Uint>(toks[1]);
   low = conv<Uint>(toks[2]);
   high = conv<Uint>(toks[3]);
   pos = conv<int>(toks[4]);

   if (pos > 0)
      error(ETFatal, "TgtNgramCountBin is buggy for pos > 0, sorry");

   if (n == 0)
      error(ETFatal, "TgtNgramCountBin order must be non-zero");
   if (n < 2 && pos == 0)
      error(ETFatal, "TgtNgramCountBin order must be >= 2 if pos is 0");

   if (high !=0 && high < low)
      error(ETFatal, "TgtNgramCountBin frequency range empty");

   if (m.ngcounts_map.find(toks[0]) != m.ngcounts_map.end())
      ngcounts = m.ngcounts_map[toks[0]];
   else {
      const string fullname = adjustRelativePath(m.path, toks[0]);
      ngcounts = m.ngcounts_map[toks[0]] = new NgramCounts(m.voc, fullname);
   }

   if (n > ngcounts->maxLen())
      error(ETWarn,
            "TgtNgramCountBin len > longest stored ngram - feature will never fire");

   vector<string> tst(1);
   tst[0] = bos;
   using_bos = ngcounts->count(tst) != 0;
   tst[0] = eos;
   using_eos = ngcounts->count(tst) != 0;

   if (pos == 0 && !using_eos)
      error(ETWarn, 
            "TgtNgramCountBin pos of 0 requires eos in ngrams data, which is missing %s",
            " - feature will never fire");
}

void TgtNgramCountBin::addEvents(const PartialTranslation& context, 
                                 vector<Uint>& event_ids) 
{
   static VectorPhrase ngram;
   ngram.clear();

   // number of words from end of ngram of interest to end of last phrase
   int extra = pos > 0 ? context.lastPhrase->phrase.size() - pos : -1 - pos;

   // get ngram of interest and trailing words if necessary
   context.getLastWords(ngram, Uint(n + extra));

   // insert sent-beg marker before (n-1)-gram if called for
   if (ngram.size() == n + extra - 1 && using_bos)
      ngram.insert(ngram.begin(), m.voc->add(bos));

   // insert sent-end marker after (n-1)-gram if called for
   if (pos == 0 && context.sourceWordsNotCovered.empty() && 
       ngram.size() == n - 1) {
      ngram.push_back(m.voc->add(eos));
      extra = 0;                // formerly -1, now we have all we need
   }

   // look up ngram to see if its count matches what we're interested in
   if (ngram.size() == n + extra) {
      const Uint c = ngcounts->count(&ngram[0], n);
      if (c >= low && (high == 0 || c <= high))
         event_ids.push_back(0); // fire
   }
}

PhrasePairLengthBin::PhrasePairLengthBin(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 4)
      error(ETFatal, "expected 4 argument values for PhrasePairLengthBin args");
   srcmin = conv<Uint>(toks[0]);
   srcmax = conv<Uint>(toks[1]);
   tgtmin = conv<Uint>(toks[2]);
   tgtmax = conv<Uint>(toks[3]);

   if (srcmax && srcmin > srcmax)
      error(ETFatal, "PhrasePairLengthBin srcmin must be <= srcmax");
   if (tgtmax && tgtmin > tgtmax)
      error(ETFatal, "PhrasePairLengthBin tgtmin must be <= tgtmax");
}

PhrasePairTotalLengthBin::PhrasePairTotalLengthBin(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 1)
      error(ETFatal, "expected one argument value for PhrasePairTotalLengthBin");
   len = conv<Uint>(toks[0]);
}

LexUnal::LexUnal(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   if (m.subvoc_map.find(args) != m.subvoc_map.end())
      lex = m.subvoc_map[args];    // file was already read in
   else
      lex = m.subvoc_map[args] = m.readSubVoc(args);
}

void LexUnalTgtAny::addEvents(const PartialTranslation& hyp, 
                              vector<Uint>& events) 
{
   if (!hyp.lastPhrase) return;

   const PhraseInfo* fbpi = hyp.lastPhrase;
   assert(fbpi);
   const Uint src_len = fbpi->src_words.size();
   const Uint tgt_len = fbpi->phrase.size();
   const AlignmentAnnotation* const a_ann = AlignmentAnnotation::get(fbpi->annotations);
   const Uint alignment = AlignmentAnnotation::getID(a_ann);


   Cache::iterator res = cache.find(CacheKey(src_len, tgt_len, alignment));
   if (res == cache.end()) {

      // parse the alignment string to a set-based representation (static to
      // cache results in case of re-processing by subsequent unal templates)

      const vector<vector<Uint > >* sets = AlignmentAnnotation::getAlignmentSets
         (alignment,src_len);

      // get link status for all target tokens (assume all are linked if
      // alignment is empty)

      static vector<bool> tgt_found;
      if (sets->empty()) {
         tgt_found.assign(tgt_len+1, true);
      } else {
         tgt_found.assign(tgt_len+1, false);
         for (Uint i = 0; i < src_len; ++i)
            for (vector<Uint>::const_iterator p = (*sets)[i].begin(); p != (*sets)[i].end(); ++p)
               tgt_found[*p] = true;
      }

      // create cache entry (list of unal positions) for this
      // src_len,tgt_len,alignment triple

      res = cache.insert(make_pair(CacheKey(src_len, tgt_len, alignment), 
                                   vector<Uint>())).first;
      vector<Uint>& unals = res->second;
      for (Uint i = 0; i < tgt_len; ++i)
         if (!tgt_found[i]) 
            unals.push_back(i);
   }

   // list of unal positions -> event vector

   vector<Uint>& unals = res->second;

   const Uint ne = events.size();
   for (Uint i = 0; i < unals.size(); ++i) {
       map<Uint,Uint>::iterator r = lex->find(fbpi->phrase[unals[i]]);
       if (r != lex->end() &&
           find(events.begin()+ne, events.end(), r->second) == events.end())
          events.push_back(r->second); // event index
   }
}

void LexUnalSrcAny::addEvents(const PartialTranslation& hyp, 
                              vector<Uint>& events) 
{
   if (!hyp.lastPhrase) return;

   const PhraseInfo* fbpi = hyp.lastPhrase;
   assert(fbpi);
   const Uint src_len = fbpi->src_words.size();
   const Uint tgt_len = fbpi->phrase.size();
   const AlignmentAnnotation* const a_ann = AlignmentAnnotation::get(fbpi->annotations);
   const Uint alignment = AlignmentAnnotation::getID(a_ann);

   Cache::iterator res = cache.find(CacheKey(src_len, tgt_len, alignment));
   if (res == cache.end()) {

      // parse the alignment string to a set-based representation (static to
      // cache results in case of re-processing by subsequent unal templates)

      const vector<vector<Uint > > *sets = AlignmentAnnotation::getAlignmentSets
         (alignment,src_len);

      // get link status for all src tokens (assume all are linked if
      // alignment is empty), and record in cache entry

      res = cache.insert(make_pair(CacheKey(src_len, tgt_len, alignment), 
                                   vector<Uint>())).first;
      vector<Uint>& unals = res->second;
      if (!sets->empty())
         for (Uint i = 0; i < src_len; ++i)
            if ((*sets)[i].empty() || (*sets)[i][0] == tgt_len)
               unals.push_back(i);
   }

   // list of unal positions -> event vector

   vector<Uint>& unals = res->second;
   const Uint ne = events.size();
   for (Uint i = 0; i < unals.size(); ++i) {
      const Uint id = m.subVocId(lex, m.srcsent[fbpi->src_words.start + unals[i]]);
      if (id < lex->size())
         events.push_back(id);
   }

   // de-duplicate
   sort(events.begin()+ne, events.end());
   vector<Uint>::iterator p = unique(events.begin()+ne, events.end());
   events.erase(p, events.end());
}

AlignedWordPair::AlignedWordPair(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 2)
      error(ETFatal, "AlignedWordPair args must be 'srcword-file tgtword-file'");
   if (m.subvoc_map.find(toks[0]) != m.subvoc_map.end())
      src_lex = m.subvoc_map[toks[0]];
   else
      src_lex = m.subvoc_map[toks[0]] = m.readSubVoc(toks[0]);

   if (m.subvoc_map.find(toks[1]) != m.subvoc_map.end())
      tgt_lex = m.subvoc_map[toks[1]];
   else
      tgt_lex = m.subvoc_map[toks[1]] = m.readSubVoc(toks[1]);
}

void AlignedWordPair::addEvents(const PartialTranslation& hyp, 
                                vector<Uint>& events)
{
   if (!hyp.lastPhrase) return;

   const PhraseInfo* fbpi = hyp.lastPhrase;
   assert(fbpi);
   const Uint src_len = fbpi->src_words.size();
   const Uint tgt_len = fbpi->phrase.size();
   const AlignmentAnnotation* const a_ann = AlignmentAnnotation::get(fbpi->annotations);
   const Uint alignment = AlignmentAnnotation::getID(a_ann);

   Cache::iterator res = cache.find(CacheKey(src_len, tgt_len, alignment));
   if (res == cache.end()) {

      // parse the alignment string to a set-based representation (static to
      // cache results in case of re-processing by subsequent unal templates)

      const vector<vector<Uint > >* sets = 
         AlignmentAnnotation::getAlignmentSets(alignment,src_len);

      // create a cache entry indicating 1-1 connections

      res = cache.insert(make_pair(CacheKey(src_len, tgt_len, alignment), 
                                   vector<Uint>())).first;
      vector<Uint>& tgt_connections = res->second;
      if (sets->empty())        // no alignment: leave connections vector empty
         return;
      tgt_connections.resize(tgt_len, src_len); // src_len means no 1-1 link
      for (Uint i = 0; i < sets->size(); ++i)
         if ((*sets)[i].size() == 1 && (*sets)[i][0] < tgt_len) {
            const Uint tpos = (*sets)[i][0];
            if (tgt_connections[tpos] == src_len) 
               tgt_connections[tpos] = i; // potential 1-1 connection
            else if (tgt_connections[tpos] < src_len)
               tgt_connections[tpos] = src_len+1; // invalidate prev connection
         }
   }
   vector<Uint>& tgt_connections = res->second;
   if (tgt_connections.empty())  // no alignment, so don't fire
      return;

   // add events for all 1-1 connections as well as tgt words in subvoc but
   // without 1-1 connections

   const Uint ne = events.size();
   vector<bool> src_connections(src_len, false);
   for (Uint i = 0; i < tgt_connections.size(); ++i) {
      const Uint tid = m.subVocId(tgt_lex, fbpi->phrase[i]);
      if (tgt_connections[i] < src_len) { // have 1-1 connection
         src_connections[tgt_connections[i]] = true;
         const Uint spos = fbpi->src_words.start + tgt_connections[i];
         const Uint sid = m.subVocId(src_lex, m.srcsent[spos]);
         if (tid < tgt_lex->size() || sid < src_lex->size())
            events.push_back(sid * (tgt_lex->size()+2) + tid);
      } else                    // no 1-1 connection for this tgt word
         if (tid < tgt_lex->size())  // word is in subvoc 
            events.push_back((src_lex->size()+1) * (tgt_lex->size()+2) + tid);
   }
   // add events for src words in subvoc but without 1-1 connections

   for (Uint i = 0; i < src_connections.size(); ++i)
      if (!src_connections[i]) {  // no 1-1 connection
         const Uint sid = m.subVocId(src_lex, m.srcsent[fbpi->src_words.start + i]);
         if (sid < src_lex->size())
            events.push_back(sid * (tgt_lex->size()+2) + tgt_lex->size() + 1);
      }

   // remove duplicates (allowing duplicate link events probably makes sense,
   // but would cause problems for the current event->feature mechanism)

   sort(events.begin()+ne, events.end());
   vector<Uint>::iterator p = unique(events.begin()+ne, events.end());
   events.erase(p, events.end());
}

string AlignedWordPair::eventDesc(Uint e) 
{
   const Uint sid = e / (tgt_lex->size()+2);
   const Uint tid = e % (tgt_lex->size()+2);
   const string src = sid < src_lex->size() ? m.voc->word(m.vocId(src_lex, sid)) :
      sid == src_lex->size() ? "OTHER" : "NOT_1-1";
   const string tgt = tid < tgt_lex->size() ? m.voc->word(m.vocId(tgt_lex, tid)) :
      tid == tgt_lex->size() ? "OTHER" : "NOT_1-1";
   return src + ":" + tgt;
}

void PhrasePairCountBin::parseBinSpec(const string& spec, Uint& minfreq, Uint& maxfreq, 
                                      bool allow_0max)
{
   vector<string> toks;
   if (split(spec, toks) != 2)
      error(ETFatal, "expected 2 argument values for frequency bin specification");
   minfreq = conv<Uint>(toks[0]);
   maxfreq = conv<Uint>(toks[1]);

   if (minfreq > maxfreq && (maxfreq || !allow_0max))
      error(ETFatal, "empty range for frequency bin specification: [%d,%d]", 
            minfreq, maxfreq);
}

PhrasePairCountBin::PhrasePairCountBin(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   parseBinSpec(args, minfreq, maxfreq);
}

void PhrasePairCountBin::addEvents(const PartialTranslation& hyp, 
                                   vector<Uint>& event_ids)
{
   const CountAnnotation* const count = CountAnnotation::get(hyp.lastPhrase->annotations);
   if (!count || count->joint_counts.empty())  // allow this, since it could have come from a rule
      return;
   if (count->joint_counts[0] >= minfreq && (maxfreq == 0 || count->joint_counts[0] <= maxfreq))
      event_ids.push_back(0);
}

PhrasePairCountMultiBin::PhrasePairCountMultiBin(SparseModel& m, const string& args) :
   EventTemplate(m, args), num_bins(0), have_high_bin(false)
{
   Uint highest_maxfreq = 0;
   vector<string> toks;
   split(args, toks);   // use 1st token as filename, as favour to derived classes

   const string fullname = adjustRelativePath(m.path, toks[0]);
   iSafeMagicStream is(fullname);   // read bins file
   string line;
   Uint minfreq, maxfreq;
   while (getline(is, line)) {
      PhrasePairCountBin::parseBinSpec(line, minfreq, maxfreq, true);
      if (minfreq > 0 && maxfreq == 0) {       // special case
         if (have_high_bin)
            error(ETFatal, 
                  "PhrasePairCountMultiBin can only have one open-ended bin");
         have_high_bin = true;
         high_bin_threshold = minfreq;
         high_bin = num_bins;
      } else {
         for (Uint i = minfreq; i <= maxfreq; ++i) {
            if (freqmap.find(i) != freqmap.end())
               error(ETFatal, "PhrasePairCountMultiBin ranges can't overlap");
            freqmap[i] = num_bins;
         }
         if (maxfreq > highest_maxfreq) highest_maxfreq = maxfreq;
      }
      ++num_bins;
   }
   if (have_high_bin && high_bin_threshold <= highest_maxfreq)
      error(ETFatal, 
            "PhrasePairCountMultiBin open-ended bin overlaps with some other bin");
}

void PhrasePairCountMultiBin::addEvents(const PartialTranslation& hyp, 
                                        vector<Uint>& event_ids)
{
   const CountAnnotation* const count = CountAnnotation::get(hyp.lastPhrase->annotations);
   if (!count || count->joint_counts.empty())  // allow this, since it could have come from a rule
      return;
   if (freqmap.find(count->joint_counts[0]) != freqmap.end())
      event_ids.push_back(freqmap[count->joint_counts[0]]);
   else if (have_high_bin && count->joint_counts[0] >= high_bin_threshold)
      event_ids.push_back(high_bin);
}

void PhrasePairCountMultiBin::populate(vector<Uint>& event_ids) 
{
   for (Uint i = 0; i < num_bins; ++i)
      event_ids.push_back(i);
}

string PhrasePairCountMultiBin::eventDesc(Uint e)
{
   ostringstream tmpstr;
   tmpstr << e;
   const string ret = "freq bin " + tmpstr.str();
   return ret;
}

PhrasePairCountsMultiBin::PhrasePairCountsMultiBin(SparseModel& m, const string& args) :
   PhrasePairCountMultiBin(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 3)
      error(ETFatal, "Expecting 'binfile b e' arguments in PhrasePairCountsMultiBin");
   beg_col = conv<Uint>(toks[1]);
   end_col = conv<Uint>(toks[2]);
   if (beg_col < 1 || end_col < beg_col)
      error(ETFatal, "Problems with 'b e' arguments in PhrasePairCountsMultiBin");
   --beg_col;  // convert to 0-based
   --end_col;
   num_bins = PhrasePairCountMultiBin::num_bins * (end_col-beg_col+1);
}

void PhrasePairCountsMultiBin::addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)
{
   const CountAnnotation* const count = CountAnnotation::get(hyp.lastPhrase->annotations);
   if (!count || count->joint_counts.empty())  // allow this, since it could have come from a rule
      return;

   Uint col_base = beg_col * PhrasePairCountMultiBin::num_bins;
   for (Uint i = beg_col; i <= end_col; ++i) {
      if (i >= count->joint_counts.size()) continue;  // what the heck
      const Uint c = Uint(count->joint_counts[i]);
      if (freqmap.find(c) != freqmap.end())
         event_ids.push_back(col_base + freqmap[c]);
      else if (have_high_bin && c >= high_bin_threshold)
         event_ids.push_back(col_base + high_bin);
      col_base += PhrasePairCountMultiBin::num_bins;
   }
}

void PhrasePairCountsMultiBin::populate(vector<Uint>& event_ids)
{
   for (Uint i = 0; i < num_bins; ++i)
      event_ids.push_back(i);
}

string PhrasePairCountsMultiBin::eventDesc(Uint e)
{
   const Uint col = e / PhrasePairCountMultiBin::num_bins;
   const Uint bin = e % PhrasePairCountMultiBin::num_bins;
   ostringstream tmpstr;
   tmpstr << "col=" << col+1 << " bin=" << bin;
   return tmpstr.str();
}

RarestTgtWordCountBin::RarestTgtWordCountBin(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 2)
      error(ETFatal, "Expecting counts and bins files in RarestTgtWordCountBin argument");
   
   string fullname = adjustRelativePath(m.path, toks[1]);
   iSafeMagicStream binsfile(fullname);
   vector< pair<Uint,Uint> > bins;
   string line;
   while (getline(binsfile, line)) {
      bins.push_back(make_pair(0, 0));
      PhrasePairCountBin::parseBinSpec(line, bins.back().first, bins.back().second);
   }
   num_bins = bins.size();
   sort(bins.begin(), bins.end());
   Uint high = 0;
   for (Uint i = 0; i < bins.size(); ++i) {
      if (high && bins[i].first <= high)
         error(ETFatal, "RarestTgtWordCountBin bin [%d,%d] overlaps with previous",
               bins[i].first, bins[i].second);
      high = bins[i].second;
   }

   fullname = adjustRelativePath(m.path, toks[0]);
   iSafeMagicStream vocfile(fullname);
   while (getline(vocfile, line)) {
      if (splitZ(line, toks) != 2)
         error(ETFatal, "RarestTgtWordCountBin format error in voc file line:\n%s",
               line.c_str());
      const Uint count = conv<Uint>(toks[1]);
      for (Uint i = 0; i < bins.size(); ++i)
         if (count >= bins[i].first && (bins[i].second == 0 || count <= bins[i].second)) {
            binmap[m.voc->add(toks[0].c_str())] = i;
            break;
         }
   }
}

void RarestTgtWordCountBin::addEvents(const PartialTranslation& hyp, vector<Uint>& events)
{
   const PhraseInfo *p = hyp.lastPhrase;
   Uint low_bin = num_bins;
   for (Uint i = 0; i < p->phrase.size(); ++i) {
      unordered_map<Uint,Uint>::const_iterator b = binmap.find(p->phrase[i]);
      if (b != binmap.end() && b->second < low_bin)
         low_bin = b->second;
   }
   if (low_bin != num_bins)
      events.push_back(low_bin);
}

void RarestTgtWordCountBin::populate(vector<Uint>& event_ids)
{
   for (Uint i = 0; i < num_bins; ++i)
      event_ids.push_back(i);
}

// not cheap
void RarestTgtWordCountBin::replaceVoc(Voc* newvoc)
{
   unordered_map<Uint,Uint> old_binmap(binmap);
   binmap.clear();
   for (Uint i = 0; i < m.voc->size(); ++i)
      binmap[newvoc->add(m.voc->word(i))] = old_binmap[i];
}

string RarestTgtWordCountBin::eventDesc(Uint e)
{
   ostringstream tmpstr;
   tmpstr << e;
   const string ret = "freq bin " + tmpstr.str();
   return ret;
}

AvgTgtWordScoreBin::AvgTgtWordScoreBin(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 3)
      error(ETFatal, "Expecting 'scores numbins method' in AvgTgtWordScore argument");

   num_bins = conv<Uint>(toks[1]);
   if (toks[2] == "lin") linear_bins = true;
   else if (toks[2] == "log") linear_bins = false;
   else
      error(ETFatal, "method argument to AvgTgtWordScoreBin must be either 'lin' or 'log'");

   double maxscore = -numeric_limits<double>::max();
   minscore = numeric_limits<double>::max();
   string line;
   const string fullname = adjustRelativePath(m.path, toks[0]);
   iSafeMagicStream vocfile(fullname);
   while (getline(vocfile, line)) {
      if (splitZ(line, toks) != 2)
         error(ETFatal, "AvgTgtWordScoreBin format error in voc file line:\n%s",
               line.c_str());
      const float score = conv<float>(toks[1]);
      binmap[m.voc->add(toks[0].c_str())] = score;
      if (!linear_bins && score < 0.0)
         error(ETFatal, "AvgTgtWordScoreBin scores must be > 0 for log binning");
      if (score > maxscore) maxscore = score;
      if (score < minscore) minscore = score;
   }
   if (!linear_bins) {
      maxscore = log(maxscore);
      minscore = log(minscore);
   }
   range = maxscore - minscore;
   range += range / 10000.0;    // want maxscore/range to be slightly < 1
}

void AvgTgtWordScoreBin::addEvents(const PartialTranslation& hyp, vector<Uint>& events)
{
   const PhraseInfo* const p = hyp.lastPhrase;
   double avg = 0.0;
   Uint nwords = 0;
   for (Uint i = 0; i < p->phrase.size(); ++i) {
      unordered_map<Uint,float>::const_iterator b = binmap.find(p->phrase[i]);
      if (b != binmap.end()) {
         ++nwords;
         avg += b->second;
      }
   }
   if (nwords) {
      avg /= nwords;
      if (!linear_bins) avg = log(avg);
      const Uint bin = floor(num_bins * (avg - minscore)/range);
      assert(bin < num_bins);
      events.push_back(bin);
   }
}

void AvgTgtWordScoreBin::populate(vector<Uint>& event_ids)
{
   for (Uint i = 0; i < num_bins; ++i)
      event_ids.push_back(i);
}

// not cheap
void AvgTgtWordScoreBin::replaceVoc(Voc* newvoc)
{
   unordered_map<Uint,float> old_binmap(binmap);
   binmap.clear();
   for (Uint i = 0; i < m.voc->size(); ++i)
      binmap[newvoc->add(m.voc->word(i))] = old_binmap[i];
}

string AvgTgtWordScoreBin::eventDesc(Uint e)
{
   ostringstream tmpstr;
   tmpstr << e;
   const string ret = "freq bin " + tmpstr.str();
   return ret;
}

void MinTgtWordScoreBin::addEvents(const PartialTranslation& hyp, vector<Uint>& events)
{
   const PhraseInfo* const p = hyp.lastPhrase;
   double ms = 0.0;
   Uint nwords = 0;
   for (Uint i = 0; i < p->phrase.size(); ++i) {
      unordered_map<Uint,float>::const_iterator b = binmap.find(p->phrase[i]);
      if (b != binmap.end()) {
         if (nwords == 0 || b->second < ms) ms = b->second;
         ++nwords;
      }
   }
   if (nwords) {
      if (!linear_bins) ms = log(ms);
      const Uint bin = floor(num_bins * (ms - minscore)/range);
      assert(bin < num_bins);
      events.push_back(bin);
   }
}

// EventTemplateWithClusters
EventTemplateWithClusters::EventTemplateWithClusters(SparseModel& m, const string& args, const string& cname) :
   EventTemplate(m,args)
{
   vector<string> toks;
   if (split(args, toks) < 1)
      error(ETFatal, cname + " args must be 'clust-file'");

   // Read in our cluster files, if necessary
   if (m.file_to_clust.find(toks[0]) != m.file_to_clust.end())
      clusters = m.file_to_clust[toks[0]];
   else
      clusters = m.file_to_clust[toks[0]] = new SparseModel::ClusterMap(m,toks[0]);
}

void EventTemplateWithClusters::populate(vector<Uint>& event_ids)
{
   for(Uint i=0; i< numEvents(); ++i) event_ids.push_back(i);
}

void EventTemplateWithClusters::replaceVoc(Voc* new_voc)
{
   clusters->replaceVoc(m.voc, new_voc);
}

// DiscriminativeDistortion base class

DiscriminativeDistortion::DiscriminativeDistortion(SparseModel& m, const string& args, const string& cname) :
   EventTemplateWithClusters(m, args, cname)
{
   vector<string> toks;
   if (split(args, toks) < 1)
      error(ETFatal, cname + " args must be 'clust-file LR?'");

   // Populate the distortion checker
   if (std::find(toks.begin()+1, toks.end(), "LR") != toks.end())
      dist = FwdLRSplitHierDistortion::instance(this);
   else
      dist = FwdHierDistortion::instance(this);
}

// DistCurrentSrcPhrase

DistCurrentSrcPhrase::DistCurrentSrcPhrase(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m, args, name()) {}

void DistCurrentSrcPhrase::addEvents(const PartialTranslation& pt,
                                           vector<Uint>& event_ids)
{
   const PhraseInfo *p = pt.lastPhrase;
   assert(p!=NULL);
   const Range r = p->src_words;
   const Uint phrase = clusters->phraseId(m.srcsent[r.start],
                                    m.srcsent[r.end-1],
                                    r.end-r.start);

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( phrase + (*type)*clusters->numPhraseIds() );
   }
}

Uint DistCurrentSrcPhrase::numEvents()
{
   return dist->numTypes() * clusters->numPhraseIds();
}

string DistCurrentSrcPhrase::eventDesc(Uint e)
{
   const Uint type = e / clusters->numPhraseIds();
   const Uint phrase = e % clusters->numPhraseIds();

   const string sType = dist->typeString(type);   
   return "dhdm src " + sType + " : " + clusters->phraseStr(phrase);
}

// DistCurrentSrcFirstWord

DistCurrentSrcFirstWord::DistCurrentSrcFirstWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistCurrentSrcFirstWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistCurrentSrcFirstWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   const PhraseInfo* const p = pt.lastPhrase;
   assert(p!=NULL);
   const Range r = p->src_words;
   const Uint word = clusters->clusterId(m.srcsent[r.start]);

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistCurrentSrcFirstWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm src " + sType + " : [" + tmpstr.str() + "..]";
}

// DistCurrentSrcLastWord

DistCurrentSrcLastWord::DistCurrentSrcLastWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistCurrentSrcLastWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistCurrentSrcLastWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   const PhraseInfo* const p = pt.lastPhrase;
   assert(p!=NULL);
   const Range r = p->src_words;
   const Uint word = clusters->clusterId(m.srcsent[r.end-1]);

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistCurrentSrcLastWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm src " + sType + " : [.." + tmpstr.str() + "]";
}

// DistCurrentTgtFirstWord

DistCurrentTgtFirstWord::DistCurrentTgtFirstWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistCurrentTgtFirstWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistCurrentTgtFirstWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   const PhraseInfo* const p = pt.lastPhrase;
   assert(p!=NULL);
   const Phrase phrase = p->phrase;
   const Uint word = clusters->clusterId(phrase.front());

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistCurrentTgtFirstWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm tgt " + sType + " : [" + tmpstr.str() + "..]";
}

// DistCurrentTgtLastWord

DistCurrentTgtLastWord::DistCurrentTgtLastWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistCurrentTgtLastWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistCurrentTgtLastWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   const PhraseInfo* const p = pt.lastPhrase;
   assert(p!=NULL);
   const Phrase phrase = p->phrase;
   const Uint word = clusters->clusterId(phrase.back());

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistCurrentTgtLastWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm tgt " + sType + " : [.." + tmpstr.str() + "]";
}

// DistReducedSrcFirstWord

DistReducedSrcFirstWord::DistReducedSrcFirstWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistReducedSrcFirstWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistReducedSrcFirstWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   // Input
   const Uint ne = event_ids.size();
   Range iTop(pt.lastPhrase->src_words.start,
              pt.lastPhrase->src_words.end);
   ShiftReducer* iTail=pt.back->shiftReduce;
   // Output
   Range oTop;
   ShiftReducer* oTail;
   // Try a fake reduce, see if anything happens
   ShiftReducer::fake_reduce(iTop,iTail,oTop,oTail);
   while(iTop!=oTop)
   {
      // Generate the features
      const Uint word = clusters->clusterId(m.srcsent[oTop.start]);
      const vector<Uint>& type_ids = dist->type(oTop,oTail);
      for(vector<Uint>::const_iterator type=type_ids.begin();
          type!=type_ids.end();
          type++)
      {
         event_ids.push_back( word + (*type)*clusters->numClustIds() );
      }

      // Reduce again and loop
      iTop.start=oTop.start; iTop.end=oTop.end; iTail=oTail;
      ShiftReducer::fake_reduce(iTop,iTail,oTop,oTail);
   }

   // Delete duplicates to avoid problems with event->feature
   sort(event_ids.begin()+ne, event_ids.end());
   vector<Uint>::iterator p = unique(event_ids.begin()+ne, event_ids.end());
   event_ids.erase(p, event_ids.end());
}

string DistReducedSrcFirstWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm red src " + sType + " : [" + tmpstr.str() + "..]";
}

// DistReducedSrcLastWord

DistReducedSrcLastWord::DistReducedSrcLastWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistReducedSrcLastWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistReducedSrcLastWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   // Input
   const Uint ne = event_ids.size();
   Range iTop(pt.lastPhrase->src_words.start,
              pt.lastPhrase->src_words.end);
   ShiftReducer* iTail=pt.back->shiftReduce;
   // Output
   Range oTop;
   ShiftReducer* oTail;
   // Try a fake reduce, see if anything happens
   ShiftReducer::fake_reduce(iTop,iTail,oTop,oTail);
   while(iTop!=oTop)
   {
      // Generate the features
      const Uint word = clusters->clusterId(m.srcsent[oTop.end-1]);
      const vector<Uint>& type_ids = dist->type(oTop,oTail);
      for(vector<Uint>::const_iterator type=type_ids.begin();
          type!=type_ids.end();
          type++)
      {
         event_ids.push_back( word + (*type)*clusters->numClustIds() );
      }

      // Reduce again and loop
      iTop.start=oTop.start; iTop.end=oTop.end; iTail=oTail;
      ShiftReducer::fake_reduce(iTop,iTail,oTop,oTail);
   }

   // Delete duplicates to avoid problems with event->feature
   sort(event_ids.begin()+ne, event_ids.end());
   vector<Uint>::iterator p = unique(event_ids.begin()+ne, event_ids.end());
   event_ids.erase(p, event_ids.end());
}

string DistReducedSrcLastWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm red src " + sType + " : [.." + tmpstr.str() + "]";
}

// DistReducedTgtLastWord

DistReducedTgtLastWord::DistReducedTgtLastWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistReducedTgtLastWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistReducedTgtLastWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   const Uint ne = event_ids.size();
   // No matter how big the block gets, it always has the same final tgt word
   const Uint word = clusters->clusterId(pt.lastPhrase->phrase.back());

   // Input
   Range iTop(pt.lastPhrase->src_words.start,
              pt.lastPhrase->src_words.end);
   ShiftReducer* iTail=pt.back->shiftReduce;
   // Output
   Range oTop;
   ShiftReducer* oTail;
   // Try a fake reduce, see if anything happens
   ShiftReducer::fake_reduce(iTop,iTail,oTop,oTail);
   while(iTop!=oTop)
   {
      // Generate the features
      const vector<Uint>& type_ids = dist->type(oTop,oTail);
      for(vector<Uint>::const_iterator type=type_ids.begin();
          type!=type_ids.end();
          type++)
      {
         event_ids.push_back( word + (*type)*clusters->numClustIds() );
      }

      // Reduce again and loop
      iTop.start=oTop.start; iTop.end=oTop.end; iTail=oTail;
      ShiftReducer::fake_reduce(iTop,iTail,oTop,oTail);
   }

   // Delete duplicates to avoid problems with event->feature
   sort(event_ids.begin()+ne, event_ids.end());
   vector<Uint>::iterator p = unique(event_ids.begin()+ne, event_ids.end());
   event_ids.erase(p, event_ids.end());
}

string DistReducedTgtLastWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm red tgt " + sType + " : [.." + tmpstr.str() + "]";
}

// DistGapSrcWord

DistGapSrcWord::DistGapSrcWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistGapSrcWord::numEvents()
{
   return 2 * clusters->numClustIds();
}

void DistGapSrcWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   // Add a feature for every word between the top of the stack and the
   // current phrase
   
   Uint iLeft = 1;  // Start point (inclusive)
   Uint iRight = 0; // End point (exclusive)
   Uint iCode = 2;
   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   if (pt.lastPhrase->src_words.start > pt.back->shiftReduce->end())
   {
      // Disjoint to the right
      iLeft = pt.back->shiftReduce->end();
      iRight = pt.lastPhrase->src_words.start;
      assert(iLeft < iRight);
      iCode = 0;
   }
   else if (pt.lastPhrase->src_words.end < pt.back->shiftReduce->start())
   {
      // Disjoint to the left
      iLeft = pt.lastPhrase->src_words.end;
      iRight = pt.back->shiftReduce->start();
      assert(iLeft < iRight);
      iCode = 1;
   }

   // Disjoint at all
   const Uint ne = event_ids.size();
   if(iLeft < iRight) {
      assert(iCode < 2);
      for(Uint iSrc = iLeft; iSrc < iRight; iSrc++) {
         const Uint word = clusters->clusterId(m.srcsent[iSrc]);
         event_ids.push_back( word + iCode * clusters->numClustIds() );
      }
   }

   // Delete duplicates to avoid problems with event->feature
   sort(event_ids.begin()+ne, event_ids.end());
   vector<Uint>::iterator p = unique(event_ids.begin()+ne, event_ids.end());
   event_ids.erase(p, event_ids.end());
}

string DistGapSrcWord::eventDesc(Uint e)
{
   const Uint code = e / clusters->numClustIds();
   assert(code < 2);
   const Uint word = e % clusters->numClustIds();
   ostringstream tmpstr;
   tmpstr << word;
   if(code==0)
      return "dhdm gap r : " + tmpstr.str();
   else
      return "dhdm gap l : " + tmpstr.str();
}

// DistTopSrcFirstWord

DistTopSrcFirstWord::DistTopSrcFirstWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistTopSrcFirstWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistTopSrcFirstWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   const Uint word = 1; // Default to Beginning of Sentence
   if(pt.back->shiftReduce->end()!=0)
      // TODO: Why to we get the clusterid if we don't use it?
      // clusterId has no set effect.
      clusters->clusterId(m.srcsent[pt.back->shiftReduce->start()]);

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistTopSrcFirstWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm top src " + sType + " : [" + tmpstr.str() + "..]";
}

// DistTopSrcLastWord

DistTopSrcLastWord::DistTopSrcLastWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistTopSrcLastWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistTopSrcLastWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   if (pt.back->shiftReduce == NULL)
      error(ETFatal, "you need to specify -force-shift-reduce or use an hldm before using the DHDM sparse features");
   const Uint word = 1; // Default to Beginning of Sentence
   if(pt.back->shiftReduce->end()!=0)
      // TODO: Why call clusterId?
      clusters->clusterId(m.srcsent[pt.back->shiftReduce->end()-1]);

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistTopSrcLastWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm top src " + sType + " : [.." + tmpstr.str() + "]";
}

// DistPrevTgtWord

DistPrevTgtWord::DistPrevTgtWord(SparseModel& m, const string& args)
   : DiscriminativeDistortion(m,args,name()) {}

Uint DistPrevTgtWord::numEvents()
{
   return dist->numTypes() * clusters->numClustIds();
}

void DistPrevTgtWord::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   VectorPhrase unigram;
   pt.getLastWords(unigram,1);
   assert(unigram.size()<=1);
   Uint word = 1; // Default to beginning of sentence
   if(unigram.size()==1)
      // TODO: are we supposed to do this word = at the other similar places.
      word = clusters->clusterId(unigram.front());

   const vector<Uint>& type_ids = dist->type(pt);
   for(vector<Uint>::const_iterator type=type_ids.begin();
       type!=type_ids.end();
       type++)
   {
      event_ids.push_back( word + (*type)*clusters->numClustIds() );
   }
}

string DistPrevTgtWord::eventDesc(Uint e)
{
   const Uint type = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   const string sType = dist->typeString(type);
   ostringstream tmpstr;
   tmpstr << word;
   return "dhdm prev tgt " + sType + " : " + tmpstr.str();
}

// LmUnigram

LmUnigram::LmUnigram(SparseModel& m, const string& args)
   : EventTemplateWithClusters(m,args,name()) {}

Uint LmUnigram::numEvents()
{
   return clusters->numClustIds();
}

void LmUnigram::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   // Walk through target phrase
   const Uint ne = event_ids.size();
   const PhraseInfo *pi = pt.lastPhrase;
   assert(pi!=NULL);
   const Phrase phrase = pi->phrase;
   for(vector<Uint>::const_iterator it = phrase.begin();it!=phrase.end();it++)
   {
      const Uint word = clusters->clusterId(*it);
      event_ids.push_back( word );
   }

   // Delete duplicates to avoid problems with event->feature
   sort(event_ids.begin()+ne, event_ids.end());
   vector<Uint>::iterator p = unique(event_ids.begin()+ne, event_ids.end());
   event_ids.erase(p, event_ids.end());
}

string LmUnigram::eventDesc(Uint e)
{
   assert(e<clusters->numClustIds());
   const Uint word = e;
   ostringstream tmpstr;
   tmpstr << word;
   return "dlm uni: " + tmpstr.str();
}

// LmBigram

LmBigram::LmBigram(SparseModel& m, const string& args)
   : EventTemplateWithClusters(m,args,name()) {}

Uint LmBigram::numEvents()
{
   return clusters->numClustIds() * clusters->numClustIds();
}

void LmBigram::addEvents(const PartialTranslation& pt, vector<Uint>& event_ids)
{
   // Get previous word
   VectorPhrase prev;
   pt.getLastWords(prev,1);
   assert(prev.size()<=1);
   Uint prevWord = 1; // Default to beginning of sentence
   if(prev.size()==1)
      prevWord = clusters->clusterId(prev.front());
   
   // Walk through target phrase
   const Uint ne = event_ids.size();
   const PhraseInfo* const pi = pt.lastPhrase;
   assert(pi!=NULL);
   const Phrase phrase = pi->phrase;
   for(vector<Uint>::const_iterator it = phrase.begin();it!=phrase.end();it++)
   {
      const Uint word = clusters->clusterId(*it);
      event_ids.push_back( word + clusters->numClustIds() * prevWord );
      prevWord = word;
   }

   // Delete duplicates to avoid problems with event->feature
   sort(event_ids.begin()+ne, event_ids.end());
   vector<Uint>::iterator p = unique(event_ids.begin()+ne, event_ids.end());
   event_ids.erase(p, event_ids.end());
}

string LmBigram::eventDesc(Uint e)
{
   const Uint prevWord = e / clusters->numClustIds();
   const Uint word = e % clusters->numClustIds();
   assert(prevWord < clusters->numClustIds());
   ostringstream tmpstr;
   tmpstr << prevWord << " " << word;
   return "dlm bi: " + tmpstr.str();
}

// PhrasePairAllBiLengthBins

PhrasePairAllBiLengthBins::PhrasePairAllBiLengthBins(SparseModel& m, const string& args)
   : EventTemplate(m,args)
{
   vector<string> toks;
   if (split(args, toks) != 2)
      error(ETFatal, "expected 2 argument values for PhrasePairBiLengthBins args");
   srcmax = conv<Uint>(toks[0]);
   tgtmax = conv<Uint>(toks[1]);

   if (srcmax == 0)
      error(ETFatal, "PhrasePairAllBiLengthBins max source length should be at least 1");
   if (tgtmax==0)
      error(ETFatal, "PhrasePairAllBiLengthBins max target length should be at least 1");
}

void PhrasePairAllBiLengthBins::addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)
{
   if (hyp.lastPhrase) {
      const Uint slen = (hyp.lastPhrase->src_words.end - hyp.lastPhrase->src_words.start)-1;
      const Uint tlen = hyp.lastPhrase->phrase.size()-1;
      if (slen < srcmax && tlen < tgtmax)
         event_ids.push_back(tlen + slen * tgtmax);
   }
}

void PhrasePairAllBiLengthBins::populate(vector<Uint>& event_ids)
{
   for(Uint i=0; i< srcmax*tgtmax; ++i) event_ids.push_back(i);   
}

string PhrasePairAllBiLengthBins::eventDesc(Uint e)
{
   const Uint slen = (e / tgtmax)+1;
   const Uint tlen = (e % tgtmax)+1;
   assert(slen <= srcmax);
   ostringstream srcstr;
   srcstr << slen;
   ostringstream tgtstr;
   tgtstr << tlen;
   return
      "biLenBin " + srcstr.str() + " " + srcstr.str()
      + " " + tgtstr.str() + " " + tgtstr.str();
}

// PhrasePairAllTgtLengthBins

PhrasePairAllTgtLengthBins::PhrasePairAllTgtLengthBins(SparseModel& m, const string& args)
   : EventTemplate(m,args)
{
   vector<string> toks;
   if (split(args, toks) != 1)
      error(ETFatal, "expected 1 argument values for PhrasePairTgtLengthBins args");
   tgtmax = conv<Uint>(toks[0]);

   if (tgtmax==0)
      error(ETFatal, "PhrasePairAllTgtLengthBins max target length should be at least 1");
}

void PhrasePairAllTgtLengthBins::addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)
{
   if (hyp.lastPhrase) {
      const Uint tlen = hyp.lastPhrase->phrase.size()-1;
      if (tlen < tgtmax)
         event_ids.push_back(tlen);
   }
}

void PhrasePairAllTgtLengthBins::populate(vector<Uint>& event_ids)
{
   for(Uint i=0; i< tgtmax; ++i) event_ids.push_back(i);   
}

string PhrasePairAllTgtLengthBins::eventDesc(Uint e)
{
   const Uint tlen = e+1;
   assert(tlen <= tgtmax);
   ostringstream tgtstr;
   tgtstr << tlen;
   return
      "tgtLenBin " + tgtstr.str() + " " + tgtstr.str();
}

// PhrasePairAllSrcLengthBins

PhrasePairAllSrcLengthBins::PhrasePairAllSrcLengthBins(SparseModel& m, const string& args)
   : EventTemplate(m,args)
{
   vector<string> toks;
   if (split(args, toks) != 1)
      error(ETFatal, "expected 1 argument values for PhrasePairSrcLengthBins args");
   srcmax = conv<Uint>(toks[0]);

   if (srcmax == 0)
      error(ETFatal, "PhrasePairAllSrcLengthBins max source length should be at least 1");
}

void PhrasePairAllSrcLengthBins::addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)
{
   if (hyp.lastPhrase) {
      const Uint slen = (hyp.lastPhrase->src_words.end - hyp.lastPhrase->src_words.start)-1;
      if (slen < srcmax)
         event_ids.push_back(slen);
   }
}

void PhrasePairAllSrcLengthBins::populate(vector<Uint>& event_ids)
{
   for(Uint i=0; i< srcmax; ++i) event_ids.push_back(i);   
}

string PhrasePairAllSrcLengthBins::eventDesc(Uint e)
{
   const Uint slen = e + 1;
   assert(slen <= srcmax);
   ostringstream srcstr;
   srcstr << slen;
   return
      "srcLenBin " + srcstr.str() + " " + srcstr.str();
}

// InQuotePos

InQuotePos::InQuotePos(SparseModel& m, const string& args) :
   EventTemplate(m, args),
   cms("")   // use LC_CTYPE locale for case conversion
{
   Uint binarray[] = {0,0,1,2,3,3,3,3,3}; // len -> bin (0 -> for convenience)
   bins.assign(&binarray[0], &binarray[0] + ARRAY_SIZE(binarray));
   num_bins = *max_element(bins.begin(), bins.end()) + 2;
   num_sptypes = num_bins * 4;  // x start|not x has-stop|not
   num_events = num_sptypes * 3; // x lc|c1|other
}

void InQuotePos::addEvents(const PartialTranslation& hyp, vector<Uint>& events)
{
   if (srcsent != m.srcsent) {  // new source sentence

      srcsent = m.srcsent;
      src_events.assign(srcsent.size(), num_sptypes);
      
      // annotate each source word with sptype index
      for (Uint i = 0; i < srcsent.size(); ++i) {
         const string w(m.voc->word(srcsent[i]));
         if (w == "\"") {       // assume 1st quote is a left quote
            bool got_stop = false;
            Uint j = i+1;
            for (; j < srcsent.size(); ++j) {
               const string v(m.voc->word(srcsent[j]));
               if (v == "." || v == "?" || v == "!")
                  got_stop = true;
               if (v == "\"")
                  break;
            }
            if (j < srcsent.size() || got_stop) { // got a quote
               Uint base = j-i-1 < bins.size() ? bins[j-i-1] : num_bins-1;
               assert(base < num_bins);
               if (got_stop) base += num_bins;
               for (Uint k = i+1; k < j; ++k) {
                  src_events[k] = k == i+1 ? 2 * num_bins + base : base;
                  assert(src_events[k] < num_sptypes);
               }
            }
            i = j;              // skip just-analyzed section
         }
      }
   }

   const Uint len = hyp.getLength();
   const Phrase& ph = hyp.getPhrase();
   assert(len >= ph.size());
   assert(len == hyp.numSourceWordsCovered);  // truecasing is 1-1
   assert(len <= srcsent.size());

   const Uint ne = events.size();
   Uint spos = len - ph.size();
   for (Uint i = 0; i < ph.size(); ++i, ++spos) {
      assert(spos < srcsent.size());
      if (src_events[spos] == num_sptypes) continue; // not in a quote
      const string s(m.voc->word(ph[i]));
      const string l = cms.toLower(s);
      const Uint cap = s == l ? 1 : s == cms.capitalize(l) ? 1 : 2;
      events.push_back(cap * num_sptypes + src_events[spos]);
      assert(events.back() < num_events);
   }

   // de-duplicate
   sort(events.begin()+ne, events.end());
   vector<Uint>::iterator p = unique(events.begin()+ne, events.end());
   events.erase(p, events.end());
}

void InQuotePos::populate(vector<Uint>& event_ids)
{
   for (Uint i = 0; i < num_events; ++i)
      event_ids.push_back(i);
}

string InQuotePos::eventDesc(Uint e) 
{
   const Uint cap_pattern = e / num_sptypes;
   const Uint sptype = e % num_sptypes;
   
   const Uint is_first = sptype / num_bins / 2;
   const Uint has_stop = sptype / num_bins % 2;
   const Uint len_bin = sptype % num_bins;

   string r;
   r = string("cap = ") + (cap_pattern == 0 ? "lc" : cap_pattern == 1 ? "c1" : "o");
   r += string(", ") + (is_first ? "is" : "isn't") + " first";
   r += string(", ") + (has_stop ? "has" : "no") + " stop";

   ostringstream srcstr;
   srcstr << ", len bin = " << len_bin;

   return r + srcstr.str();
}

// PhrasePairContextMatch

PhrasePairContextMatch::PhrasePairContextMatch(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks;
   if (split(args, toks) != 5)
      error(ETFatal, "expecting 5 arguments for PhrasePairContextMatch feature");
   const string tagfile = toks[0];
   const string idfile = m.idname;   // used to be an argument, now comes in from m
   idcol = conv<Uint>(toks[1]);
   assert(idcol > 0);
   idcol--;                     // convert to 0-based
   field = conv<Uint>(toks[2]);
   assert(field > 0);
   field--;                     // convert to 0-based
   pweight = conv<double>(toks[3]);
   numbins = conv<Uint>(toks[4]);
   assert(numbins > 0);         // should maybe insist on 2+ bins?

   if (m.verbose)
      cerr << "PhrasePairContextMatch args:" << endl
           << "tagfile = " << tagfile << endl
           << "idfile = " << idfile << endl
           << "idcol = " << idcol+1 << endl
           << "field = " << field+1 << endl
           << "pweight = " << pweight << endl
           << "numbins = " << numbins << endl;
   
   // read tagfile into tag_voc and tag_freqs

   const string fullname = adjustRelativePath(m.path, tagfile);
   iSafeMagicStream tagstr(fullname);
   string line;
   while (getline(tagstr, line)) {
      if (splitZ(line, toks) != 2)
         error(ETFatal, "expecting 2 tokens per line in tagfile: %s", 
               tagfile.c_str());
      const Uint s = tag_voc.size();
      if (tag_voc.add(toks[0].c_str()) != s)
         error(ETFatal, "duplicate tag in tagfile: %s", toks[0].c_str());
      tag_freqs.push_back(conv<Uint>(toks[1]));
   }
   if (m.verbose)
      cerr << tag_voc.size() << " tags read from " << tagfile << endl;
   
   // read idfile, pick out selected column, and check tags

   if (idfile != "") {
      iSafeMagicStream idstr(idfile);
      bool at_least_one_tag_found = false;
      while (getline(idstr, line)) {
         splitZ(line, toks);
         if (idcol >= toks.size())
            error(ETFatal, "there is no column %d in id file line: %s",
                  idcol+1, idfile.c_str());
         sent_tags.push_back(tag_voc.index(toks[idcol].c_str()));
         if (sent_tags.back() == tag_voc.size())
            error(ETWarn, "tag %s from id file not found in tags file", 
                  toks[idcol].c_str());
         else
            at_least_one_tag_found = true;
      }
      if (!at_least_one_tag_found)
         error(ETFatal, "not a single tag from column %d of id file %s found in tags file %s%s",
               idcol+1, idfile.c_str(), tagfile.c_str(), " - looks suspicious, quitting!");
      if (m.verbose)
         cerr << sent_tags.size() << " tags read from column " 
              << idcol+1 << " of id file " << idfile << endl;
   } else
      error(ETWarn, "creating model with empty idfile");

   // set up derived quantities

   double totfreq = 0;
   for (Uint i = 0; i < tag_freqs.size(); ++i)
      totfreq += tag_freqs[i];
   prior.resize(tag_freqs.size());
   for (Uint i = 0; i < tag_freqs.size(); ++i)
      prior[i] = pweight * tag_freqs[i] / totfreq;

   bins.resize(numbins);
   for (Uint i = 0; i < numbins; ++i)
      bins[i] = (i+1.0) / numbins;
   bins[numbins-1] += 1e-02;    // end catch
}

namespace Portage {
   struct SparsePill : public PhrasePairAnnotation {
      vector< vector_map<Uint,Uint> > events;  // field -> (tag -> event)

      virtual PhrasePairAnnotation* clone() const { return new SparsePill(*this); }
      static SparsePill* getOrCreate(const AnnotationList& b, bool* is_new = NULL) {
         return PhrasePairAnnotation::getOrCreate<SparsePill>(name, b, is_new);
      }
      static const string name;
   };
   const string SparsePill::name = "sparse_pill";
}


void PhrasePairContextMatch::addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)
{
   if (sent_tags[sid] == tag_voc.size())  // no events for unk tags
      return;

   bool pill_is_new;
   SparsePill* const pill = SparsePill::getOrCreate(hyp.lastPhrase->annotations, &pill_is_new);

   // if this field,tag pair is already in pill, return the relevant event

   if (!pill_is_new && field < pill->events.size()) {
      vector_map<Uint,Uint>::iterator p = pill->events[field].find(sent_tags[sid]);
      if (p != pill->events[field].end())
         event_ids.push_back(p->second);
         return;
   }

   // calculate event if pill is new or doesn't yet contain this field,tag pair

   if (field >= pill->events.size()) pill->events.resize(field+1);
      
   // find field boundaries in joint_counts vector

   const CountAnnotation* const count = CountAnnotation::get(hyp.lastPhrase->annotations);
   const Uint joint_counts_size(count ? count->joint_counts.size() : 0);
   Uint b = joint_counts_size;  // beg of field
   Uint e = joint_counts_size;  // end+1 of field
   for (Uint i = 0; i < joint_counts_size; ++i) {
      const float v = count->joint_counts[i];
      if (v < 0) {
         if (static_cast<Uint>(-v) == field) b = i;
         else if (static_cast<Uint>(-v) > field) {e = i; break;}
      }
   }
   if (field == 0) b = 0;  // always
   else if (b < joint_counts_size) b++;

   // find current tag in field, and calculate its prob

   if (b < e) {
      assert((e-b) % 2 == 0);
      float tot = 0.0, tagfreq = 0.0;
      for (Uint i = b; i < e; i += 2) {
         const Uint t = static_cast<Uint>(count->joint_counts[i]-1);  // 1-based
         assert(t < tag_voc.size());
         if (t == sent_tags[sid]) tagfreq = count->joint_counts[i+1];
         tot += count->joint_counts[i+1];
      }
      float pr = (tagfreq + prior[sent_tags[sid]]) / (tot + pweight);

      // bin to determine event to add, then add it

      Uint bin = 0;
      for (; bin < numbins; ++bin)
         if (pr < bins[bin])
            break;
      assert(bin < numbins);
      pill->events[field][sent_tags[sid]] = bin;  // add the event to the pill
      event_ids.push_back(bin);  // add the event to the events list
   } else {
      static Uint warning_printed_count = 0;
      if (warning_printed_count < 3) {
         error(ETWarn, "field %d not found for phrase pair; num c= columns = %d", 
               field+1, joint_counts_size);
         ++warning_printed_count;
      } else if (warning_printed_count == 3) {
         error(ETWarn, "field %d not found for phrase pair; num c= columns = %d%s", 
               field+1, joint_counts_size, 
               " - suppressing further warnings about this");
         ++warning_printed_count;
      }
   }
}

string PhrasePairContextMatch::eventDesc(Uint e)
{
   ostringstream srcstr;
   srcstr << "bin " << e << ": [" 
          << (e > 0 ? bins[e-1] : 0.0) 
          << "," << bins[e] << ")";
   return srcstr.str();
}

PhrasePairSubCorpRank::PhrasePairSubCorpRank(SparseModel& m, const string& args) :
   EventTemplate(m, args)
{
   vector<string> toks, toks2;
   if (split(args, toks) != 3)
      error(ETFatal, "expecting 3 arguments for PhrasePairSubCorpRank feature");
   ncpts = conv<Uint>(toks[0]);
   col = conv<Uint>(toks[1]);
   if (col == 0)
      error(ETFatal, "column argument for PhrasePairSubCorpRank must be > 0");
   col--;
   if (split(toks[2], toks2, "-") != 2)
      error(ETFatal, "final PhrasePairSubCorpusRank argument must be lo-hi");
   low = conv<Uint>(toks2[0]);
   high = conv<Uint>(toks2[1]);
   if (low == 0 || high == 0 || low > ncpts || high > ncpts || low > high)
      error(ETFatal, "final PhrasePairSubCorpusRank argument isn't a valid range");
}

void PhrasePairSubCorpRank::addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)
{
   const CountAnnotation* const count = CountAnnotation::get(hyp.lastPhrase->annotations);

   // Allow for un-annotated phrase pairs. Some pairs wind up with the
   // equivalent of c=0, not sure why.
   if (!count || count->joint_counts.empty() ||
       (count->joint_counts.size() == 1 && count->joint_counts[0] == 0.0))
       return;

   // find [b,e) for column
   Uint ncols = 0, b = 0, e = 0;
   for (Uint i = 0; i < count->joint_counts.size(); ++i)
      if (count->joint_counts[i] == 0) {
         e = i;
         if (++ncols > col) break;
         b = i+1;
      }
   assert(ncols > col);  // else there are fewer than col+1 columns in c!

   // fire events
   for (Uint i = b+low-1; i < min(e,b+high); ++i) {
      const Uint id = count->joint_counts[i];
      assert(id > 0 && id <= ncpts);
      event_ids.push_back(id-1);
   }
}

string PhrasePairSubCorpRank::eventDesc(Uint e)
{
   ostringstream s;
   s << "col " << col+1 << " prob for cpt " << e+1 
     << " in rank range [" << low << "," << high << "]";
   return s.str();
}
