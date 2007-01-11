/**
 * @author Nicola Ueffing
 * @file confidence_score.cc  Implementation of confidence score (ConfScore).
 *
 * $Id$
 *
 * COMMENTS: class for storing word confidence measures calculated over N-best lists
 * contains word posterior probability, rank weighted frequency, and relative frequency
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Conseil national de recherches du Canada / Copyright 2006, National Research Council of Canada
*/

#include <cmath>
#include "confidence_score.h"
#include <stdio.h>

using namespace std;
using namespace Portage;

void ConfScore::reset() {
  wpp   = INFINITY;
  rankf = 0;
  relf  = 0;
}

void ConfScore::update(const double &p, const uint &n) {
  if (wpp == INFINITY) wpp = p;
  //  else wpp   = log(exp(wpp) + exp(p));
  else wpp  += log(1 + exp(p-wpp));
  rankf     += n;
  relf++;
//  cerr << "update with prob. " << p << "/" << exp(p) << " leads to " << wpp << "/" << exp(wpp) << endl;
}

void ConfScore::update(const ConfScore &conf) {
  if (wpp == INFINITY) wpp = conf.wpp;
  //  else wpp   = log(exp(wpp) + exp(conf.wpp));
  else wpp  += log(1 + exp(conf.wpp-wpp));
  rankf     += conf.rankf;
  relf      += conf.relf;
//  cerr << "update with prob. " << conf.wpp << "/" << exp(conf.wpp) << " leads to " << wpp << "/" << exp(wpp) << endl;
}

void ConfScore::normalize(const double &t, const uint &N) {
  wpp     -= t;
  rankf    = 2.0*(relf*double(N)-rankf)/(double)(N*(N+1));
  relf    /= (double)N;
//  cerr << "norm. with prob. " << t << "/" << exp(t) << " leads to " << wpp << "/" << exp(wpp) << endl;
}

void ConfScore::normalize(const ConfScore &c) {
  wpp     -= c.wpp;
  rankf    = 2.0*(relf*double(c.relf)-rankf)/(double)(c.relf*(c.relf+1));
  relf    /= (double)c.relf;
//  cerr << "norm. with prob. " << c.wpp << "/" << exp(c.wpp) << " leads to " << wpp << "/" << exp(wpp) << endl;
}

double ConfScore::prob() const {
  return wpp;
}

double ConfScore::rank() const {
  return rankf;
}

double ConfScore::relfreq() const {
  return relf;
}
