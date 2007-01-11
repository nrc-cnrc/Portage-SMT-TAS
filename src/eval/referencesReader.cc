/**
 * @author Samuel Larkin
 * @file referencesReader.cc  Implementation of references reader
 *
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches du Canada / Copyright 2006, National Research Council of Canada
 */

#include <referencesReader.h>
#include <errors.h>
#include <file_utils.h>

using namespace Portage;

referencesReader::referencesReader(const vector<string>& sRefFiles)
: m_R(sRefFiles.size())
{
   if (m_R == 0) error(ETFatal, "No Reference files");
   m_ifRefs.resize(m_R);
   for (unsigned int r(0); r<m_R; ++r)
   {
      m_ifRefs[r] = new IMagicStream(sRefFiles[r].c_str());
      assert(m_ifRefs[r]);
      if (m_ifRefs[r]->fail())
         error(ETFatal, "Unable to open reference file: %s", sRefFiles[r].c_str());
   }
}


referencesReader::~referencesReader()
{
   typedef vector<istream*>::iterator viit;
   for (viit it(m_ifRefs.begin()); it!=m_ifRefs.end(); ++it)
   {
      delete (*it);
   }
}


void referencesReader::poll(References& gr)
{
   gr.clear();
   gr.resize(m_R);

   for (unsigned int r(0); r<m_R; ++r)
   {
      istream& myStream(*m_ifRefs[r]);
      if (myStream.eof()) error(ETFatal, "Premature end of reference file(%d)", r);
      getline(myStream, gr[r]);
   }
}


void referencesReader::poll(AllReferences& mr, const unsigned int S)
{
   if (S == 0) error(ETFatal, "in references reader S == 0");
   mr.clear();
   mr.resize(S);
   for (unsigned int s(0); s<S; ++s)
   {
      poll(mr[s]);
   }
}


void referencesReader::integrityCheck(bool bAfterAWhile)
{
   References gr;
   if (!bAfterAWhile) poll(gr);
   for (unsigned int r(0); r<m_R; ++r)
   {
      if (!m_ifRefs[r]->eof()) error(ETFatal, "Number of lines is not consistent in reference files #%d", r);
   }
}
