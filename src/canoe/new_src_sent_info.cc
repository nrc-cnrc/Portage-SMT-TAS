#include "new_src_sent_info.h"
#include "phrasedecoder_model.h"
#include "str_utils.h"
#include "toJSON.h"

ostream& SrcWall::toJSON(ostream& out) const {
   out << '{';
   out << to_JSON("pos", pos);
   out << ',';
   out << to_JSON("name", name);
   out << '}';
   return out;
}

ostream& SrcZone::toJSON(ostream& out) const {
   out << '{';
   out << to_JSON("range", range);
   out << ',';
   out << to_JSON("name", name);
   out << '}';
   return out;
}

ostream& SrcLocalWall::toJSON(ostream& out) const {
   out << '{';
   out << to_JSON("pos", pos);
   out << ',';
   out << to_JSON("zone", zone);
   out << ',';
   out << to_JSON("name", name);
   out << '}';
   return out;
}

ostream& newSrcSentInfo::toJSON(ostream& out, Voc const * const voc) const
{
   out << "{";

   out << to_JSON("internal_src_sent_seq", internal_src_sent_seq);
   out << ',';
   out << to_JSON("external_src_sent_id", external_src_sent_id);
   out << ',';
   out << to_JSON("src_sent", src_sent);
   out << ',';
   out << to_JSON("marks", marks);
   out << ',';
   out << to_JSON("src_sent_tags", src_sent_tags);
   if (tgt_sent != NULL) {
      out << ',';
      out << to_JSON("tgt_sent", *tgt_sent);
   }
   out << ',';
   out << to_JSON("tgt_sent_ids", tgt_sent_ids);

   if (oovs != NULL) {
      out << ',';
      out << to_JSON("oovs", *oovs);
   }

   out << ',';
   out << to_JSON("walls", walls);
   out << ',';
   out << to_JSON("zones", zones);
   out << ',';
   out << to_JSON("local_walls", local_walls);

   out << ',';
   out << keyJSON("potential_phrases");
   out << '[';
   for ( Uint i = 0; i < src_sent.size(); ++i ) {
      if (i>0) out << ',';
      out << '[';
      for ( Uint j = i; j < src_sent.size(); ++j ) {
         if (j!=i) out << ',';
         const vector<PhraseInfo*>& ph = potential_phrases[i][j-i];
         // Let's write the phraseInfo's phrase with words instead of ints.
         out << to_JSON(ph, voc);
      }
      out << ']';
   }
   out << ']';
   out << "}";

   return out;
}
