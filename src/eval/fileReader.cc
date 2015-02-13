/**
 * @author Aaron Tikuisis
 * @file fileReader.cc  Implementation of objects that transparently allow
 *                      reading in fix block size or in dynamic sized blocks.
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
#include <fileReader.h>
#include <str_utils.h>

using namespace Portage;
using namespace Portage::FileReader;
using namespace std;



////////////////////////////////////////
// BASE CLASS
template<class T>
FileReaderBase<T>::FileReaderBase(const string& szFileName, Uint K)
: m_file(szFileName)
, m_K(K)
, m_nGroupNo(0)
{
   if (!m_file)
   {
      error(ETFatal, "Can't open %s", szFileName.c_str());
   }
}


template<class T>
FileReaderBase<T>::~FileReaderBase()
{
   //m_file.close();
}


////////////////////////////////////////
// FIX CLASS
template<class T>
FixReader<T>::FixReader(const string& szFileName, Uint K)
: Parent(szFileName, K)
, m_nSentNo(0)
{}


template<class T>
FixReader<T>::~FixReader()
{}


template<class T>
bool FixReader<T>::poll(T& s, Uint* groupId)
{
   if (groupId != NULL) *groupId = Parent::m_nGroupNo;
   ++m_nSentNo;
   if (m_nSentNo == Parent::m_K) {
      m_nSentNo = 0;
      ++Parent::m_nGroupNo;
   }

   return getline(Parent::m_file, s) && Parent::pollable();
}


template<class T>
bool FixReader<T>::poll(Group& g, Uint* groupId)
{
   g.clear();
   g.reserve(Parent::m_K);
   if (groupId) *groupId = Parent::m_nGroupNo;
   for (Uint k(0); k<Parent::m_K; ++k)
   {
      g.push_back(T());
      poll(g.back());
   }

   return Parent::pollable();
}



////////////////////////////////////////
// DYNAMIC CLASS
template<class T>
DynamicReader<T>::DynamicReader(const string& szFileName, Uint K)
: Parent(szFileName, K)
{
   Parent::m_file >> Parent::m_nGroupNo;
}


template<class T>
DynamicReader<T>::~DynamicReader()
{}


template<class T>
bool DynamicReader<T>::poll(T& s, Uint* groupId)
{
   const Uint current = Parent::m_nGroupNo;
   if (groupId != NULL) *groupId = Parent::m_nGroupNo;

   bool bRetour((Parent::m_file.get() == '\t')
                && (getline(Parent::m_file, s))
                && (Parent::m_file >> Parent::m_nGroupNo));  // Reads next translation ID.

   // Parent::m_nGroupNo holds the next sentence number
   return bRetour && (Parent::m_nGroupNo == current);
}


template<class T>
bool DynamicReader<T>::poll(Group& g, Uint* groupId)
{
   if (groupId) *groupId = Parent::m_nGroupNo;
   g.clear();
   g.reserve(Parent::m_K);
   do
   {
      g.push_back(T());
   }
   while (poll(g.back()));

   g.resize(g.size());

   return Parent::m_file;
}



////////////////////////////////////////
// FACTORY FOR FILE READER
template<class T>
std::auto_ptr<FileReaderBase<T> > FileReader::create(const string& szFileName, Uint K)
{
   if (K == 0)
   {
      //LOG_VERBOSE3(myLogger, "Using Dynamic File Reader");
      return std::auto_ptr<FileReaderBase<T> >(new DynamicReader<T>(szFileName, K));
   }
   else
   {
      //LOG_VERBOSE3(myLogger, "Using Fix File Reader");
      return std::auto_ptr<FileReaderBase<T> >(new FixReader<T>(szFileName, K));
   }
}
