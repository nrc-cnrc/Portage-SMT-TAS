/**
 * @author Samuel Larkin
 * @file fileReader.h  File reader allows to read fix/dinamic size
 *                     blocks(lines) from a file.
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
#ifndef __FILE_READER_H__
#define __FILE_READER_H__

#include <portage_defs.h>
#include <basic_data_structure.h>
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

      /// Base class for fix / dynamic file reading for nbest files.
      template<class T>
      class FileReaderBase : private NonCopyable
      {
         public:
            /// Definition of a group AKA nbest.
            typedef vector<T>      Group;
            /// Definition of a list of group AKA list of nbests.
            typedef vector<Group>  Everything;

         protected:
            mutable iMagicStream   m_file;     ///< file from which to read.
            const Uint             m_K;        ///< number of hypotheses per source.
            Uint                   m_nGroupNo; ///< keep track of hypothesis group number.

         protected:
            /**
             * Constructor.
             * @param[in] szFileName  file name
             * @param[in] K           number of hypotheses per source or 0 for
             *                        dynamic size.
             */
            explicit FileReaderBase(const string& szFileName, Uint K);

         public:
            /// Destructor.
            virtual ~FileReaderBase();

         public:
            /**
             * Checks if there is some hypothesis left in the stream.
             * @return Returns true if there is still some hypothesis left to be read.
             */
            virtual bool pollable() const { return !!m_file && (m_file.peek() != EOF);}

            /**
             * Reads one hypothesis and returns its index
             * @param[out] s        hypothesis
             * @param[out] groupId  hypothesis group's index
             * @return  Returns true if there is some hypothesis still available in the stream.
             */
            virtual bool poll(T& s, Uint* groupId = NULL) = 0;
            /**
             * Reads a group of sentence AKA a nBest list.
             * @param[out] g      returned read group aka nbest list.
             * @param[out] index  hypothesis group's index
             * @return  Returns true if there is some hypothesis still available in the stream.
             */
            virtual bool poll(Group& g, Uint* groupId = NULL) = 0;

         private:
            /// Deactivated default constructor.
            FileReaderBase();
      };

      /**
       * Most interesting when it comes to read a FIXED size Group which is a sequence of lines in a file.
       * Format is one hypothesis per line.  All lines contiguous.
       */
      template<class T>
      class FixReader : public FileReaderBase<T>
      {
            Uint                   m_nSentNo;  ///< keep track of hypothesis number within group.

         public:
            /// Definition of Parent's type
            typedef FileReaderBase<T>   Parent;
            /// Inherited definition of a Group
            typedef typename Parent::Group       Group;
            /// Inherited definition of Everything
            typedef typename Parent::Everything  Everything;

            /// See FileReaderBase<T>::FileReaderBase(const string& szFileName, Uint S, Uint K)
            explicit FixReader(const string& szFileName, Uint K);
            /// Destructor.
            virtual ~FixReader();

            virtual bool poll(T& s, Uint* groupId = NULL);
            virtual bool poll(Group& g, Uint* groupId = NULL);
      };

      /**
       * Most interesting when it comes to read a DYNAMIC size Group which is a sequence of lines in a file.
       * Format is "<number>\t<hypothesis>". All lines contiguous.
       */
      template<class T>
      class DynamicReader : public FileReaderBase<T>
      {
         public:
            /// Definition of Parent's type
            typedef FileReaderBase<T>   Parent;
            /// Inherited definition of a Group
            typedef typename Parent::Group       Group;
            /// Inherited definition of Everything
            typedef typename Parent::Everything  Everything;

            /// See FileReaderBase<T>::FileReaderBase(const string& szFileName, Uint S, Uint K)
            explicit DynamicReader(const string& szFileName, Uint K=1000);
            /// Destructor.
            virtual ~DynamicReader();

            virtual bool poll(T& s, Uint* groupId = NULL);
            virtual bool poll(Group& g, Uint* groupId = NULL);
      };

      /**
       * Creational factory for fix / dynamic reader based on the value of K.
       * @param[in] szFileName  file name
       * @param[in] K           number of hypotheses per source => file contains K x S lines.
       * @return Returns a new fix/dynamic reader based on the value of K.
       */
      template<class T>
      std::auto_ptr<FileReaderBase<T> > create(const string& szFileName, Uint K);
   } // ends FileReader

   /// Since these classes were intended to facilitate nbest reading, here is the definition for a nBest reader.
   typedef std::auto_ptr<FileReader::FileReaderBase<Translation> > NbestReader;
} // ends Portage

#include <fileReader.cc>

#endif  // __FILE_READER_H__
