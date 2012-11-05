/**
 * @author Eric Joanis
 * @file alignment_file.cc - Structure to keep track of a word-alignment file
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include "alignment_file.h"
#include "word_align_io.h"
#include "tp_alignment.h"
#include "line_indexed_file.h"

using namespace Portage;

AlignmentFile* AlignmentFile::create(const string& filename)
{
   if (TPAlignment::isTPAlignmentFile(filename))
      return new TPAlignment(filename);
   else
      return new GreenAlignmentFile(filename);
}

GreenAlignmentFile::GreenAlignmentFile(const string& filename)
   : AlignmentFile(filename)
{
   file = new LineIndexedFile(filename);
   assert(file);
}

GreenAlignmentFile::~GreenAlignmentFile() {
   delete file;
}

Uint GreenAlignmentFile::size() const
{
   return file->size();
}

bool GreenAlignmentFile::get(Uint i, vector< vector<Uint> >& sets) const
{
   sets.clear();
   if (i >= file->size()) return false;
   string line = file->get(i);
   GreenReader()(line, sets);
   return true;
}
