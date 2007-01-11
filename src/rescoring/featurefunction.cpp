/**
 * @author Aaron Tikuisis / George Foster
 * @file featurefunction.cpp
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#include <featurefunction.h>
#include <rescoring_general.h>
#include <ibm1aaron.h>
#include <rescore_io.h>
#include <errors.h>
#include <str_utils.h>
#include <fileReader.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <lm_ff.h>
#include <ibm_ff.h>
#include <ibm1del.h>

namespace Portage {

/// Callable entity that helps managing empty hypotheses.
class isEmptyLine
{
   private:
      Uint nEmpty;       ///< Number of empty hypotheses found.
      Uint nContiguous;  ///< last number of contiguous empty hypotheses.
  
   public:
      /// Constructor.
      isEmptyLine() : nEmpty(0), nContiguous(0) {}
      
      /// Checks if the empty line were all contiguous and at the end.
      /// @return Returns true if all contiguous empty hypotheses are at the end.
      bool allContiguous() const { return nEmpty == nContiguous; }
      /// Getsthe number of empty hypotheses found.
      /// @return Returns the number of empty hypotheses found.
      Uint numberOfEmpty() const { return nEmpty; }
      
      /// Counts the empty translations and the contiguous one.
      /// @param t  hypothesis / empty hypothesis.
      /// @return Returns true if t is empty.
      bool operator()(const Translation& t) {
         const bool isEmpty = t.empty();
         if (isEmpty) {
            ++nEmpty;
            ++nContiguous;
         }
         else
            nContiguous = 0;

         return isEmpty;
      }
};

void writeFFMatrix(ostream &out, const vector<uMatrix>& vH)
{
   assert(!!out);

   out << vH.size() << endl;
   out << setprecision(10);
   typedef vector<uMatrix>::const_iterator IT;
   for (IT it(vH.begin()); it!=vH.end(); ++it) {
      out << *it << endl;
   }
}

void readFFMatrix(istream &in, vector<uMatrix>& vH)
{
   assert(!!in);

   Uint S(0);
   in >> S;
   vH.reserve(S);
   for (Uint s(0); s<S; ++s) {
      vH.push_back(uMatrix());
      in >> vH.back();
   }
}


//---------------------------------------------------------------------------------------
// FileFF
//---------------------------------------------------------------------------------------

FileFF::FileFF(const string& filespec)
: m_column(0)
{
   string::size_type idx = filespec.rfind(',');
   m_filename = filespec.substr(0, idx);
   
   if (idx != string::npos) {
      string coldesc = filespec.substr(idx+1);
      if (!conv(coldesc, m_column))
         error(ETFatal, "can't convert column spec %s to a number", coldesc.c_str());
      --m_column;  // Convert to 0 based index
   }

   m_info = multiColumnFileFFManager::getManager().getUnit(m_filename);
   assert(m_info != 0);
}

//---------------------------------------------------------------------------------------
// FileDFF
//---------------------------------------------------------------------------------------

FileDFF::FileDFF(const string& filespec)
{
   string::size_type idx = filespec.rfind(',');
   m_filename = filespec.substr(0, idx);

   Uint col(0);
   if (idx != string::npos)
   {
      string coldesc = filespec.substr(idx+1);
      if (!conv(coldesc, col))
         error(ETFatal, "can't convert column spec %s to a number", coldesc.c_str());
   }

   vector<string> fields;
   FileReader::DynamicReader<string> dr(m_filename, 1, 1);
   vector<string> gc;
   while (dr.pollable())
   {
      dr.poll(gc);
      const Uint K(gc.size());
      m_vals.push_back(vector<double>(K, 0.0f));
      for (Uint k(0); k<K; ++k)
      {
         string* convertme = &gc[k];
         if (col)
         {
            fields.clear();
            split(gc[k], fields, " \t\n\r");
            convertme = &fields[col-1];
         }

         if (!conv(*convertme, m_vals.back()[k]))
            error(ETFatal, "can't convert value to double: %s", (*convertme).c_str());
      }
   }
}

void FileDFF::source(Uint s, Nbest * const nbest)
{
   if (s >= m_vals.size())
      error(ETFatal, "Too many source sentences %d/%d", s, m_vals.size());
   
   this->s = s;
}

double FileDFF::value(Uint k)
{
   if (s < m_vals.size())
   {
      if (k >= m_vals[s].size())
         error(ETFatal, "Not that many hypothesis for s: %d, k: %d/%d", s, k, m_vals[s].size());
   }
   else
      error(ETFatal, "Too many source sentences %d/%d", s, m_vals.size());
      
   return m_vals[s][k];
}

//---------------------------------------------------------------------------------------
// FeatureFunctionSet
//---------------------------------------------------------------------------------------

void FeatureFunctionSet::initFFMatrix(const Sentences& src_sents, const int K)
{
   for (Uint m(0); m < M(); ++m) {
      ff_infos[m].function->init(&src_sents, K);
   }
}

void FeatureFunctionSet::computeFFMatrix(uMatrix& H, const Uint s, Nbest &nbest)
{
   const Uint K(nbest.size());

   typedef Nbest::const_iterator NIT;
   isEmptyLine isEmpty = std::for_each(nbest.begin(), nbest.end(), isEmptyLine());
   const Uint empty = isEmpty.numberOfEmpty();

   if (empty == K) {
       // error(ETFatal, "The Nbest list for %d only contains empty lines", s);
       cerr << "The Nbest list for " << s << " only contains empty lines" << endl;
       H.resize(0, M(), false);
       nbest.resize(0);
       return;
   }

   if (!isEmpty.allContiguous())
      error(ETFatal, "The Nbest list for %d contains some non-contiguous empty lines", s);
   
   H.resize(K-empty, M(), false);

   Uint required = FF_NEEDS_NOTHING;
   for (Uint m(0); m < M(); ++m) {
      required |= ff_infos[m].function->requires();    // Check what feature functions need
      ff_infos[m].function->source(s, &nbest);
   }

   Uint l(0);
   for (Uint k = 0; k < K; ++k) {
      if (!isEmpty(nbest[k])) {
         if (required & FF_NEEDS_TGT_TOKENS ) // Target tokenization
            nbest[k].getTokens();
         if ((required & FF_NEEDS_ALIGNMENT) && !nbest[k].alignment) // Alignment
            error(ETFatal, "Alignment needed and not found in nbest[%d][%d]\n", s, k);
         for (Uint m = 0; m < M(); ++m)
            H(l, m) = ff_infos[m].function->value(k);
         ++l;
      }
   }

   Nbest::iterator last = remove_if(nbest.begin(), nbest.end(), isEmpty);
   assert(Uint(nbest.end()-last) == empty);
   assert(Uint(last-nbest.begin()) == K-empty);
   nbest.resize(K-empty);
}

// Read description of a feature set from a file.
Uint FeatureFunctionSet::read(const string& filename, bool verbose, 
                              const char* fileff_prefix,
                              bool isDynamic, bool useNullDeleter)
{
   ff_infos.clear();

   ifstream istr(filename.c_str());
   if (!istr)
      error(ETFatal, "unable to open file %s", filename.c_str());

   string line;
   vector<string> toks;

   // read contents of file
   while (getline(istr, line)) {
      split(line, toks);
      if (toks.size() == 0)
         continue;      // blank lines are ok
      if (toks.size() > 2)
         error(ETFatal, "lines in feature-set file must contain feature name and weight (only)");

      double w(0.0f);
      if (toks.size() == 2) {
         if (!conv(toks[1], w))
            error(ETFatal, "can't convert %s to a weight value", toks[1].c_str());
      }

      string name, arg;
      string::size_type sep = toks[0].find(":");
      if (sep == string::npos)
         name = toks[0];
      else
      {
         name = toks[0].substr(0, sep);
         arg  = toks[0].substr(sep+1);
      }

      if (verbose)
         cerr << "initializing feature " << name << " with arg: [" << arg << "]" << endl;

      ptr_FF  function(create(name, arg, fileff_prefix, isDynamic, useNullDeleter));
      if (!(function))
         error(ETFatal, "unknown feature: %s", name.c_str());

      ff_infos.push_back(ff_info(toks[0], name, w, function));
      toks.clear();
   }

   return ff_infos.size();
}

void FeatureFunctionSet::getWeights(uVector& v) const
{
   if (v.size() != M())
      v.resize(M());
      
   for (Uint m(0); m<M(); ++m)
      v(m) = ff_infos[m].weight;
}

void FeatureFunctionSet::setWeights(const uVector& v)
{
   assert(v.size() == M());
   for (Uint m(0); m<M(); ++m)
      ff_infos[m].weight = v(m);
}

void FeatureFunctionSet::write(const string& filename)
{
   ofstream ostr(filename.c_str());
   if (!ostr)
      error(ETFatal, "unable to open file %s", filename.c_str());
   ostr << setprecision(10);

   for (Uint i(0); i < ff_infos.size(); ++i)
      ostr << ff_infos[i].fullDescription << " " << ff_infos[i].weight << endl;
}

template<typename T>
struct null_deleter
{
   void operator()(T const *) const {}
};

                    
ptr_FF FeatureFunctionSet::create(const string& name, const string& arg, 
                                  const char* fileff_prefix,
                                  bool isDynamic, bool useNullDeleter)
{
   FeatureFunction* ff;

   //cerr << "create(name = " << name << ", arg = " << arg << endl;

   if (name == "FileFF") {
      string fileff_arg = fileff_prefix ? fileff_prefix + arg : arg;
      if (isDynamic)
         ff = new FileDFF(fileff_arg);
      else
         ff = new FileFF(fileff_arg);
   } else if (name == "LengthFF") {
      ff = new LengthFF();
   } else if (name == "NgramFF") {
      ff = new NgramFF(arg);
   } else if (name == "IBM1TgtGivenSrc") {
      ff = new IBM1TgtGivenSrc(arg);
   } else if (name == "IBM1SrcGivenTgt") {
      ff = new IBM1SrcGivenTgt(arg);
   } else if (name == "IBM2TgtGivenSrc") {
      ff = new IBM2TgtGivenSrc(arg);
   } else if (name == "IBM2SrcGivenTgt") {
      ff = new IBM2SrcGivenTgt(arg);
   } else if (name == "IBM1AaronSrcGivenTgt" || name == "IBM1WTransSrcGivenTgt") {
      ff = new IBM1WTransSrcGivenTgt(arg);
   } else if (name == "IBM1AaronTgtGivenSrc" || name == "IBM1WTransTgtGivenSrc") {
      ff = new IBM1WTransTgtGivenSrc(arg);
   } else {
      error(ETFatal, "Invalid feature function: %s:%s", name.c_str(), arg.c_str());
      return ptr_FF(static_cast<FeatureFunction*>(0), null_deleter<FeatureFunction>());
   }

   if ( useNullDeleter ) {
      return ptr_FF(ff, null_deleter<FeatureFunction>());
   } else {
      return ptr_FF(ff);
   }
}


const string& FeatureFunctionSet::help() 
{
   static string help_str = "\
Features available:\n\
\n\
 LengthFF - number of characters\n\
 NgramFF:lm-file[#order] - log prob according to lm-file\n\
 IBM1TgtGivenSrc:ibm1.tgt_given_src - IBM1 forward probability\n\
 IBM1SrcGivenTgt:ibm1.src_given_tgt - IBM1 backward probability\n\
 IBM2TgtGivenSrc:ibm2.tgt_given_src - IBM2 forward probability\n\
 IBM2SrcGivenTgt:ibm2.src_given_tgt - IBM2 backward probability\n\
 IBM1WTransTgtGivenSrc:ibm1.tgt_given_src - IBM1 check if all words translated\n\
 IBM1WTransSrcGivenTgt:ibm1.src_given_tgt - IBM1 check if no words inserted\n\
 FileFF:file[,column] - pre-computed feature\n\
\n\
The <FileFF> feature reads from a file of pre-computed values, optionally\n\
picking out a particular column; the program gen_feature_values can be used to\n\
create files like this.\n\
";
   return help_str;
}

}
