/**
 * @author George Foster
 * @file sparsemodel.h Represent large feature sets, not all of which are
 * active in a given context.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 *
 * TODO: see list at beginning of palminer.cc.
 */

#ifndef SPARSEMODEL_H
#define SPARSEMODEL_H

#include <set>
#include <boost/unordered_map.hpp>
#include "voc.h"
#include "parse_pal.h"
#include "phrase_table.h"
#include "phrasetable.h"
#include "config_io.h"
#include "basicmodel.h"
#include "inputparser.h"
#include "ngram_counts.h"
#include "unal_feature.h"
#include "casemap_strings.h"
#include "mm_map.h"

using namespace std;

namespace Portage
{

/**
 * Model consisting of sparse features. 
 */
class SparseModel : public DecoderFeature
{
public:
   struct IMapper {
      Uint num_clust_id;            // number of cluster IDs
      Uint unknown;

      IMapper(Uint unknown)
      : num_clust_id(0)
      , unknown(unknown)
      {}
      virtual ~IMapper() {}
      virtual Uint operator()(Uint in) const = 0;
      virtual Uint numClustIds() const {
         return num_clust_id;
      }
   };

   struct WordClassesMapper : public IMapper {
      typedef unordered_map<Uint,Uint> Word2ClassMap;

      Word2ClassMap word2class_map;

      WordClassesMapper(const string& filename, Voc& voc, Uint unknown = 0)
      : IMapper(unknown)
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

      virtual Uint operator()(Uint voc_id) const
      {
         Word2ClassMap::const_iterator p = word2class_map.find(voc_id);
         return p == word2class_map.end() ? unknown : p->second;
      }
   };

   struct WordClassesMapper_MemoryMapped : public IMapper {
      MMMap  word2class_map;
      const Voc& voc;

      WordClassesMapper_MemoryMapped(const string& filename, const Voc& voc, Uint unknown = 0)
      : IMapper(unknown)
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

      virtual Uint operator()(Uint voc_id) const
      {
         // TODO: Using the mapper<const char*, const char*> that was created for coarse model
         MMMap::const_iterator it = word2class_map.find(voc.word(voc_id));
         if (it == word2class_map.end())
            return unknown;

         return conv<Uint>(it.getValue());
      }
   };

   static IMapper* loadWordClassesMapper(const string& filename, Voc& voc) {
      string magicNumber;
      iSafeMagicStream is(filename);
      if (!getline(is, magicNumber))
         error(ETFatal, "Empty classfile %s", filename.c_str());

      IMapper* mapper = NULL;
      if (magicNumber == MMMap::version1)
         error(ETFatal, "SparseModels require MMmap >= 2.0.");
      else if (magicNumber == MMMap::version2)
         mapper = new WordClassesMapper_MemoryMapped(filename, voc);
      else
         mapper = new WordClassesMapper(filename, voc);

      assert(mapper != NULL);
      return mapper;
   }

public:
   /**
    * Interface for event templates. To add a new template:
    * 1) Derive a class from EventTemplate, and instantiate virtual functions.
    * 2) Add constructor call to createEventTemplates().
    * 3) Add static description to describeAllEventTemplates().
    *
    * NB1: If your template takes file arguments, do the following to allow
    * them to be specified relative to the model directory, but accessed from
    * anywhere:
    * instead of: iSafeMagicStream is(myfile);
    * do:         string fullname = adjustRelativePath(m.path, myfile);
    *             iSafeMagicStream is(fullname);
    *
    * NB2: For efficiency of the recombination operation during decoding, it is
    * better to avoid templates that depend on both the current phrase pair and
    * its context. Such relations can be captured with conjunctions instead.
    */
   class EventTemplate {
   protected:

      SparseModel& m;           // containing model
      string args;              // argument string for derived model

   public:
      /**
       * Constructor, to factor out storage of containing model and args.
       * @param m containing model
       * @param args parameters to derived model
       */
      EventTemplate(SparseModel& m, const string& args) : m(m), args(args) {}

      /**
       * Called by the decoder every time a new source sentence is
       * encountered; templates can do any necessary work at this point.
       */
      virtual void newSrcSent(const newSrcSentInfo& info) {}

      /**
       * Add events generated by this template for a given hypothesis to a list. 
       * @param hyp partial translation: the lastPhrase member contains current
       * phrase, and the back pointer is guaranteed to be non-null (and the
       * prev phrase too, though it is empty at sentence start)
       * @param event_ids list to append events ids to; each id must uniquely
       * identify this event within current template.
       */
      virtual void addEvents(const PartialTranslation& hyp, 
                             vector<Uint>& event_ids) = 0;

      /**
       * Same as addEvents(), but assume that the current phrase is still to
       * come, rather than being included in <context>. Eg, the last phrase in
       * <context> should be treated as the "previous" phrase, and so
       * forth. Templates that depend only on the current phrase (isPure())
       * should add nothing; ones that depend on both context and current
       * phrase (isDirect() && !isPure()) should add events for all possible
       * current phrases.  NB: The default instantiation should work for many
       * cases, but not all!
       *
       */
      virtual void addContextEvents(const PartialTranslation& context, 
                                    vector<Uint>& event_ids) {
         if (isPure()) {
            return;         // always the right thing to do
         } else if (!isDirect()) { // fake a next phrase; override if necessary
            static const PhraseInfo emptyPhrase;
            PartialTranslation dummy(&context, &emptyPhrase);
            addEvents(dummy, event_ids);
         } else {
            assert(false); // override to add events for all future phrases
         }
      }

      /**
       * Similar to addEvents, but rather than adding actual events in some
       * context, add all events that could be generated from this
       * template. The definition of 'could' usually depends on what is currently
       * known, such as a vocabulary, as in the default instantiation below.
       */
      virtual void populate(vector<Uint>& event_ids) {
         for (Uint i = 0; i < m.voc->size(); ++i)
            event_ids.push_back(i);
      }
      
      /**
       * Return true if this event depends on the current phrase pair in any
       * way. 
       */
      virtual bool isDirect() {return false;}

      /**
       * Return true if this event depends only on the current phrase pair.
       */
      virtual bool isPure() {return false;}

      /**
       * Return true if this event depends only on the current phrase pair and
       * its distortion direction with respect to the SR stack.
       */
      virtual bool isPureDistortion() {return false;}

      /**
       * Remap an event id, established under m.voc, for use with a new voc.
       * This is a declaration that new_voc will be replacing m.voc at some
       * point in the future; in particular, words in PartialTranslation
       * objects will be indexed using new_voc. The default version is
       * appropriate for templates whose events are identical to voc indexes.
       * When this isn't the case, most templates should just return id (but
       * templates with indirect dependence may need to instantiate
       * replaceVoc() as well).
       * @param id event id to remap
       * @param new_voc new vocabulary
       * @return remapped id
       */
      virtual Uint remapEvent(Uint id, Voc* new_voc)
         {assert(id < m.voc->size()); return new_voc->add(m.voc->word(id));}

      /**
       * Replace any dependence on m.voc with new_voc. This is a declaration
       * that new_voc will be replacing m.voc; in particular, words in
       * PartialTranslation objects will be indexed using new_voc. Templates
       * that use the default remapEvent(), or that do word mapping using the
       * shared subvoc_map, or that don't depend on word indexes, typically
       * won't need to do anything.
       */
      virtual void replaceVoc(Voc* new_voc) {}

      /**
       * Return true if events generated by the current template imply those
       * generated by the given one, provided local ids match. This can test
       * args members if necessary. It doesn't have to be efficient, since the
       * implication matrix will be memoized. It won't be called for et = *this.
       */
      virtual bool implies(EventTemplate& et) {return false;}

      /** 
       * Return the class name of the current template.
       */
      virtual string name() = 0;

      /**
       * Return a description of the current template instantiation, in 2
       * formats.
       */
      string description() {return "[" + name() + "] " + args;}
      string description2() {return name() + ":" + args;}

      /**
       * Return a string description of a given event for the current template.
       * Default version assumes that events are voc indexes. Override if this
       * is false!  
       * @param e event id previously generated by addEvents()
       */
      virtual string eventDesc(Uint e) {return m.voc->word(e);}
   };

   // Data structure to map voc ids to clusters
   class ClusterMap {
      IMapper* word2class_map;
   public:
      // Constructor: Read many word\tcluster entries from a file, entering the 
      // mapping from vocid to cluster into a common map
      ClusterMap(SparseModel& m, const string& filename);

      // Find the cluster id that corresponds to a given voc id. Returns 0
      // if not found (mkcls starts at 2: 0 is UNK, 1 is Beginning of Sentence)
      Uint clusterId(Uint voc_id);

      // Number of phrases the system can return
      Uint numPhraseIds();

      // Number of clusters the sysem can return
      Uint numClustIds();
      
      void replaceVoc(Voc* oldvoc, Voc* newvoc);

      Uint phraseId(Uint startLex, Uint endLex, Uint len);
      string phraseStr(Uint phraseId);
   };

   // Helper interface to encapsulate distortion decisions
   class Distortion {
   public:
      virtual Uint numTypes() const = 0;
      virtual const vector<Uint>& type(const PartialTranslation& pt) const = 0;
      virtual const vector<Uint>& type(const Range& shift,
                                       const ShiftReducer* stack) const = 0;
      virtual Uint sig(const PartialTranslation& pt) const = 0;
      virtual Uint sigSize() const = 0;
      virtual string typeString(Uint type) const = 0;
      virtual string name() const = 0;
      virtual const vector<EventTemplate*>& clients() const = 0;
   };
   
   private:

   // Represent a specific Boolean event.
   struct Event {
      Uint tid;                 // id of generating template
      Uint eid;                 // event id from template
      Event(Uint tid, Uint eid) : tid(tid), eid(eid) {}
      bool operator<(const Event& e) const 
         {return tid < e.tid || (tid == e.tid && eid < e.eid);}
      bool operator>(const Event& e) const 
         {return tid > e.tid || (tid == e.tid && eid > e.eid);}
      bool operator==(const Event& e) const {return tid == e.tid && eid == e.eid;}
      string toString(SparseModel& m) const {
         return m.event_templates[tid]->description2() + "/" 
            + m.event_templates[tid]->eventDesc(eid);
      }
      struct Hash {
         size_t operator()(const Event& e) const { return e.tid*2551+e.eid; }
      };
   };
   
   // Represent a feature (a conjunction of events). NB: not all subsets of an
   // event conjunction are also features, due to pruning, so we maintain event
   // sets explicitly rather than as pointers to constituent features.
   struct Feature {
      vector<Event> events;      // events in the conjunction
      float weight;
      Feature() : weight(0.0) {}
      Feature(Event e) : events(1, e), weight(0.0) {}
      Feature(const vector<Event>& events) : events(events), weight(0.0) {}
      bool operator==(const Feature& f) const {return events == f.events;}
      bool operator!=(const Feature& f) const {return events != f.events;}
      void add(Event e) {
         events.push_back(e); sort(events.begin(), events.end());
      }
      string toString(SparseModel& m) const {
         string ret;
         for (Uint i = 0; i < events.size(); ++i) {
            ret += events[i].toString(m);
            if (i+1 < events.size()) ret += " & ";
         }
         return ret;
      }
   };

   // Functor for feature comparisons during pruning. Use frequencies or weights.
   struct PruneComp {
      SparseModel& m;
      bool by_freq;
      bool operator()(Uint a, Uint b) const {
         return by_freq ?
            m.feature_freqs[a] > m.feature_freqs[b] :
            abs(m.features[a].weight) > abs(m.features[b].weight);
      }
      PruneComp(SparseModel& m, bool by_freq = false) : m(m), by_freq(by_freq) {}
   };

   // Functor for lexicographic feature comparisons.
   struct LexComp {
      SparseModel& m;
      bool operator()(Uint a, Uint b) const {
         return operator()(m.features[a], m.features[b]);
      }
      bool operator()(const Feature& a, const Feature& b) const {
         Uint s = min(a.events.size(), b.events.size());
         for (Uint i = 0; i < s; ++i) {
            if (a.events[i] < b.events[i]) return true;
            else if (a.events[i] > b.events[i]) return false;
         }
         return a.events.size() < b.events.size();
      }
      LexComp(SparseModel& m) : m(m) {}
   };

   // File extensions for saved models, and other conventions
   static string templ_exten;   // .template
   static string feats_exten;   // .feats.gz
   static string wts_exten;     // .wts.gz
   static string wtsums_exten;  // .wtsums.gz
   static string freqs_exten;   // .freqs.gz
   static string voc_exten;     // .voc.gz
   static string numex_exten;   // .numex
   static string pfs_flag;      // #pfs
   static string fire_flag;     // #col

   vector<EventTemplate*> event_templates; // event templates in this model
   vector< vector<bool> > event_template_implies;  // tid1 -> implies tid2
   
   vector<Feature> features;    // feature id -> feature
   vector<Uint> feature_freqs;  // feature id -> frequency, during learning
   vector<Uint> feature_lastex; // feature id -> last example where wt changed
   vector<float> feature_wtsums; // feature id -> average wt (over examples)

   typedef unordered_map<Event, vector<Uint>, Event::Hash> EventMap;
   //typedef map< Event, vector<Uint> > EventMap;
   EventMap events; // event -> feature id list

   vector<Feature> partial_features; // feat id -> partial feat (recombination)
   EventMap partial_events;     // event -> id list in partial features ("")

   // Features that can be cached by phrase pair, and the event templates and
   // events that constitute them.
   vector<Uint> pure_event_template_ids;
   EventMap pure_events;

   // Features that can be cached by phrase pair and distortion configuration,
   // and the event templates and events that constitute them.
   vector<Uint> pure_distortion_event_template_ids;
   EventMap pure_distortion_events;

   // Features that have to be calculated for each decoder state, and the event
   // templates and events that constitute them.
   vector<Uint> other_event_template_ids;
   EventMap other_events;

   // Strip #xxx suffix flag(s) from a given model name, optionally recording
   // their values in the given variables.
   static string stripSuffixFlags(const string& modelname, 
                                  bool* had_pfs = NULL,
                                  string* idname = NULL,
                                  string* fireval = NULL,
                                  string* tag = NULL);

   // Make a local filename for a weights file from a given model name
   // (possibly including path info). If suff is not "" it will be appended,
   // before the .wts extension, with any #/ or blanks replaced by underscores.
   // @param relative_to if modelname is a relative path, preprend relative_to/ to it.
   static string localWtsFilename(const string& modelname, const string& relative_to,
         const string& suff);

   // Populate event_templates list from spec arg to constructor.
   void createEventTemplates(const string& spec);

   // Tally the set of features that are active from the given feature subset
   // Implemented for a decoder optimization via caching.
   double tallyFeatures(const PartialTranslation& context,
         vector<Uint>& event_template_ids, EventMap& event_map);

   // Get the set of features that are active in current context. This will add
   // new atomic features if new_atoms is set, and add new conjoined features
   // if new_conj is set.
   void getFeatures(const PartialTranslation& context, set<Uint>& fset, 
                    bool new_atoms, bool new_conj);

   // Get the set of partial features that are active in the current context.
   // This is for hypothesis recombination in the decoder, and polls
   // EventTemplate::addContextEvents() instead of addEvents().
   // compilePartialFeatures() is assumed to have been called.
   void getPartialFeatures(const PartialTranslation& context, set<Uint>& fset);

   // Test if the given event can be conjoined to an existing set. If so,
   // add the resulting conjoined feature to the end of the features list, and
   // make appropriate entries in the events map.
   bool conjoin(const vector<Event>& evs, Event event);

   // Prune any features having weight of 0, starting at a given position in
   // the feature list.  Retained features will be re-indexed if necessary.  
   // Should only be called if learning (assumes feature_* arrays are
   // non-empty).
   void pruneZeroWeightFeatures(Uint start_index);

   // Increment the weight for a given feature by a given amount. This takes
   // care of weight averaging book-keeping too.
   void incrWeight(Uint fid, float incr) {
      feature_wtsums[fid] += features[fid].weight * (exno - feature_lastex[fid]);
      feature_lastex[fid] = exno;
      features[fid].weight += incr;
   }

   // Compare template list in current model with that in m. Die with error msg
   // if they don't match.
   void ensureTemplateMatch(const SparseModel& m);

   // Dump context to stderr.
   void dump(const PartialTranslation& context, const PhraseInfo* alt_lastphrase = NULL);

   // Dump events generated by given template id in current context.
   void dump(Uint template_id, vector<Uint>& event_ids);

   // Dump a feature
   void dumpFeature(Uint fid) {
      cerr << "f" << fid << ": " << features[fid].weight << " " 
           << features[fid].toString(*this) << endl;
   }
   
   // Rebuilds events map from information in the features vector.
   void rebuildEventMap();

   // helper for buildDecodingEventMaps()
   void buildDecodingEventMapsHelper(Uint feature_id, EventMap& event_map,
      set<Uint>& template_id_set);

   // Build the event maps specialized for caching during decoding
   void buildDecodingEventMaps();

   // Check consistency of model (debugging only).
   void check();

   // Compile the partial_features map, for recombination.
   void compilePartialFeatures();

public:

   Uint verbose;

   // learning hyper-parameters (see 1st constructor for doc):
   bool learn_conjunctions;
   Uint max_features_for_conjoin;
   Uint min_freq_for_conjoin;
   bool prune0;

   // whether to precompute future score during decoding or not
   bool precompute_future_score;

   // source sent id -> whole model fires or not (#colN=T flag)
   vector<bool> fire_on_sent;

   // resources shared among many event templates, managed by SparseModel
   string path;                 // path to main model, for relativization
   Voc* voc;                    // general voc, maps word strings <-> indexes
   vector<Uint> srcsent;        // indexes of words in current source sentence
   Uint ssid;                   // index of current source sentence during decoding
   Uint exno;                   // index of current example during learning
   bool active;                 // whole model is active on this source sentence
   map<string,NgramCounts*> ngcounts_map; // filename -> NgramCounts object
   map<string, map<Uint,Uint>*> subvoc_map; // filename -> subvoc (voc-id -> sub-id)
   map<string, ClusterMap*> file_to_clust; // filename -> clustervoc (voc-id -> clus-id)
   string idname;               // name of id file line-aligned with source

   // Read a list of unique words from a file, entering them into the common
   // voc. Return a map from voc id to subvoc id (0-based position in the
   // list). This is a utility function used to add to subvoc_map.
   map<Uint,Uint>* readSubVoc(const string& filename);

   // Find the subvoc id that corresponds to given voc id. Return subvoc size
   // if not found.
   Uint subVocId(map<Uint,Uint>* subvoc, Uint voc_id) {
      map<Uint,Uint>::const_iterator p = subvoc->find(voc_id);
      return p == subvoc->end() ? subvoc->size() : p->second;
   }

   // Find the voc id that corresponds to given subvoc id (slow).
   Uint vocId(map<Uint,Uint>* subvoc, Uint subvoc_id);

   // All possible event templates.
   static string describeAllEventTemplates();

   // Summarize structural parameters of current model.
   string describeModelStructure() const;

   // Implications between templates in current model.
   string describeActiveEventTemplateImplications() const;

   Uint numFeatures() const {return features.size();}
   Uint numEvents() const {return events.size();}

   /** 
    * Read a vector of weights from a model previously save()'d to disk. This
    * first checks in the current directory for a local weights file produced
    * by writeWeights(); if no such exists, if reads the model's original
    * weights. 
    * @param name model's name, including suffix #flags if any
    * @param relative_to if name is a relative path, preprend relative_to/ to it.
    * @param weights vector in which to store weights (cleared first)
    * @param allow_non_local indicates whether looking for the model's original
    * weights as a fallback to local weights is permitted.
    */
   static void readWeights(const string& name, const string& relative_to,
         vector<float>& weights, bool allow_non_local);

   /** 
    * Write a vector of weights for a model previously save()'d to disk.
    * Weights are written in the current directory, to the file <m>.wts.gz,
    * where <m> is a filename constructed from the model's name. This will
    * overwrite the model's original weights if <name> is in the current
    * directory (with warning and backup), but the intent is that it produce a
    * separate, 'local', weights file.
    * @param name model's name, including suffix #flags if any
    * @param relative_to if name is a relative path, preprend relative_to/ to it.
    * @param weights vector to be written
    * @param os start position in vector at which to find weights
    * @return number of weights written
    */
   static Uint writeWeights(const string& name, const string& relative_to,
         vector<float>& weights, Uint os);


   /**
    * Construct from a file containing an event-template spec, and a
    * vocabulary. This creates a model with no features; the intent is that it
    * be populated with calls to learn().
    * @param a file containing a set of event template specifiers in the form
    * of one or more space-separated pairs '[et] args', where et is an event
    * template (class) name, and args is a set of optional arguments to that
    * template.
    * @param verbose controls level of messages written to stderr: 0 = none, 1
    * = brief, 2 = some, 3 = lots.
    * @param learn_conjunctions turns on learning of conjunctions of atomic
    * events
    * @param max_features_for_conjoin if learning conjunctions, the maximum
    * number of active features in a given context for new conjunctions to be
    * acquired; larger feature sets take longer
    * @param min_freq_for_conjoin if learning conjunctions, the minimum
    * feature frequency (not weight) required for a feature to be considered
    * for conjunction.
    * @param prune0 prune any newly-acquired features that have weight 0
    * @param voc vocabulary, used to access string versions of all word indexes
    * in a partial translation; must be non-NULL, and must exist for the
    * lifetime of the SparseModel object.
    */
   SparseModel(const string& specfile, Uint verbose, bool learn_conjunctions, 
               Uint max_features_for_conjoin, Uint min_freq_for_conjoin, 
               bool prune0, Voc* voc) : 
      verbose(verbose), 
      learn_conjunctions(learn_conjunctions), 
      max_features_for_conjoin(max_features_for_conjoin),
      min_freq_for_conjoin(min_freq_for_conjoin),
      prune0(prune0), precompute_future_score(false),
      voc(voc)
   {
      assert(voc);
      string mstring;
      string& spec = gulpFile(specfile.c_str(), mstring);
      path = DirName(specfile);
      createEventTemplates(spec);
   }
   
   /**
    * Load from a file.  
    * @param file name of file to which model was previously save()'d. This
    * string may end with one or more of the following suffixes, all of which
    * are stripped before the file is read:
    * #pfs - set precompute_future_score
    * #ID - set idname to ID, interpreted as the name of an id file
    *  line-aligned with the current source file.
    * #colN=T - features in the model will fire only for sentences for
    *  which the Nth column in ID contains tag T (#ID must also be given).
    * #_tag - tag is a user-defined string that will be appended to the local
    * weights file in order to distinguish it from other sparse features
    * based on the same model file (eg for FEDA)
    * @param relative_to if file is a relative path, preprend relative_to/ to it.
    * @param verbose see constructor above
    * @param voc external vocabulary (see 1st constructor); may not be NULL
    * @param local_wts look for a local weights file, as previously written by
    * writeWeights 
    * @param allow_non_local with local_wts, indicates whether looking for the
    * model's original weights as a fallback is also permitted.
    * @param load_opts load optional statistics not needed for use as a
    * decoder feature: freqs, wtsums, and numex.
    * @param in_decoder compile partial features map, for recombination during
    * decoding, and build decoding event maps.
    */
   SparseModel(const string& file, const string& relative_to,
               Uint verbose, Voc* voc,
               bool local_wts, bool allow_non_local,
               bool load_opts, bool in_decoder);

   /**
    * Generate all possible events from all event templates using each
    * template's populate() function, and create atomic features from these
    * when possible (when the event is direct() and doesn't already correspond
    * to a feature. This function makes most sense with the first constructor.
    */
   void populate();
      
   /**
    * Read in the contents of a model(s) stored on disk, and add to the current
    * one. The stored model must have exactly the same set of event templates
    * (not necessarily all active) as the current one. The current model must
    * have active frequency/wtsums/lastex information: it must be freshly
    * learned, or read in with load_opts set. The result contains the
    * union of features and templates from the two models, with weights,
    * frequencies, wtsums, and number of examples added, and last example set
    * to exno for all features.
    * @param files name of file to which one or more model were previously
    * save()'d
    * @param prune don't add any features with abs(weight) < this value
    */
   void add(const vector<string>& files, double prune=0.0);
   void add2(const string& file);

   /**
    * Compare the contents of the current model to a stored model, and write
    * the first difference to stderr. Models are deemed identical if they
    * contain the same templates (in the same order), and the same set of
    * features (independent of order). Weights, freqs, etc. are not
    * compared. This sorts both feature sets, so it's expensive.
    * @param file name of file to which model was previously save()'d
    * @return true if models are identical
    */
   bool compare(const string& file);

   /**
    * Save this model to disk. This creates files
    * <name>.{templates,numex}, <name>.{feats,wts,wtsums,freqs,voc}.gz, for
    * templates, number of training examples, features, weights, weight sums,
    * frequencies, and vocabulary, respectively. Weight sums are completed up
    * to current example before saving.
    * @param name base name to save to, as explained above
    * @param prune_templates prune inactive templates if true
    */
   void save(const string& name, bool prune_templates=true);

   /**
    * Replace the model's existing vocabulary with a new one. This involves
    * re-indexing events, so it's potentially expensive.
    * @param oldvoc the vocab used until now with the model (== the voc member)
    * @param the new vocab (the voc member will be set to this)
    */
   void replaceVoc(Voc* oldvoc, Voc* newvoc);

   /**
    * (Re-)Initialize data structures for learning. This must be called before
    * the first call to learn(). It clears, then allocates, any
    * feature-frequency and weight-averaging data from previous learn() calls
    * (so be careful).
    */
   void learnInit();

   /**
    * Learn from a phrase pair in context. 
    * @param ref the reference hypothesis, ending with correct phrase pair
    * @param mbest the decoder model's preferred hyp. This is identical to ref
    * except for, possibly, the last phrase pair. If it is non-null, learning
    * is discriminative; otherwise we just count active features.
    * @return the number of features that are active on either ref or nbest,
    * minus the number that are active on both (0 if feature vects are
    * identical)
    */
   Uint learn(const PartialTranslation* ref, const PartialTranslation* mbest = NULL);

   /**
    * Prune to a given number of features, using weight as a criterion (by
    * default).  This requires a previous call to learnInit(), and doesn't
    * really make sense without at least one previous call to learn(). It
    * assumes that frequency and averaging information are available.
    * @param num_features number of features to retain
    * @param by_freq prune by frequency instead of weight
    * @param conj_only retain only conjunctive features
    */
   void prune(Uint num_features, bool by_freq = false, bool conj_only = false);

   /**
    * Set weights to their average values over all examples, for averaged
    * perceptron learning. This requires a previous call to learnInit(). It is
    * optional, and should be done after learning is finished.
    * @param sum_only only complete the weight sums (up to current example);
    * don't modify actual weights.
    */
   void averageWeights(bool sum_only=false);

   /**
    * Scale weights (including sums if they exist) by a given values.
    */
   void scaleWeights(double v);

   /**
    * Dump a text representation of all features to a stream.
    * @param os stream to write to
    * @param start_index if non-zero, prepend index numbers to feature list
    * @param verbose if true, write one line for each feature, in the form
    * 'index\twt\tdesc'; otherwise group all consecutive features sharing the
    * same event template set (NB: set, not bag), with index ranges and summed
    * weights.
    */
   void dump(ostream& os, Uint start_index = 0, Uint verbose = 0);

   /**
    * Get component features that are active in current context.
    * @param context current partial translation
    * @param fset will be cleared and set to 0-based indexes of features that
    * apply to the last phrase pair in context
    */
   void getComponents(const PartialTranslation& context, set<Uint>& fset) {
      if (active) getFeatures(context, fset, false, false);
   }

   /*
    * The following are DecoderFeature virtuals. See descriptions in
    * decoder_feature.h.
    */
   void newSrcSent(const newSrcSentInfo& info);
   double precomputeFutureScore(const PhraseInfo& phrase_info);
   double futureScore(const PartialTranslation &trans) {return 0.0;}
   double score(const PartialTranslation& pt);
   double partialScore(const PartialTranslation &trans) {return 0.0;}
   Uint computeRecombHash(const PartialTranslation &pt);
   bool isRecombinable(const PartialTranslation &pt1,
                       const PartialTranslation &pt2);
   Uint lmLikeContextNeeded() { return 1; }
};

/*==============================================================================
  Event Templates
  ============================================================================*/

/*------------------------------------------------------------------------------
  Pick out specific words at given positions in current or previous source or
  target phrases, or target context.
  ----------------------------------------------------------------------------*/

/**
 * Occurrence of a particular target word at a given position in current
 * context.
 */
class TgtContextWordAtPos : public SparseModel::EventTemplate
{
   Uint pos;                     // word position

public:
   // args is a string -n, designating the nth word before the current phrase.
   TgtContextWordAtPos(SparseModel& m, const string& args);
   virtual string name() {return "TgtContextWordAtPos";}
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events);
};

/**
 * Virtual base class to capture occurrence of a particular word at a given
 * position relative to the beginning or end of a vector<Uint> range.
 */
class PhraseWordAtPos : public SparseModel::EventTemplate
{
   int pos;                     // word position, relative to end if < 0

public:
   /**
    * Construct.
    * @param m containing model
    * @param args a number n, designating the nth word in the phrase if n > 0,
    * and the -nth word from the end if n < 0. 
    * @param cname name of derived class, for error message
    */
   PhraseWordAtPos(SparseModel& m, const string& args, const string& cname);

   /**
    * Add designated word, if it's in range.
    */
   void addEvents(vector<Uint>::const_iterator b,
                  vector<Uint>::const_iterator e,
                  vector<Uint>& events);
};

/**
 * Occurrence of word at a given position in current target phrase.
 */
class CurTgtPhraseWordAtPos : public PhraseWordAtPos
{
public:
   CurTgtPhraseWordAtPos(SparseModel& m, const string& args) : 
      PhraseWordAtPos(m, args, name()) {}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual string name() {return "CurTgtPhraseWordAtPos";}
   virtual bool implies(EventTemplate& et) {
      return et.name() == "CurTgtPhraseContainsWord" ? true : false;
   }
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      const PhraseInfo *p = context.lastPhrase;
      PhraseWordAtPos::addEvents(p->phrase.begin(), p->phrase.end(), 
                                 events);
   }
};

/**
 * Occurrence of word at a given position in current source phrase.
 */
class CurSrcPhraseWordAtPos : public PhraseWordAtPos
{
public:

   CurSrcPhraseWordAtPos(SparseModel& m, const string& args) :
      PhraseWordAtPos(m, args, name()) {}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual string name() {return "CurSrcPhraseWordAtPos";}
   virtual bool implies(EventTemplate& et) {
      return et.name() == "CurSrcPhraseContainsWord" ? true : false;
   }
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      const PhraseInfo *p = context.lastPhrase;
      PhraseWordAtPos::addEvents(m.srcsent.begin() + p->src_words.start,
                                 m.srcsent.begin() + p->src_words.end,
                                 events);
   }
};

/**
 * Occurrence of word at a given position in previous target phrase.
 */
class PrevTgtPhraseWordAtPos : public PhraseWordAtPos
{
public:
   PrevTgtPhraseWordAtPos(SparseModel& m, const string& args) : 
      PhraseWordAtPos(m, args, name()) {}
   virtual string name() {return "PrevTgtPhraseWordAtPos";}
   virtual bool implies(EventTemplate& et) {
      return et.name() == "PrevTgtPhraseContainsWord" ? true : false;
   }
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      if (!context.back->back) return;
      const PhraseInfo *p = context.back->lastPhrase;
      PhraseWordAtPos::addEvents(p->phrase.begin(), p->phrase.end(), events);
   }
};

/**
 * Occurrence of word at a given position in previous source phrase.
 */
class PrevSrcPhraseWordAtPos : public PhraseWordAtPos
{
public:

   PrevSrcPhraseWordAtPos(SparseModel& m, const string& args) :
      PhraseWordAtPos(m, args, name()) {}
   virtual string name() {return "PrevSrcPhraseWordAtPos";}
   virtual bool implies(EventTemplate& et) {
      return et.name() == "PrevSrcPhraseContainsWord" ? true : false;
   }
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      if (!context.back->back) return;
      const PhraseInfo *p = context.back->lastPhrase;
      PhraseWordAtPos::addEvents(m.srcsent.begin() + p->src_words.start,
                                 m.srcsent.begin() + p->src_words.end,
                                 events);
   }
};

/*------------------------------------------------------------------------------
  Pick out specific words occurring anywhere in current or previous source or
  target phrases.
  ----------------------------------------------------------------------------*/

/**
 * Virtual base class to capture occurrence of word anywhere in a phrase.
 */
class PhraseContainsWord : public SparseModel::EventTemplate
{
public:
   PhraseContainsWord(SparseModel& m, const string& args) : 
      EventTemplate(m, args) {
         if (args != "") 
            error(ETFatal, "unexpected argument for *PhraseContainsWord Template");
      }

   /**
    * Add all words in given range, avoiding duplication.
    */
   void addEvents(vector<Uint>::const_iterator b,
                  vector<Uint>::const_iterator e,
                  vector<Uint>& events);
};

/**
 * Occurrence of word anywhere in current target phrase.
 */
class CurTgtPhraseContainsWord : public PhraseContainsWord
{
public:
   CurTgtPhraseContainsWord(SparseModel& m, const string& args) : 
      PhraseContainsWord(m, args) {}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual string name() {return "CurTgtPhraseContainsWord";}
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      const PhraseInfo *p = context.lastPhrase;
      PhraseContainsWord::addEvents(p->phrase.begin(), p->phrase.end(), events);
   }
};

/**
 * Occurrence of word anywhere in current source phrase.
 */
class CurSrcPhraseContainsWord : public PhraseContainsWord
{
public:

   CurSrcPhraseContainsWord(SparseModel& m, const string& args) :
      PhraseContainsWord(m, args) {}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual string name() {return "CurSrcPhraseContainsWord";}
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      const PhraseInfo *p = context.lastPhrase;
      PhraseContainsWord::addEvents(m.srcsent.begin() + p->src_words.start,
                                    m.srcsent.begin() + p->src_words.end,
                                    events);
   }
};

/**
 * Occurrence of word anywhere in previous target phrase.
 */
class PrevTgtPhraseContainsWord : public PhraseContainsWord
{
public:
   PrevTgtPhraseContainsWord(SparseModel& m, const string& args) : 
      PhraseContainsWord(m, args) {}
   virtual string name() {return "PrevTgtPhraseContainsWord";}
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      if (!context.back->back) return;
      const PhraseInfo *p = context.back->lastPhrase;
      PhraseContainsWord::addEvents(p->phrase.begin(), p->phrase.end(), events);
   }
};

/**
 * Occurrence of word anywhere in previous source phrase.
 */
class PrevSrcPhraseContainsWord : public PhraseContainsWord
{
public:

   PrevSrcPhraseContainsWord(SparseModel& m, const string& args) :
      PhraseContainsWord(m, args) {}
   virtual string name() {return "PrevSrcPhraseContainsWord";}
   virtual void addEvents(const PartialTranslation& context, 
                          vector<Uint>& events) {
      if (!context.back->back) return;
      const PhraseInfo *p = context.back->lastPhrase;
      PhraseContainsWord::addEvents(m.srcsent.begin() + p->src_words.start,
                                    m.srcsent.begin() + p->src_words.end,
                                    events);
   }
};

/*------------------------------------------------------------------------------
  Ngram count bins
  ----------------------------------------------------------------------------*/

/**
 * Target-language ngram count bin. 
 * NB: implementation assumes that ngram order is <= order of current LM.
 */
class TgtNgramCountBin : public SparseModel::EventTemplate
{
   static const char* bos;  // <s>
   static const char* eos;  // </s>

   Uint n;                      // order (ngram length)
   Uint low;                    // min freq
   Uint high;                   // max freq (0 for none)
   int pos;                     // position of last word within phrase
   NgramCounts* ngcounts;       // ptr to possibly-shared cnts in ngcounts_map

   bool using_bos;              // true if ngrams include sent-beg indicator
   bool using_eos;              // true if ngrams include sent-end indicator

public:

   /**
    * Construct.
    * @param m containing model
    * @param args: 'cfile n low high pos', where:
    * - cfile is an ngram count file in SRILM format, optionally containing
    *   bos/eos markers (detected and handled automatically).
    * - n is ngram length; should not exceed current LM order
    * - [low,high] is a count range (no upper limit if high is 0): the event
    *   will be active if current ngram has count from cfile in this range
    * - pos is the position of the ngram's last word relative to the last
    *   phrase in the current hyp: values >= 1 designate the first and
    *   subsequent words, values < 1 designate the last and previous words, and
    *   0 designates the word after the phrase iff this is the last word in the
    *   sentence (ngrams must end with eos in order to match).
    */
   TgtNgramCountBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp,
                          vector<Uint>& event_ids);

   virtual void populate(vector<Uint>& event_ids) {event_ids.push_back(0);}
   
   // do nothing for all recomb fcns, on the assumption that we're covered by
   // the LM in any case
   virtual void addContextEvents(const PartialTranslation& context, 
                                 vector<Uint>& event_ids) {}

   virtual bool isDirect() {return true;}
   virtual bool isPure() {return false;}

   // event index is always 0; re-voc'ing handled by reloading ngcounts_map, if
   // necessary (this is handled by the SparseModel itself)
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}

   virtual string name() {return "TgtNgramCountBin";}
   virtual string eventDesc(Uint e) {return "single event";}
};

/*------------------------------------------------------------------------------
  Phrase pair length bin
  ----------------------------------------------------------------------------*/

class PhrasePairLengthBin : public SparseModel::EventTemplate
{
   Uint srcmin;
   Uint srcmax;
   Uint tgtmin;
   Uint tgtmax;

public:
   
   /**
    * Construct.
    * @param m containing model
    * @param args: 'srcmin srcmax tgtmin tgtmax' - use 0 for no max.
    */
   PhrasePairLengthBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp,
                          vector<Uint>& event_ids) {
      if (hyp.lastPhrase) {
         Uint slen = hyp.lastPhrase->src_words.end - hyp.lastPhrase->src_words.start;
         Uint tlen = hyp.lastPhrase->phrase.size();
         if (slen >= srcmin && (srcmax == 0 || slen <= srcmax) &&
             tlen >= tgtmin && (tgtmax == 0 || tlen <= tgtmax))
            event_ids.push_back(0);
      }
   }

   virtual void populate(vector<Uint>& event_ids) {event_ids.push_back(0);}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairLengthBin";}
   virtual string eventDesc(Uint e) {return "single event";}
};

/*------------------------------------------------------------------------------
  Phrase pair total length bin
  ----------------------------------------------------------------------------*/

class PhrasePairTotalLengthBin : public SparseModel::EventTemplate
{
   Uint len;

public:
   
   /**
    * Construct.
    * @param m containing model
    * @param args: len
    */
   PhrasePairTotalLengthBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp,
                          vector<Uint>& event_ids) {
      if (hyp.lastPhrase) {
         Uint slen = hyp.lastPhrase->src_words.end - hyp.lastPhrase->src_words.start;
         Uint tlen = hyp.lastPhrase->phrase.size();
         if (slen + tlen == len) event_ids.push_back(0);
      }
   }

   virtual void populate(vector<Uint>& event_ids) {event_ids.push_back(0);}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairTotalLengthBin";}
   virtual string eventDesc(Uint e) {return "single event";}
};

/*------------------------------------------------------------------------------
  Lexicalized unal features
  ----------------------------------------------------------------------------*/

/**
 * Virtual base class for lexicalized unal features. Factors out wordlist
 * handling, and caching the alignment -> unal positions mapping.
 */
class LexUnal : public SparseModel::EventTemplate
{
protected:

   map<Uint,Uint>* lex;         // voc id -> event id

   typedef UnalFeature::CacheKey CacheKey;
   typedef UnalFeature::CacheKeyHash CacheKeyHash;
   typedef unordered_map<CacheKey, vector<Uint>, CacheKeyHash> Cache;
   Cache cache; // align-index, src-len, tgt-len -> list of unal positions

public:

   /**
    * Construct.
    * @param m containing model
    * @param args a file containing a list of words to test for "unalignment"
    */
   LexUnal(SparseModel& m, const string& args);
   
   virtual void addEvents(const PartialTranslation& hyp, 
                          vector<Uint>& event_ids) = 0;

   virtual void populate(vector<Uint>& events) {
      for (Uint i = 0; i < lex->size(); ++i)
         events.push_back(i);
   }
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}

   // id's never change, but lex map does if the voc is changed; this is
   // factored into SparseModel, since lex may be shared among many template
   // objects:
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual bool implies(EventTemplate& et) = 0;
   virtual string name() = 0;
   virtual string eventDesc(Uint e) {return m.voc->word(m.vocId(lex, e));}
};

/**
 * Selected word appears unaligned anywhere in current target phrase.
 */
class LexUnalTgtAny : public LexUnal
{
public:

   LexUnalTgtAny(SparseModel& m, const string& args) : LexUnal(m, args) {}
   virtual string name() {return "LexUnalTgtAny";}
   virtual bool implies(EventTemplate& et) {return false;} // most general case
   virtual void addEvents(const PartialTranslation& hyp, 
                vector<Uint>& event_ids);
};

/**
 * Selected word appears unaligned anywhere in current source phrase.
 */
class LexUnalSrcAny : public LexUnal
{
public:

   LexUnalSrcAny(SparseModel& m, const string& args) : LexUnal(m, args) {}
   virtual string name() {return "LexUnalSrcAny";}
   virtual bool implies(EventTemplate& et) {return false;} // most general case
   virtual void addEvents(const PartialTranslation& hyp, 
                vector<Uint>& event_ids);
};

/*------------------------------------------------------------------------------
  Aligned word pair feature(s)
  ----------------------------------------------------------------------------*/

class AlignedWordPair : public SparseModel::EventTemplate
{
   map<Uint,Uint>* src_lex;     // voc id -> src subvoc id
   map<Uint,Uint>* tgt_lex;     // voc id -> tgt subvoc id

   typedef UnalFeature::CacheKey CacheKey;
   typedef UnalFeature::CacheKeyHash CacheKeyHash;
   typedef unordered_map<CacheKey, vector<Uint>, CacheKeyHash> Cache;
   Cache cache; // align-index, src-len, tgt-len -> (tgt-pos -> unique src-pos)

   // Events are pairs (sid,tid), where sid is: < src_lex->size() to indicate a
   // specific subvoc word; src_lex->size() to indicate any non-subvoc word;
   // and src_lex->size()+1 to indicate a non-1-1 connection (null or
   // multiple). The scheme for mapping (sid,tid) to unique event indexes
   // includes four pairs that will never occur, but there is no requirement
   // that all event indexes result in features anyway.
   Uint numEvents() {return (src_lex->size()+2) * (tgt_lex->size()+2);}

public:

   /**
    * Construct.
    * @param m containing model
    * @param args 'srcfile tgtfile' - names of source and target files
    * containing words to participate in aligned pairs. 
    */
   AlignedWordPair(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, 
                          vector<Uint>& event_ids);

   virtual void populate(vector<Uint>& events) {
      for (Uint i = 0; i < numEvents(); ++i) events.push_back(i);
   }
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() {return "AlignedWordPair";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Phrase pair count bin
  ----------------------------------------------------------------------------*/

class PhrasePairCountBin : public SparseModel::EventTemplate
{
   Uint minfreq;
   Uint maxfreq;

public:

   // parse "min max" spec or die with error
   static void parseBinSpec(const string& spec, Uint& minfreq, Uint& maxfreq, 
                            bool allow_0max=true);

   /**
    * Construct.
    * @param m containing model
    * @param args 'minfreq maxfreq' - fire if current phrase pair has joint
    * count in [minfreq,maxfreq] (no upper limit if maxfreq is 0)
    */
   PhrasePairCountBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids) {event_ids.push_back(0);}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() {return "PhrasePairCountBin";}
   virtual string eventDesc(Uint e) {return "single event";}
};

/*------------------------------------------------------------------------------
  Phrase pair count bin, with multiple bins handled as internal events, for
  speed.
 ----------------------------------------------------------------------------*/

class PhrasePairCountMultiBin : public SparseModel::EventTemplate
{
protected:

   Uint num_bins;
   unordered_map<Uint,Uint> freqmap;      // freq -> bin

   bool have_high_bin;          // true if using open-ended high bin
   Uint high_bin_threshold;     // lowest value in open-ended high bin
   Uint high_bin;               // index of open-ended high bin

public:
   /**
    * Construct.
    * @param m containing model
    * @param args binfile - file containing a set of bins, in the form of "low
    * high" pairs, one per line, corresponding to events where joint count is
    * in [low high]. Ranges can't overlap. One bin can have high value of 0 to
    * indicate no upper bound.
    */
   PhrasePairCountMultiBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids);
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() {return "PhrasePairCountMultiBin";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Phrase pair count bin, with multiple bins over multiple count columns.
 ----------------------------------------------------------------------------*/

class PhrasePairCountsMultiBin : public PhrasePairCountMultiBin
{
   Uint beg_col;  // first count column used
   Uint end_col;  // last count column used
   Uint num_bins; // total number of bins (one set per column)

public:
   /**
    * Construct for a given set of bins and range of count columns.
    * @param m containing model
    * @param args binfile b e - binfile is as for PhrasePairCountMultiBin, b is
    * the 1-based index of the first count column to bin and e is the
    * index of the last column. 
    */
   PhrasePairCountsMultiBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids);
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() {return "PhrasePairCountsMultiBin";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Binned frequency of rarest target word in current phrase 
 ----------------------------------------------------------------------------*/

class RarestTgtWordCountBin : public SparseModel::EventTemplate
{
   Uint num_bins;
   unordered_map<Uint,Uint> binmap;   // voc id -> bin (low bin = low count)

public:
   /**
    * Construct.
    * @param m containing model
    * @param args "counts bins" - a counts file containing "word count" pairs,
    * and a bins file containing bin specifications in the form of
    * non-overlapping "low high" pairs.
    */
   RarestTgtWordCountBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids);
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual void replaceVoc(Voc* new_voc);
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() {return "RarestTgtWordCountBin";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Binned average target word score.
 ----------------------------------------------------------------------------*/

class AvgTgtWordScoreBin : public SparseModel::EventTemplate
{
protected:

   bool linear_bins;   // bin linearly if true, else logwise
   Uint num_bins;
   double minscore;    // lowest score read in
   double range;       // range of scores read in
   unordered_map<Uint,float> binmap;   // voc id -> score

public:
   /**
    * Construct.
    * @param m containing model
    * @param args "scores numbins method" - a scores file containing "word
    * score" pairs, an integer number of desired bins, then either 'lin' to
    * select evenly-spaced bins, or 'log' to select log-spaced bins (scores
    * must be > 0 for log).
    */
   AvgTgtWordScoreBin(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids);
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual void replaceVoc(Voc* new_voc);
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() {return "AvgTgtWordScoreBin";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Binned minimum target word score.
 ----------------------------------------------------------------------------*/

class MinTgtWordScoreBin : public AvgTgtWordScoreBin
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "scores numbins method" - a scores file containing "word
    * score" pairs, an integer number of desired bins, then either 'lin' to
    * select evenly-spaced bins, or 'log' to select log-spaced bins (scores
    * must be > 0 for log).
    */
   MinTgtWordScoreBin(SparseModel& m, const string& args) : 
      AvgTgtWordScoreBin(m, args) {}

   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string name() {return "MinTgtWordScoreBin";}
};

/*------------------------------------------------------------------------------
  Cluster Feature Interface - Boilerplate for any feature using a single
                              wordcluster map
 ----------------------------------------------------------------------------*/

class EventTemplateWithClusters : public SparseModel::EventTemplate
{
protected:
   SparseModel::ClusterMap* clusters;   // src voc id -> src cluster id
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    * @param cname name of derived class, for error message
    */
   EventTemplateWithClusters(SparseModel& m, const string& args, const string& cname);
public:
   virtual Uint numEvents()=0;
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids)=0;
   virtual string eventDesc(Uint e)=0;

   virtual void populate(vector<Uint>& event_ids);
   virtual void replaceVoc(Voc* new_voc);

   // do nothing for all recomb fcns, on the assumption that we're covered by natural recomb
   virtual void addContextEvents(const PartialTranslation& context, vector<Uint>& event_ids)
   {/*Intentionally empty*/}
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return false;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual bool implies(EventTemplate& et) {return false;}
   virtual string name() = 0;
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Base class for all disc. dist events
 ----------------------------------------------------------------------------*/
/*
 * Discriminative distortion. All implementations assumes that a generative HLDM is
 * switched on, and therefore enforcing the necessary recombination.
 * Alternatively, the user can use -force-shift-reduce to enable the SR parser
 * and take it into account for recombination.
 * LmBigram and DistPrevTgtWord also assume we have at least one word of
 * LM context considered for recombination, which is indeed always the case.
 */
class DiscriminativeDistortion : public EventTemplateWithClusters
{
protected:
   SparseModel::Distortion* dist;       // maps translation states to orientations
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    * @param cname name of derived class, for error message
    */
   DiscriminativeDistortion(SparseModel& m, const string& args, const string& cname);
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Current Phrase Cluster Representation
 ----------------------------------------------------------------------------*/
class DistCurrentSrcPhrase : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistCurrentSrcPhrase(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual bool isPureDistortion() { return true; }
   virtual string name() { return "DistCurrentSrcPhrase"; }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Current Phrase First Source Word
 ----------------------------------------------------------------------------*/
class DistCurrentSrcFirstWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistCurrentSrcFirstWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual bool isPureDistortion() { return true; }
   virtual string name() { return "DistCurrentSrcFirstWord"; }
};
/*------------------------------------------------------------------------------
  Discriminative distortion - Current Phrase Last Source Word
 ----------------------------------------------------------------------------*/
class DistCurrentSrcLastWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistCurrentSrcLastWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual bool isPureDistortion() { return true; }
   virtual string name() { return "DistCurrentSrcLastWord"; }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Current Phrase First Target Word
 ----------------------------------------------------------------------------*/
class DistCurrentTgtFirstWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistCurrentTgtFirstWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual bool isPureDistortion() { return true; }
   virtual string name() { return "DistCurrentTgtFirstWord"; }
};


/*------------------------------------------------------------------------------
  Discriminative distortion - Current Phrase Last Target Word
 ----------------------------------------------------------------------------*/
class DistCurrentTgtLastWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistCurrentTgtLastWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual bool isPureDistortion() { return true; }
   virtual string name() { return "DistCurrentTgtLastWord"; }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Reduced Phrase First Source Word
 ----------------------------------------------------------------------------*/
class DistReducedSrcFirstWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistReducedSrcFirstWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistReducedSrcFirstWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Reduced Phrase Last Source Word
 ----------------------------------------------------------------------------*/
class DistReducedSrcLastWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistReducedSrcLastWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistReducedSrcLastWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Reduced Phrase Last Target Word
  
  Note that First Target Word is not possible, as after reduction, we will
  not know how the resulting target phrase began.
 ----------------------------------------------------------------------------*/
class DistReducedTgtLastWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistReducedTgtLastWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistReducedTgtLastWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Words in gap between Current and Top Stack
 ----------------------------------------------------------------------------*/
class DistGapSrcWord : public DiscriminativeDistortion
{
   /**
    * Fire only on a Disjoint orientation
    * Return an event for every source word between the current phrase
    * and the top of the stack.
    */
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistGapSrcWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistGapSrcWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Top Stack First Source Word
 ----------------------------------------------------------------------------*/
class DistTopSrcFirstWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistTopSrcFirstWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistTopSrcFirstWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Top Stack Last Source Word
 ----------------------------------------------------------------------------*/
class DistTopSrcLastWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistTopSrcLastWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistTopSrcLastWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative distortion - Previous Target Word
 ----------------------------------------------------------------------------*/
class DistPrevTgtWord : public DiscriminativeDistortion
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "src_clusters" 
    *   src_clusters: class file generated by mkcls for source language
    */
   DistPrevTgtWord(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "DistPrevTgtWord";
   }
};

/*------------------------------------------------------------------------------
  Discriminative LM - Target Unigram
 ----------------------------------------------------------------------------*/
class LmUnigram : public EventTemplateWithClusters
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "tgt_clusters" 
    *   tgt_clusters: class file generated by mkcls for target language
    */
   LmUnigram(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual bool isPure() { return true; }
   virtual string name() {
      return "LmUnigram";
   }
};

/*------------------------------------------------------------------------------
  Discriminative LM - Target Bigram
 ----------------------------------------------------------------------------*/
class LmBigram : public EventTemplateWithClusters
{
public:
   /**
    * Construct.
    * @param m containing model
    * @param args "tgt_clusters" 
    *   tgt_clusters: class file generated by mkcls for target language
    */
   LmBigram(SparseModel& m, const string& args);
   virtual Uint numEvents();
   
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);

   virtual string name() {
      return "LmBigram";
   }
};

/*------------------------------------------------------------------------------
  Phrase pair all bilingual length bins
  ----------------------------------------------------------------------------*/

/**
 * Less flexible, but faster version of PhrasePairLengthBin
 * Will automatically create a bin for each [srclen, tgtlen] pair.
 */

class PhrasePairAllBiLengthBins : public SparseModel::EventTemplate
{
   Uint srcmax;
   Uint tgtmax;

public:
   
   /**
    * Construct.
    * @param m containing model
    * @param args: 'srcmax tgtmax' - phrase length limit in source and target
    */
   PhrasePairAllBiLengthBins(SparseModel& m, const string& args);
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids); 
   virtual void populate(vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);
   
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairAllBiLengthBins";}
};

/*------------------------------------------------------------------------------
  Phrase pair all target length bins
  ----------------------------------------------------------------------------*/

/**
 * Less flexible, but faster version of PhrasePairLengthBin
 * Will automatically create a bin for each tgtlen.
 */

class PhrasePairAllTgtLengthBins : public SparseModel::EventTemplate
{
   Uint tgtmax;
public:
   
   /**
    * Construct.
    * @param m containing model
    * @param args: 'tgtmax' - phrase length limit in target
    */
   PhrasePairAllTgtLengthBins(SparseModel& m, const string& args);
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids); 
   virtual void populate(vector<Uint>& event_ids);
   virtual string eventDesc(Uint e);
   
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairAllTgtLengthBins";}
};

/*------------------------------------------------------------------------------
  Phrase pair all source length bins
  ----------------------------------------------------------------------------*/

/**
 * Less flexible, but faster version of PhrasePairLengthBin
 * Will automatically create a bin for each tgtlen.
 */

class PhrasePairAllSrcLengthBins : public SparseModel::EventTemplate
{
   Uint srcmax;
public:
   
   /**
    * Construct.
    * @param m containing model
    * @param args: 'srcmax' - phrase length limit in source
    */
   PhrasePairAllSrcLengthBins(SparseModel& m, const string& args);
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids); 
   virtual void populate(vector<Uint>& event_ids);
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairAllSrcLengthBins";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  In-quote positions for truecasing. Assumes 1-1 correspondence between source
  and target tokens.
  ----------------------------------------------------------------------------*/

class InQuotePos : public SparseModel::EventTemplate
{
   CaseMapStrings cms;    // util for case handling

   Uint num_bins;
   Uint num_sptypes;            // num_bins x has-stop|not x first|not
   Uint num_events;             // num_sptypes x lc|cap|other
   vector<Uint> bins;           // length -> bin id

   vector<Uint> srcsent; // copy of SparseModel's srcsent, for new-sent detection
   vector<Uint> src_events; // source pos -> source-pos 

public:
   
   /**
    * Construct.
    * @param m containing model
    * @param args - ignored
    */
   InQuotePos(SparseModel& m, const string& args);
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids); 
   virtual void populate(vector<Uint>& event_ids);
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "InQuotePos";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Capture how well a phrase pair matches sentence-level context, as expressed
  by a tag drawn from a fixed, pre-defined inventory.
  ----------------------------------------------------------------------------*/

class PhrasePairContextMatch : public SparseModel::EventTemplate 
{
   Voc tag_voc; // tag <-> tag index
   vector<Uint> tag_freqs;      // tag index -> global freq
   vector<Uint> sent_tags;      // sent id -> tag index
   Uint idcol;                  // index of column in id file
   Uint field;                  // index of field in c= counts for phrase pair
   double pweight;              // weight on prior distn
   Uint numbins;                // number of prob bins = num events

   vector<float> prior;         // prior distn from tag_freqs, * pweight
   vector<float> bins;          // bin -> high boundary

   Uint sid;                    // current source sent id

public:
   /**
    * Construct. 
    * 
    * @param m  Note that the containing model m must have idname set to a file
    * containing a list of id vectors, line-aligned to the current source text.
    * Construct m using the #ID suffix on the model filename.
    *
    * Space-separated parameters in args are: 
    *
    *    tagfile idcol field pweight numbins
    * 
    * @param tagfile file containing a list of all tags with frequencies, in
    * format tag\wfreq, one per line. Frequencies are used to establish prior
    * distn.
    *
    * @param idcol 1-based column index within each vector of tags in idfile to
    * pick out for matching. A warning is generated for each tag not contained
    * in tagfile; a fatal error occurs if no tags are found in tagfile.
    *
    * @param field 1-based field index within phrasetable's c= columns, where
    * each field after the first is introduced by -i+1,  where i is the
    * field's index.  Beware: if you are using multiple cpts, you need to put
    * the -append-joint-counts flag in canoe.ini in order get fields for
    * successive cpts marked like this.  Fields are assumed to contain
    * successive tag,freq pairs, where tag is a 1-based index into the list of
    * tags in tagfile (careful: easy to mess this up).  For example, in
    * c=1,2,-2,20,3,8,1 field 1 contains [1,2], field 2 is empty, and field 3
    * contains [20,3,7,4], indicating that the tag on the 20th line of tagfile
    * occurs 3 times with this phrase pair, and the tag on the 7th line occurs
    * 4 times.
    *
    * @param pweight weight on the prior distn from tagfile in MAP smoothing,
    * eg 10.
    *
    * @param numbins number of probability bins = number of events
    */
   PhrasePairContextMatch(SparseModel& m, const string& args);

   virtual void newSrcSent(const newSrcSentInfo& info) {
      sid = info.external_src_sent_id;
      assert(sid < sent_tags.size());
   }
   virtual void addEvents(const PartialTranslation& hyp, vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids) {
      for (Uint i = 0; i < numbins; ++i) event_ids.push_back(i);
   }
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairContextMatch";}
   virtual string eventDesc(Uint e);
};

/*------------------------------------------------------------------------------
  Assuming conditional probabilities assigned to the current phrase pair by
  phrase tables from different sub-corpora are ranked in descending order, this
  feature captures which sub-corpora fall into a given rank range, for a given
  column of conditional probability estimates. It assumes that rank information
  is coded in the c= field in the format written by gen_tm_features -c.
  ----------------------------------------------------------------------------*/

class PhrasePairSubCorpRank : public SparseModel::EventTemplate {

   Uint ncpts;
   Uint col;  // column index (0-based)
   Uint low;  // low rank
   Uint high; // high rank

public:

   /**
   * Construct.
   * @param m containing model
   * @param args: 'ncpts col lo-hi', where ncpts is the number of cpts to which
   * ranks apply, col is a 1-based column index, and lo-hi is a rank range. Eg:
   * '10 1 1-1'  means pick out the top-ranked cpt out of 10 possible cpts
   *             for column 1
   * '10 1 1-10' means pick out all cpts listed (for many phrase pairs this
   *             won't be ALL possible cpts, since the coding format can
   *             optionally list only high-prob cpts - see gen_tm_features -h).
   * '10 1 2-4'  pick out cpts whose probabilities rank 2nd to 4th
   */
   PhrasePairSubCorpRank(SparseModel& m, const string& args);

   virtual void addEvents(const PartialTranslation& hyp, 
                          vector<Uint>& event_ids);
   virtual void populate(vector<Uint>& event_ids) {
      for (Uint i = 0; i < ncpts; ++i) event_ids.push_back(i);
   }
   virtual bool isDirect() {return true;}
   virtual bool isPure() {return true;}
   virtual Uint remapEvent(Uint id, Voc* new_voc) {return id;}
   virtual string name() {return "PhrasePairSubCorpRank";}
   virtual string eventDesc(Uint e);
};
}

#endif // SPARSEMODEL_H
