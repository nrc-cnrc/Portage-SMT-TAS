/**
 * @author Samuel Larkin
 * @file test_toJSON.h
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include <sstream>
#include "portage_defs.h"
#include "canoe_general.h"
#include "marked_translation.h"
#include "voc.h"
#include "phrasedecoder_model.h"
#include "new_src_sent_info.h"

using namespace Portage;

namespace Portage {

class TestCanoeToJSON : public CxxTest::TestSuite
{
public:
   void testRange() {
      Range r(7, 9);
      const string goldStd = "{\"start\":7,\"end\":9}";

      {
         ostringstream oss;
         r.toJSON(oss);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         ostringstream oss;
         oss << to_JSON(r);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testMarkTranslation() {
      MarkedTranslation m;
      m.src_words.start = 4;
      m.src_words.end   = 8;
      m.markString.push_back("Hello");
      m.markString.push_back("World");
      m.markString.push_back("!!!");
      m.log_prob = -18;
      m.class_name = "Unittest";

      const string goldStd = "{\"src_words\":{\"start\":4,\"end\":8},\"log_prob\":-18,\"markString\":[\"Hello\",\"World\",\"!!!\"],\"class_name\":\"Unittest\"}";

      {
         ostringstream oss;
         m.toJSON(oss);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         ostringstream oss;
         oss << to_JSON(m);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testPhrase() {
      Voc v;
      Phrase p;
      p.push_back(v.add("Hello"));
      p.push_back(v.add("World"));
      p.push_back(v.add("!!!"));

      const string goldStd = "[\"Hello\",\"World\",\"!!!\"]";

      {
         ostringstream oss;
         to_JSON(oss, p, v);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testPhraseInfo() {
      Voc v;
      PhraseInfo pi;
      pi.src_words.start = 1;
      pi.src_words.end   = 10;
      pi.phrase.push_back(v.add("Hello"));
      pi.phrase.push_back(v.add("World"));
      pi.phrase.push_back(v.add("!!!"));
      pi.phrase_trans_prob = -1;
      pi.phrase_trans_probs.push_back(-10);
      pi.phrase_trans_probs.push_back(-11);
      pi.forward_trans_prob = -2;
      pi.forward_trans_probs.push_back(-20);
      pi.forward_trans_probs.push_back(-21);
      pi.adir_prob = -3;
      pi.adir_probs.push_back(-30);
      pi.adir_probs.push_back(-31);
      pi.lexdis_probs.push_back(-40);
      pi.partial_score = -17;

      // Without Voc.
      {
         const string goldStd = "{\"src_words\":{\"start\":1,\"end\":10},\"phrase\":[0,1,2],\"phrase_trans_prob\":-1,\"phrase_trans_probs\":[-10,-11],\"forward_trans_prob\":-2,\"forward_trans_probs\":[-20,-21],\"adir_prob\":-3,\"adir_probs\":[-30,-31],\"lexdis_probs\":[-40],\"partial_score\":-17}";
         ostringstream oss;
         pi.toJSON(oss);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         const string goldStd = "{\"src_words\":{\"start\":1,\"end\":10},\"phrase\":[\"Hello\",\"World\",\"!!!\"],\"phrase_trans_prob\":-1,\"phrase_trans_probs\":[-10,-11],\"forward_trans_prob\":-2,\"forward_trans_probs\":[-20,-21],\"adir_prob\":-3,\"adir_probs\":[-30,-31],\"lexdis_probs\":[-40],\"partial_score\":-17}";
         ostringstream oss;
         pi.toJSON(oss, &v);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         const string goldStd = "{\"src_words\":{\"start\":1,\"end\":10},\"phrase\":[\"Hello\",\"World\",\"!!!\"],\"phrase_trans_prob\":-1,\"phrase_trans_probs\":[-10,-11],\"forward_trans_prob\":-2,\"forward_trans_probs\":[-20,-21],\"adir_prob\":-3,\"adir_probs\":[-30,-31],\"lexdis_probs\":[-40],\"partial_score\":-17}";
         ostringstream oss;
         oss << to_JSON(pi, &v);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testSrcWall() {
      SrcWall sw(4, "unittest src wall");

      const string goldStd = "{\"pos\":4,\"name\":\"unittest src wall\"}";

      {
         ostringstream oss;
         sw.toJSON(oss);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         ostringstream oss;
         oss << to_JSON(sw);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testSrcZone() {
      SrcZone sz(Range(7,11), "unittest src zone");

      const string goldStd = "{\"range\":{\"start\":7,\"end\":11},\"name\":\"unittest src zone\"}";

      {
         ostringstream oss;
         sz.toJSON(oss);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         ostringstream oss;
         oss << to_JSON(sz);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testSrcLocalWall() {
      SrcLocalWall slw(6, "unittest src local wall");
      slw.zone.start = 7;
      slw.zone.end = 11;

      const string goldStd = "{\"pos\":6,\"zone\":{\"start\":7,\"end\":11},\"name\":\"unittest src local wall\"}";

      {
         ostringstream oss;
         slw.toJSON(oss);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }

      {
         ostringstream oss;
         oss << to_JSON(slw);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
      }
   }

   void testNesSrcSentInfo() {
      Voc v;

      PhraseInfo pi;
      pi.src_words.start = 1;
      pi.src_words.end   = 10;
      pi.phrase.push_back(v.add("Hello"));
      pi.phrase.push_back(v.add("World"));
      pi.phrase.push_back(v.add("!!!"));
      pi.phrase_trans_prob = -1;
      pi.phrase_trans_probs.push_back(-10);
      pi.phrase_trans_probs.push_back(-11);
      pi.forward_trans_prob = -2;
      pi.forward_trans_probs.push_back(-20);
      pi.forward_trans_probs.push_back(-21);
      pi.adir_prob = -3;
      pi.adir_probs.push_back(-30);
      pi.adir_probs.push_back(-31);
      pi.lexdis_probs.push_back(-40);
      pi.partial_score = -17;

      MarkedTranslation m;
      m.src_words.start = 4;
      m.src_words.end   = 8;
      m.markString.push_back("Hello");
      m.markString.push_back("World");
      m.markString.push_back("!!!");
      m.log_prob = -18;
      m.class_name = "Unittest";

      vector<string> tgt_sent;
      tgt_sent.push_back("Bonjour");
      tgt_sent.push_back("a tous!");

      vector<bool> oovs;
      oovs.push_back(true);
      oovs.push_back(true);
      oovs.push_back(false);

      SrcWall sw(4, "unittest src wall");

      SrcZone sz(Range(7,11), "unittest src zone");

      SrcLocalWall slw(6, "unittest src local wall");
      slw.zone.start = 7;
      slw.zone.end = 11;

      newSrcSentInfo nssi;
      nssi.internal_src_sent_seq = 4;
      nssi.external_src_sent_id = 5;
      nssi.src_sent.push_back("Hello");
      nssi.src_sent.push_back("World");
      nssi.src_sent.push_back("!!!");
      nssi.marks.push_back(m);
      nssi.src_sent_tags.push_back("A");
      nssi.src_sent_tags.push_back("B");
      nssi.tgt_sent = &tgt_sent;
      nssi.tgt_sent_ids.push_back(37);
      nssi.tgt_sent_ids.push_back(40);
      nssi.oovs = &oovs;
      nssi.walls.push_back(sw);
      nssi.zones.push_back(sz);
      nssi.local_walls.push_back(slw);

      nssi.potential_phrases = TriangArray::Create<vector<PhraseInfo *> >()(nssi.src_sent.size());
      nssi.potential_phrases[0][0].push_back(&pi);

      {
         const string goldStd = "{\"internal_src_sent_seq\":4,\"external_src_sent_id\":5,\"src_sent\":[\"Hello\",\"World\",\"!!!\"],\"marks\":[{\"src_words\":{\"start\":4,\"end\":8},\"log_prob\":-18,\"markString\":[\"Hello\",\"World\",\"!!!\"],\"class_name\":\"Unittest\"}],\"src_sent_tags\":[\"A\",\"B\"],\"tgt_sent\":[\"Bonjour\",\"a tous!\"],\"tgt_sent_ids\":[37,40],\"oovs\":[true,true,false],\"walls\":[{\"pos\":4,\"name\":\"unittest src wall\"}],\"zones\":[{\"range\":{\"start\":7,\"end\":11},\"name\":\"unittest src zone\"}],\"local_walls\":[{\"pos\":6,\"zone\":{\"start\":7,\"end\":11},\"name\":\"unittest src local wall\"}],\"potential_phrases\":[[[{\"src_words\":{\"start\":1,\"end\":10},\"phrase\":[\"Hello\",\"World\",\"!!!\"],\"phrase_trans_prob\":-1,\"phrase_trans_probs\":[-10,-11],\"forward_trans_prob\":-2,\"forward_trans_probs\":[-20,-21],\"adir_prob\":-3,\"adir_probs\":[-30,-31],\"lexdis_probs\":[-40],\"partial_score\":-17}],[],[]],[[],[]],[[]]]}";
         ostringstream oss;
         nssi.toJSON(oss, &v);
         TS_ASSERT_EQUALS(oss.str(), goldStd);
         //cerr << oss.str() << endl;  // SAM DEBUGGING / VALIDATION with jq.
      }

      TriangArray::Delete<vector<PhraseInfo *> >()(nssi.potential_phrases, nssi.src_sent.size());
   }

}; // TestCanoeToJSON

} // Portage
