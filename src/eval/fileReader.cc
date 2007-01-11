/**
 * @author Aaron Tikuisis
 * @file fileReader.cc  Implementation of objects that transparently allow reading in fix block size or in dynamic sized blocks.
 *
 * $Id$
 *
 * Evaluation Module
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */
#include <fileReader.h>
#include <str_utils.h>
#include <logging.h>

using namespace Portage;
using namespace Portage::FileReader;
using namespace std;

static Logging::logger myLogger(Logging::getLogger("verbose.dynamicsizereader"));


////////////////////////////////////////
// BASE CLASS
template<class T>
FileReaderBase<T>::FileReaderBase(const string& szFileName, const Uint& S, const Uint& K) throw(InvalidFileName)
: m_file(szFileName)
, m_S(S)
, m_K(K)
, m_nSentNo(0)
{
   if (!m_file)
   {
      throw InvalidFileName(szFileName);
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
FixReader<T>::FixReader(const string& szFileName, const Uint& S, const Uint& K)
: Parent(szFileName, S, K)
{}


template<class T>
FixReader<T>::~FixReader()
{}


template<class T>
bool FixReader<T>::poll(Uint& index, string& s)
{
   index = Parent::m_nSentNo++;
   if (Parent::m_nSentNo == Parent::m_K) Parent::m_nSentNo = 0;

   getline(Parent::m_file, s);

   return Parent::pollable();
}


template<class T>
bool FixReader<T>::poll(Group& g)
{
   g.clear();
   g.reserve(Parent::m_K);
   Uint bidon(0);
   for (Uint k(0); k<Parent::m_K; ++k)
   {
      g.push_back(T());
      poll(bidon, g.back());
   }

   return Parent::pollable();
}


/*template<class T>
bool FixReader<T>::poll(matrixCandidate& m)
{
   m.clear();
   m.reserve(m_S);
   for (Uint s(0); s<m_S; ++s)
   {
      m.push_back(groupCandidate());
      poll(m.back());
   }

   return pollable();
}*/


////////////////////////////////////////
// DYNAMIC CLASS
template<class T>
DynamicReader<T>::DynamicReader(const string& szFileName, const Uint& S, const Uint& K)
: Parent(szFileName, S, 1000)
{
   Parent::m_file >> Parent::m_nSentNo;
   --Parent::m_nSentNo;
}


template<class T>
DynamicReader<T>::~DynamicReader()
{}


template<class T>
bool DynamicReader<T>::poll(Uint& index, string& s)
{
   index = Parent::m_nSentNo;

   bool bRetour((Parent::m_file.get() == '\t') && (getline(Parent::m_file, s)) && (Parent::m_file >> Parent::m_nSentNo));
   --Parent::m_nSentNo;

   return bRetour && (Parent::m_nSentNo == index);
}


template<class T>
bool DynamicReader<T>::poll(Group& g)
{
   g.clear();
   g.reserve(Parent::m_K);
   Uint bidon(0);
   do
   {
      g.push_back(T());
   }
   while (poll(bidon, g.back()));

   g.resize(g.size());

   return Parent::m_file;
}


/*template<class T>
bool DynamicReader<T>::poll(matrixCandidate& m)
{
   m.clear();
   m.reserve(m_S);
   do
   {
      m.push_back(groupCandidate());
   }
   while (poll(m.back()));

   m.resize(m.size());

   return true;
}*/


////////////////////////////////////////
// FACTORY FOR FILE READER
template<class T>
std::auto_ptr<FileReaderBase<T> > FileReader::create(const string& szFileName, const Uint& S, const Uint& K)
{
   if (K == 0)
   {
      LOG_VERBOSE3(myLogger, "Using Dynamic File Reader");
      return std::auto_ptr<FileReaderBase<T> >(new DynamicReader<T>(szFileName, S, K));
   }
   else
   {
      LOG_VERBOSE3(myLogger, "Using Fix File Reader");
      return std::auto_ptr<FileReaderBase<T> >(new FixReader<T>(szFileName, S, K));
   }
}
