/**
 * @author Eric Joanis
 * @file count_annotation.cc
 *
 * Implements the annotation used to store the count field in a phrase table.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "count_annotation.h"
#include "phrasetable.h"
#include "str_utils.h"

using namespace Portage;

const string CountAnnotation::name = "c";
bool CountAnnotation::appendJointCounts = false;
const PhraseTable* CountAnnotation::phraseTable = NULL;
static const char* count_sep = ",";

void CountAnnotation::setAppendJointCountsMode(const PhraseTable* _phraseTable) {
   appendJointCounts = true;
   phraseTable = _phraseTable;
   assert(phraseTable);
}

void CountAnnotation::parseCounts(const char* value)
{
   // count the number of fields up front, to reserve the right size for joint_counts
   Uint num_counts = 1;
   for (const char* c = value; *c; ++c)
      if (*c == ',')
         ++num_counts;

   if (appendJointCounts) {
      const Uint numTextFilesRead = phraseTable->getNumTextFilesRead();
      joint_counts.reserve(joint_counts.size() + num_counts + (numTextFilesRead?1:0));
      if (numTextFilesRead)     // insert field start marker
         joint_counts.push_back(-static_cast<float>(numTextFilesRead));
   } else {
      joint_counts.reserve(num_counts);
   }

   // destructively parse the string in a local copy
   char buffer[strlen(value)+1];
   strcpy(buffer, value);
   Uint nc = 0;
   char* strtok_state;
   for (char* pch = strtok_r(buffer, count_sep, &strtok_state); pch;
        pch = strtok_r(NULL, count_sep, &strtok_state), ++nc) {
      float count_val(0);
      if (!conv(pch, count_val))
         error(ETFatal, "Invalid joint_freq count value (%s) in count string c=%s.",
               pch, value);
      assert(nc <= joint_counts.size());
      if (appendJointCounts || nc == joint_counts.size())
         joint_counts.push_back(count_val);
      else
         joint_counts[nc] += count_val;
   }
}

CountAnnotation::CountAnnotation(const char* value)
{
   parseCounts(value);
}

void CountAnnotation::updateValue(const char* value)
{
   if (!joint_counts.empty() && !appendJointCounts) {
      static bool warningDisplayed = false;
      if (!warningDisplayed) {
         warningDisplayed = true;
         error(ETWarn, "Duplicate joint count(s) information found - Adding counts together. "
               "(This message will only be printed once even if there are many cases.)");
      }
   }
   
   parseCounts(value);
}

void CountAnnotation::display(ostream& out) const
{
   out << "\tjoint count(s)        " << join(joint_counts) << endl;
}

void CountAnnotation::write(ostream& phrase_table_file) const
{
   write_helper(phrase_table_file, name, join(joint_counts, count_sep));
}
