/**
 * @author Aaron Tikuisis / George Foster / Samuel Larkin / Eric Joanis
 * @file file_ff.h  Read-from-file feature function and its dynamic version
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005 - 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005 - 2008, Her Majesty in Right of Canada
 */

#ifndef FILE_FF_H
#define FILE_FF_H

#include "featurefunction.h"
#include "multiColumnFileFF.h"

namespace Portage {

//----------------------------------------------------------------------------
/**
 * Read-from-file feature function.
 */
class FileFF : public FeatureFunction
{
protected:
   string           m_filename;  ///< file name
   Uint             m_column;    ///< column index for this feature function.
   multiColumnUnit  m_info;  // NOT TO BE DELETED

protected:
   virtual bool loadModelsImpl();
public:

   static const char separator;   ///< Column separator token: ,

   /**
    * Construct.
    * @param filespec spec of form filename[,i] - if "<i>" specified, use the
    * ith column in file "<filename>", otherwise there must only be one value
    * per line
    */
   FileFF(const string& filespec);

   virtual Uint requires() { return FF_NEEDS_NOTHING; }
   virtual bool parseAndCheckArgs();

   virtual double value(Uint k) {
      return m_info->get(m_column, s*K + k);
   }

   virtual bool done() {
      if (!m_info->eof()) {
         cerr << "Not done: " << m_filename << ":" << m_column << endl;
         return false;
      }
      return true;
   }
};


//----------------------------------------------------------------------------
/**
 * Read-from-dynamic-file feature feature.
 */
class FileDFF : public FeatureFunction
{
   /// Internal definition of a matrix for this feature function.
   typedef vector< vector<double> > matrice;
   string      m_filename;  ///< file name
   Uint        m_column;    ///< column index for this feature function.
   /**
    * m_vals is not an even matrix since we are in dynamic mode this means
    * that we don't have nbest of the same size.
    */
   matrice     m_vals;

protected:
   virtual bool loadModelsImpl();
public:

   static const char separator;   ///< Column separator token: ,

   /**
    * Construct.
    * @param filespec spec of form filename[,i] - if "<i>" specified, use the
    * ith column in file "<filename>", otherwise there must only be one value
    * per line
    */
   FileDFF(const string& filespec);

   virtual Uint requires() { return FF_NEEDS_NOTHING; }
   virtual bool parseAndCheckArgs();
   virtual void source(Uint s, const Nbest * const nbest);
   virtual double value(Uint k);
};

} // Portage

#endif // FILE_FF_H
