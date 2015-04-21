/**
 * @author Samuel Larkin
 * @file fileReader.h  File reader allows to read fix/dinamic size
 *                     blocks(lines) from a file.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
#ifndef __TRANSLATION_READER_H__
#define __TRANSLATION_READER_H__

#include <portage_defs.h>
#include <basic_data_structure.h>
#include "fileReader.h"
#include <string>
#include <vector>
#include <file_utils.h>
#include <exception>
#include <memory> //auto_ptr

namespace Portage {
   /// All relevant code for file reader which transparently reader fix or
   /// dynamic nbest list.
   /// Prevents global namespace pollution in doxygen.
   namespace FileReader {
      using namespace std;

      // Error string.  Can be reused in unittest.
      const char* const szIncompleteAlignmentRead = "Not all alignments were read.";
      const char* const szIncompleteTranslationRead = "Not all translations were read.";
      const char* const szNotEnoughAlignments = "You don't have enough alignments => file inconsistency!";
      const char* const szMisaligned = "translation & phrase alignment misaligned!";

      /**
       * Most interesting when it comes to read a FIXED size Group which is a sequence of lines in a file.
       * Format is one hypothesis per line.  All lines contiguous.
       */
      class FixTranslationReader : public FixReader<Translation>
      {
         protected:
            mutable iMagicStream  m_alignment; ///< file with the phrase alignment.

         protected:
            bool hasMore() const {
               return !!m_alignment && (m_alignment.peek() != EOF);
            }

         public:
            /// Definition of Parent's type
            typedef FixReader<Translation>   Parent;

            /// See FileReaderBase::FileReaderBase(const string& szFileName, Uint S, Uint K)
            explicit FixTranslationReader(const string& szFileName, const string& szAlignment, Uint K);
            /// Destructor.
            virtual ~FixTranslationReader();

            /**
             * Checks if there is some hypothesis left in the stream.
             * @return Returns true if there is still some hypothesis left to be read.
             */
            virtual bool pollable() const;

            virtual bool poll(Translation& s, Uint* groupId = NULL);
      };

      /**
       * Most interesting when it comes to read a DYNAMIC size Group which is a sequence of lines in a file.
       * Format is "<number>\t<hypothesis>". All lines contiguous.
       */
      class DynamicTranslationReader : public DynamicReader<Translation>
      {
         protected:
            Uint m_nalignmentNo;
            mutable iMagicStream  m_alignment; ///< file with the phrase alignment.

         protected:
            bool hasMore() const {
               return !!m_alignment && (m_alignment.peek() != EOF);
            }

         public:
            /// Definition of Parent's type
            typedef DynamicReader<Translation>   Parent;

            /// See FileReaderBase::FileReaderBase(const string& szFileName, Uint S, Uint K)
            explicit DynamicTranslationReader(const string& szFileName, const string& szAlignment, Uint K=1000);
            /// Destructor.
            virtual ~DynamicTranslationReader();

            /**
             * Checks if there is some hypothesis left in the stream.
             * @return Returns true if there is still some hypothesis left to be read.
             */
            virtual bool pollable() const;

            virtual bool poll(Translation& s, Uint* groupId = NULL);
      };

      /**
       * Creational factory for fix / dynamic reader based on the value of K.
       * @param[in] szFileName  file name
       * @param[in] K           number of hypotheses per source => file contains K x S lines.
       * @return Returns a new fix/dynamic reader based on the value of K.
       */
      NbestReader createT(const string& szFileName, const string& szAlignment, Uint K);
   } // ends FileReader
} // ends Portage

#endif  // __TRANSLATION_READER_H__
