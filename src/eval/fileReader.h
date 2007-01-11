/**
 * @author Samuel Larkin 
 * @file fileReader.h  File reader allows to read fix/dinamic size blocks(lines) from a file.
 *
 * $Id$
 *
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
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
#include <boost/noncopyable.hpp>

namespace Portage {
   /// All relevant code for file reader which transparently reader fix or dynamic nbest list.
   /// Prevents global namespace pollution in doxygen.
   namespace FileReader {
      using namespace std;

      /// Exception to be thrown if we can't open the file.
      class InvalidFileName : public std::exception
      {
         private:
            const string   m_fn;   ///< error message
         public:
            /// Constructor
            /// @param fn  file name or error message
            explicit InvalidFileName(const string& fn) : m_fn(fn) {}
            /// Destructor.
            ~InvalidFileName() throw() {}
            /// Get the error message.
            /// @return Returns the error message.
            virtual const char* what() const throw () { return m_fn.c_str(); }
      };

      /// Base class for fix / dynamic file reading for nbest files.
      template<class T>
      class FileReaderBase : private noncopyable
      {
         public:
            /// Definition of a group AKA nbest.
            typedef vector<T>      Group;
            /// Definition of a list of group AKA list of nbests.
            typedef vector<Group>  Everything;

         protected:
            mutable iMagicStream   m_file;     ///< file from which to read.
            const Uint             m_S;        ///< number of sources.
            const Uint             m_K;        ///< number of hypotheses per source.
            Uint                   m_nSentNo;  ///< keep track of hypothesis number.


         protected:
            /**
             * Constructor.
             * @param[in] szFileName  file name
             * @param[in] S           number of source
             * @param[in] K           number of hypotheses per source or 0 for dynamic size.
             * @exception InvalidFileName  exception is thrown if the file can't be opened
             */
            explicit FileReaderBase(const string& szFileName, const Uint& S, const Uint& K) throw(InvalidFileName);

         public:
            /// Destructor.
            virtual ~FileReaderBase();

         public:
            /**
             * Checks if there is some hypothesis left in the stream.
             * @return Returns true if there is still some hypothesis left to be read.
             */
            bool pollable() const { return !!m_file && (m_file.peek() != EOF);}
            
            /**
             * Reads one hypothesis and returns its index
             * @param[out] index  hypothesis's index
             * @param[out] s      hypothesis
             * @return  Returns true if there is some hypothesis still available in the stream.
             */
            virtual bool poll(Uint& index, string& s) = 0;
            /**
             * Reads a group of sentence AKA a nBest list.
             * @param[out] g  returned read group aka nbest list.
             * @return  Returns true if there is some hypothesis still available in the stream.
             */
            virtual bool poll(Group& g) = 0;

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
         public:
            /// Definition of Parent's type
            typedef FileReaderBase<T>   Parent;
            /// Inherited definition of a Group
            typedef typename Parent::Group       Group;
            /// Inherited definition of Everything
            typedef typename Parent::Everything  Everything;
            
            /// See FileReaderBase<T>::FileReaderBase(const string& szFileName, const Uint& S, const Uint& K)
            explicit FixReader(const string& szFileName, const Uint& S, const Uint& K);
            /// Destructor.
            virtual ~FixReader();

            virtual bool poll(Uint& index, string& s);
            virtual bool poll(Group& g);
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
            
            /// See FileReaderBase<T>::FileReaderBase(const string& szFileName, const Uint& S, const Uint& K)
            explicit DynamicReader(const string& szFileName, const Uint& S, const Uint& K);
            /// Destructor.
            virtual ~DynamicReader();

            virtual bool poll(Uint& index, string& s);
            virtual bool poll(Group& g);
      };

      /**
       * Creational factory for fix / dynamic reader based on the value of K.
       * @param[in] szFileName  file name
       * @param[in] S           expected number of source in file
       * @param[in] K           number of hypotheses per source => file contains K x S lines.
       * @return Returns a new fix/dynamic reader based on the value of K.
       */
      template<class T>
      std::auto_ptr<FileReaderBase<T> > create(const string& szFileName, const Uint& S, const Uint& K);
   } // ends FileReader

   /// Since these classes were intended to facilitate nbest reading, here is the definition for a nBest reader.
   typedef std::auto_ptr<FileReader::FileReaderBase<Translation> > NbestReader;
} // ends Portage

#include <fileReader.cc> 

#endif  // __FILE_READER_H__
