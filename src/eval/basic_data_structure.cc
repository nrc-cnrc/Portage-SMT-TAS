/**
 * @author Michel Simard
 * @file basic_data_structure.cc  Implementation of the eval basic type functionalities.
 *
 * $Id$
 *
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
*/

#include <basic_data_structure.h>
#include <errors.h>
#include <str_utils.h>

using namespace Portage;

Nbest::Nbest(const vector<string>& nbest) {
  reserve(nbest.size());
  typedef vector<string>::const_iterator IT;
  for (IT it(nbest.begin()); it!=nbest.end(); ++it)
    push_back(Translation(it->c_str()));
}



bool
Alignment::read(istream &in) {
  string line;
  vector<string> pairs;
  vector<Uint> data;

  clear();			// We assume it's a new alignment!!!

  if (!getline(in, line))	// EOF: return false
    return false;

  // Otherwise, go on:
  pairs.clear();
  split(line, pairs);

  for (vector<string>::iterator p = pairs.begin(); p != pairs.end(); p++) {
    data.clear();
    split((*p), data, ":-");
    if (data.size() != 5) {
      error(ETFatal, "Something went wrong while reading alignments...\n");
      return false;
    }
    push_back(AlignedPair(data[1], data[2], data[3], data[4])); // 0 is just a count
  }
  sortOnTarget();
  
  return true;
}

void
Alignment::write(ostream &out) {
  for (Uint i = 0; i < size(); i++)
    out << (i ? " " : "") 
	<< i 
	<< ':' << (*this)[i].source.first << '-' << (*this)[i].source.last
	<< ':' << (*this)[i].target.first << '-' << (*this)[i].target.last;
}

