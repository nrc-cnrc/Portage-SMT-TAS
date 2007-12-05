/**
 * @author George Foster
 * @file file_utils.cc File utilities.
 * 
 * 
 * COMMENTS: 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <cerrno>
#include "portage_defs.h"
#include "file_utils.h"
#include "errors.h"
#include <ext/stdio_filebuf.h>

using namespace std;
using namespace Portage;

string& Portage::gulpFile(istream& istr, string& dst) {
   ostringstream ss;
   ss << istr.rdbuf();
   return dst = ss.str();
}

string& Portage::gulpFile(const char* filename, string& dst) {
   IMagicStream file(filename);
   return gulpFile(file, dst);
}

void Portage::readFileLines(std::istream& istr, vector<string>& lines)
{
   string null;
   lines.push_back(null);
   while (getline(istr, lines.back()))
      lines.push_back(null);
   lines.pop_back();
}

void Portage::writeFileLines(std::ostream& ostr, const vector<string>& lines)
{
   for (vector<string>::const_iterator p = lines.begin(); p != lines.end(); ++p)
      ostr << *p << endl;
}

Uint Portage::countFileLines(std::istream& istr)
{
   string line;
   Uint counter(0);
   while (getline(istr, line)) {
      ++counter;
   }
   return counter;
}

void Portage::delete_if_exists(const char* filename, const char* warning_msg) {
   ifstream file(filename);
   if ( file ) {
      error(ETWarn, warning_msg, filename);
      file.close();
      if ( 0 != remove(filename) ) {
         int errnum = errno;
         error(ETFatal, "Can't delete file %s: %s",
               filename, strerror(errnum));
      }
   }
}

void Portage::error_if_exists(const char* filename, const char* error_msg) {
   ifstream file(filename);
   if ( file ) error(ETFatal, error_msg, filename);
}

bool Portage::check_if_exists(const string& filename)
{
   if (filename.empty()) return false;
   if (filename == "-") return true;
   if (*filename.begin()  == '|') return true;
   if (*filename.rbegin() == '|') return true;

   ifstream ifs(filename.c_str());
   if (!ifs) {
      const string fz = filename + ".gz";
      ifstream ifsz(fz.c_str());
      return !!ifsz;
   }
   return true;
}

void Portage::DecomposePath(const string& filename, string* path, string* file)
{
   assert(filename.size() > 0);
   
   string rpath, rfile;
   
   // filename = "/"
   if (filename == "/") {
      rpath = rfile = "/";
   }
   else {
      string f = filename;
      if (f[f.size()-1] == '/')
         f.erase(f.size()-1);
         
      size_t p = f.rfind("/");
      if (p == string::npos) {
         rpath = ".";
         rfile = f;
      }
      else {
         rfile = f.substr(p+1, string::npos);
         rpath = f.substr(0, p);
      }
   }
   
   if (path != NULL) *path = rpath.size() > 0 ? rpath : "/";
   if (file != NULL) *file = rfile;
}

string Portage::DirName(const string& filename)
{
   string r;
   DecomposePath(filename, &r, NULL);
   return r;
}

string Portage::BaseName(const string& filename)
{
   string r;
   DecomposePath(filename, NULL, &r);
   return r;
}

bool Portage::isZipFile(const string& filename)
{
   const size_t dot = filename.rfind(".");
   return dot != string::npos
             && (filename.substr(dot) == ".gz"
                 || filename.substr(dot) == ".z"
                 || filename.substr(dot) == ".Z");
}

string Portage::removeZipExtension(const string& filename)
{
   const size_t dot = filename.rfind(".");
   if (dot != string::npos
       && (filename.substr(dot) == ".gz"
           || filename.substr(dot) == ".z"
           || filename.substr(dot) == ".Z"))
   {
      return filename.substr(0, dot);
   }
   else
   {
      return filename;
   }
}

string Portage::addExtension(const string& filename, const string& toBeAdded)
{
   if (isZipFile(filename))
      return removeZipExtension(filename) + toBeAdded + ".gz";
   else
      return filename + toBeAdded;
}

string Portage::extractFilename(const string& path)
{
   size_t pos = path.rfind("/");
   if (pos == string::npos)
      return path;
   else
      return path.substr(pos+1);
}

