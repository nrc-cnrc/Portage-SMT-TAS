/**
 * @author George Foster
 * @file file_utils.h File utilities.
 * 
 * 
 * COMMENTS: 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <errors.h>
#include "str_utils.h"
#include "portage_defs.h"
#include "MagicStream.h" // We include MagicStream because we want it to be part of the file_utils

namespace Portage {

using std::string;
using namespace boost;

/// An iMagicStream class with a constructor that does a fatal error when the
/// file can't be opened.
class iSafeMagicStream : public iMagicStream
{
public:
   /**
    * Default constructor, opens and checks the stream.
    * @param fname  file name
    * @param bQuiet suppresses the Broken pipe message from gz/pipe fname
    */
   iSafeMagicStream(const string& fname, bool bQuiet = false)
   : iMagicStream(fname, bQuiet) {
      if (this->fail())
	 error(ETFatal, "Unable to open %s for reading", fname.c_str());
   }
};

/// An oMagicStream class with a constructor that does a fatal error when the
/// file can't be opened.
class oSafeMagicStream : public oMagicStream
{
public:
   /**
    * Default constructor, opens and checks the stream.
    * @param fname  file name
    * @param bQuiet suppresses the Broken pipe message from gz/pipe fname
    */
   oSafeMagicStream(const string& fname, bool bQuiet = false)
   : oMagicStream(fname, bQuiet) {
      if (this->fail())
	 error(ETFatal, "Unable to open %s for writing", fname.c_str());
   }
};

/**
 * Gulp a stream into a string. This is fairly inefficient.
 *
 * @param istr  source stream
 * @param dst   string to gulp into
 * @return ref to dst
 */
string& gulpFile(std::istream& istr, string& dst);

/**
 * Open a file and gulp into a string, or die if file can't be opened. This is
 * fairly inefficient 
 * @param filename  name of file
 * @param dst       string to gulp into
 * @return ref to dst
 */
string& gulpFile(const char* filename, string& dst);

/**
 * Read a stream into a vector of lines (appending to what is already there).
 * @param istr   input stream to read from
 * @param lines  returned read lines
 */
void readFileLines(std::istream& istr, vector<string>& lines);

/**
 * Read a stream into a vector of lines (appending to what is already there).
 * @param filename  file name to read from
 * @param lines     returned read lines
 */
inline void readFileLines(const char* filename, vector<string>& lines) {
   iSafeMagicStream istr(filename);
   readFileLines(istr, lines);
}

/**
 * Read a stream into a vector of lines (appending to what is already there).
 * @param filename  file name to read from
 * @param lines     returned read lines
 */
inline void readFileLines(const string& filename, vector<string>& lines) {
   readFileLines(filename.c_str(), lines);
}

/**
 * Write a vector of lines to a stream.
 * @param ostr   output stream to write lines
 * @param lines  lines to write to ostr
 */
void writeFileLines(std::ostream& ostr, const vector<string>& lines);

/**
 * Count the lines in a file (equivalent to wc -l).
 * @param istr  input stream to count lines
 * @return Returns the number of line in istr
 */
Uint countFileLines(std::istream& istr);

/**
 * Count the lines in a file (equivalent to wc -l).
 * @param filename  file name to count lines
 * @return Returns the number of line in istr
 */
inline Uint countFileLines(const char* filename) {
   iSafeMagicStream istr(filename);
   return countFileLines(istr);
}

/**
 * Count the lines in a file (equivalent to wc -l).
 * @param filename  file name to count lines
 * @return Returns the number of line in istr
 */
inline Uint countFileLines(const string& filename) {
   return countFileLines(filename.c_str());
}

/**
 * Determine the size of a file
 * @param filename  name of file
 * @return size in bytes of filename; 0 in case of error
 */
Uint64 fileSize(const string& filename);

/**
 * Test if a file exists, and if so, delete it.  If warning != NULL, it will be
 * issued using error(ETWarn) before the file is deleted.  Issues a fatal error
 * if filename can't be deleted.
 * @param filename     filename to check existence and delete
 * @param warning_msg  warning message before deletion must contain one %s, which will refer to filename.
 */
void delete_if_exists(const char* filename, const char* warning_msg);

/**
 * Test if a file exists, and if so, terminate with error.
 * @param filename   file name to check existence
 * @param error_msg  error message must contain exactly one %s, which will be
 *                   replaced by filename.
 */
void error_if_exists(const char* filename, const char* error_msg);

/**
 * Verifies if a file exists (or optionally its .gz version).
 * @param filename           file to check
 * @param accept_compressed  if true, consider the .gz version as equivalent
 * @return Returns true if the file (or the its .gz version) exists.
 */
bool check_if_exists(const string& filename, bool accept_compressed = true);

/**
 * Like basename and dirname, will decompose the original filename into its
 * path and file
 * @param[in]  filename  original file name to decompose.
 * @param[out] path      returned path if path not NULL
 * @param[out] file      returned file if file not NULL
 */
void DecomposePath(const string& filename, string* path, string* file);

/**
 * Gets the directory's name out of the filepath.
 * @param[in] filename  file path
 * @return  the path of filename
 */
string DirName(const string& filename);

/**
 * Gets the filename's name out of the filepath.
 * @param[in] filename  file path
 * @return  the name of file
 */
string BaseName(const string& filename);

/**
 * Verifies that the file has .Z, .z or .gz extension.
 * @param filename filename to check if it's a gzip file
 * @return Returns true if filename contains .Z, .z or .gz
 */
bool isZipFile(const string& filename);

/**
 * Removes any extension of the form "\.[^\.]*".
 * @param filename file name to remove extension.
 * @return Returns filename without the extension.
 */
string removeExtension(const string& filename);

/**
 * Removes the .Z, .z or .gz extension from filename, if any
 * @param filename file name to remove gzip extension
 * @return Returns filename without the gzip extension
 */
string removeZipExtension(const string& filename);

/**
 * Adds an extension to a file name and takes care of gzip files.
 * filename + toBeAdded
 * filename%(.gz|.z|.Z) + toBeAdded + .gz
 * @param filename  file name to extended
 * @param toBeAdded extension to add (should include the . if one is desired)
 * @return Returns the extended file name
 */
string addExtension(const string& filename, const string& toBeAdded);

/**
 * Extracts the file name from a path
 * @param path  path from which to extract the filename
 * @return Return the file name
 */
string extractFilename(const string& path);

/**
 * Parse s, typically a file name, and swap its two languages.
 *
 * Works by looking for pattern
 * (^|[^[:alnum:]])[[:alnum:]]+separator[[:alnum:]]+($|[^[:alnum:]])
 *
 * Also produces a full parse of s that can be used to generate other derived
 * patterns than just swapping the two languages.  Note that all out parameters
 * are optional, pass NULL if you don't care about a component.
 *
 * @param s name in which to reverse the languages
 * @param separator the string between l1 and l2, e.g., "_given_" or "2"
 * @param[out] l1 the alpha-numeric sequence found immediately before separator
 * @param[out] l2 the alpha-numeric sequence found immediately after separator
 * @param[out] prefix the part of s before l1
 * @param[out] suffix the part of s after l2
 *
 * @return prefix+l2+separator+l1+suffix, or an empty string if filename can't
 *         be parsed correctly, in which case none of the out parameters will
 *         have been modified.
 */
string swap_languages(const string& s, const string& separator,
      string* l1 = NULL, string* l2 = NULL,
      string* prefix = NULL, string* suffix = NULL);

/**
 * This declaration creates ambiguity with the standard tmpnam function, so
 * that your code won't compile if you try to use tmpnam.  Intentionally, no
 * implementation is provided either.
 * As the g++ warns, tmpnam() is dangerous and should never be used.  Use
 * mkstemp instead, there are several examples in Portage of how to do so.
 */
char *tmpnam(char *);

/// Read the contents of a tokenized file, line by line.
class TokReader 
{
   iSafeMagicStream* istr;
   string fname;
   string line;
   Uint lineno;

public:
   
   /// Default constructor.
   TokReader() : istr(NULL), lineno(0) {}
   // TODO: add from-stream constructor.
   
   /// Default destructor.
   ~TokReader() {if (istr) delete istr;}

   /**
    * Open a new file, after closing any existing one. Die if file is
    * impossible to open.
    * @param filename file name to open.
    */
   void open(const char* filename) {
      if (istr) delete istr;
      istr = new iSafeMagicStream(filename);
      fname = filename;
      lineno = 0;
   }

   vector<string> seg;   ///< Next segment read from the file.

   /**
    * Set the member seg to the next segment read from the file.
    * @return false if at end of file.
    */
   bool nextSeg() {
      if (!getline(*istr, line)) return false;
      seg.clear();
      split(line, seg, " \t\n\r");
      ++lineno;
      return true;
   }

   /**
    * Returns true if last-read segment contains markup.
    * @return Returns true if last-read segment contains markup.
    */
   bool hasMarkup() {
      Uint i = 0;
      for (; i < seg.size(); ++i)
	 if (seg[i].length() > 1 && seg[i][0] == '<')
	    break;
      for (; i < seg.size(); ++i)
	 if (seg[i].length() > 1 && last(seg[i]) == '>')
	    break;
      return i < seg.size();
   }

   /**
    * Returns 1-based index of last line read from file.
    * @return Returns 1-based index of last line read from file.
    */
   Uint lineNumber() {return lineno;}

   /**
    * Returns name of current file.
    * @return Returns name of current file.
    */
   string filename() {return fname;}

};

/// Read a pair of aligned, tokenized files line by line (synchronously).
struct TokReaderPair 
{
   TokReader tr1;
   TokReader tr2;

   /**
    * Open a new file, after closing any existing one. Die if file is
    * impossible to open.
    * @param file1  first file to open
    * @param file2  second file to open
    */
   void open(const char* file1, const char* file2) {
      tr1.open(file1);
      tr2.open(file2);
   }

   /**
    * Read next segment pair into tr1.seg and tr2.seg. Warns if either file
    * ends prematurely.
    * @return false if at end of file.
    */
   bool nextSeg() {
      bool s1 = tr1.nextSeg();
      bool s2 = tr2.nextSeg();
      if (s1 && !s2)
	 error(ETWarn, "file %s is too short", tr2.filename().c_str());
      else if (s2 && !s1)
	 error(ETWarn, "file %s is too short", tr1.filename().c_str());
      else 
	 return s1 && s2;
   }

   /**
    * Returns true if last-read segment pair contains markup.
    * @return Returns true if last-read segment pair contains markup
    */
   bool hasMarkup() {return tr1.hasMarkup() || tr2.hasMarkup();}
};


} //ends Portage namespace
#endif
