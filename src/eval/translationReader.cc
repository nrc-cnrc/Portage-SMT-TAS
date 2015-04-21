/**
 * @author Samuel Larkin
 * @file fileReader.cc  Implementation of objects that transparently allow
 *                      reading in fix block size or in dynamic sized blocks.
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */
#include <translationReader.h>
#include <str_utils.h>

using namespace Portage;
using namespace Portage::FileReader;
using namespace std;


////////////////////////////////////////
// FIX CLASS
FixTranslationReader::FixTranslationReader(const string& szFileName, const string& szAlignment, Uint K)
: Parent(szFileName, K)
, m_alignment(szAlignment.c_str())
{
   if (!m_alignment) error(ETFatal, "Can't open alignment %s", szFileName.c_str());
}


FixTranslationReader::~FixTranslationReader()
{
   if (hasMore())
      error(ETFatal, szIncompleteAlignmentRead);

   if (Parent::pollable())
      error(ETFatal, szIncompleteTranslationRead);
}


bool FixTranslationReader::pollable() const {
   return Parent::pollable() && hasMore();
}


bool FixTranslationReader::poll(Translation& s, Uint* groupId)
{
   if (hasMore())
      s.phraseAlignment.read(m_alignment);
   else
      error(ETFatal, szNotEnoughAlignments);

   return Parent::poll(s, groupId);
}



////////////////////////////////////////
// DYNAMIC CLASS
DynamicTranslationReader::DynamicTranslationReader(const string& szFileName, const string& szAlignment, Uint K)
: Parent(szFileName, K)
, m_alignment(szAlignment.c_str())
{
   if (!m_alignment) error(ETFatal, "Can't open alignment %s", szFileName.c_str());
   m_alignment >> m_nalignmentNo;
}


DynamicTranslationReader::~DynamicTranslationReader()
{
   if (hasMore()) error(ETFatal, szIncompleteAlignmentRead);

   if (Parent::pollable()) error(ETFatal, szIncompleteTranslationRead);
}


bool DynamicTranslationReader::pollable() const {
   return Parent::pollable() && hasMore();
}


bool DynamicTranslationReader::poll(Translation& s, Uint* groupId)
{
   const Uint currentAlignment = m_nalignmentNo;
   bool rAlignment = false;
   if (hasMore()) {
      rAlignment = ((m_alignment.get() == '\t')
                    && s.phraseAlignment.read(m_alignment)
                    && (m_alignment >> m_nalignmentNo));  // Reads next phrase alignment id.
   }
   else
      error(ETFatal, szNotEnoughAlignments);

   Uint currentTranslation = 0;
   const bool rTranslation = Parent::poll(s, &currentTranslation);

   if (currentAlignment != currentTranslation)
      error(ETFatal, szMisaligned);

   if (groupId != NULL) *groupId = currentAlignment;

   return rAlignment && rTranslation;
}



////////////////////////////////////////
// FACTORY FOR FILE READER
NbestReader FileReader::createT(const string& szFileName, const string& szAlignment, Uint K)
{
   if (szAlignment.empty())  return create<Translation>(szFileName, K);

   if (K == 0)
   {
      //LOG_VERBOSE3(myLogger, "Using Dynamic Translation Reader");
      return NbestReader(new DynamicTranslationReader(szFileName, szAlignment, K));
   }
   else
   {
      //LOG_VERBOSE3(myLogger, "Using Fix Translation Reader");
      return NbestReader(new FixTranslationReader(szFileName, szAlignment, K));
   }
}
