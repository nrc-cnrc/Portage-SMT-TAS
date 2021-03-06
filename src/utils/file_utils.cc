/**
 * @author George Foster
 * @file file_utils.cc File utilities.
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

#include <cerrno>
#include <climits>
#include <sys/stat.h>
#include <cstdlib>

#include "portage_defs.h"
#include "file_utils.h"
#include "errors.h"
#ifdef Darwin
#include <mach-o/dyld.h>
#endif

using namespace std;
using namespace Portage;

string& Portage::gulpFile(istream& istr, string& dst) {
   ostringstream ss;
   ss << istr.rdbuf();
   return dst = ss.str();
}

string& Portage::gulpFile(const char* filename, string& dst) {
   iSafeMagicStream file(filename);
   return gulpFile(file, dst);
}

void Portage::gulpFile(const string& filename) {
   const streamsize length = 8192;
   char buffer[length];
   iSafeMagicStream in(filename);
   while (in) {
      in.read(buffer, length);
   }
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

bool Portage::matchMagicNumber(const string& filename, const string& magic_number)
{
   iMagicStream in(filename, true);
   char buffer[magic_number.size()+1];
   in.read(buffer, magic_number.size());
   return in.good() &&
          0 == strncmp(buffer, magic_number.c_str(), magic_number.size());
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

Uint64 Portage::fileSize(const string& filename)
{
   ifstream f(filename.c_str());
   if ( !f ) return 0;
   Uint64 start_pos = f.tellg();
   f.seekg(0, ios_base::end);
   if ( !f ) return 0;
   Uint64 end_pos = f.tellg();
   assert(end_pos >= start_pos);
   return end_pos - start_pos;
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

void Portage::error_unless_exists(const string& filename, bool accept_compressed,
      const char* file_type)
{
   if (!check_if_exists(filename, accept_compressed))
      error(ETFatal, "Error: can't open%s%s file %s: %s",
         file_type ? " " : "",
         file_type ? file_type : "",
         filename.c_str(),
         errno ? strerror(errno) : "unknown problem");
}

bool Portage::check_if_exists(const string& filename, bool accept_compressed)
{
   if (filename.empty()) return false;
   if (filename == "-") return true;
   if (*filename.begin()  == '|') return true;
   if (*filename.rbegin() == '|') return true;

   ifstream ifs(filename.c_str());
   if (!ifs) {
      if ( accept_compressed ) {
         const string fz = filename + ".gz";
         ifstream ifsz(fz.c_str());
         return !!ifsz;
      } else {
         //int errnum = errno;
         //cerr << "errno = " << errnum << " strerr = " << strerror(errnum) << endl;
         return false;
      }
   }
   return true;
}

bool Portage::is_directory(const string& dirname)
{
   struct stat stat_buf;
   if (stat(dirname.c_str(), &stat_buf) == 0 && S_ISDIR(stat_buf.st_mode))
      return true;
   return false;
}

void Portage::DecomposePath(const string& filename, string* path, string* file)
{
   string rpath, rfile;
   
   if (filename.empty() ) {
      // Follow what dirname(3) and basename(3) do for an empty argument
      // See "man 3 basename" for details.
      rpath = rfile = ".";
   }
   else if (filename == "/") {
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

string Portage::GetAppPath() {
   char program_path[PATH_MAX];
#ifdef Darwin
   program_path[0] = '\0';
   char path[PATH_MAX+1];
   uint32_t program_path_length = sizeof(path);
   if (_NSGetExecutablePath(path, &program_path_length) != 0 ||
       !realpath(path, program_path)) {
      error(ETWarn, "Can't get real path for '%s': %s", path, strerror(errno));
      return "";
   }
   program_path_length = strlen(program_path);
#else
   ssize_t program_path_length = readlink("/proc/self/exe", program_path, PATH_MAX);
   if ( program_path_length < 0 ) {
      int errnum = errno;
      error(ETWarn, "Can't read /proc/self/exe link: %s", strerror(errnum));
      return "";
   }
#endif
   assert( program_path_length < PATH_MAX );
   program_path[program_path_length] = '\0';
   return program_path;
}

string Portage::adjustRelativePath(const string& basedir, const string& filename)
{
   if (filename.empty() or filename[0] == '/' or basedir.empty())
      return filename;
   return basedir + '/' + filename;
}

bool Portage::isZipFile(const string& filename)
{
   const size_t dot = filename.rfind(".");
   return dot != string::npos
             && (filename.substr(dot) == ".gz"
                 || filename.substr(dot) == ".z"
                 || filename.substr(dot) == ".Z");
}

string Portage::removeExtension(const string& filename) {
   return filename.substr(0, filename.rfind("."));
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

string Portage::path2file(const string& path)
{
   string ret;
   if (path[0] == '.') ret = "r";
   else if (path[0] == '/') ret = "a";
   for (Uint i = 0; i < path.size(); ++i)
      if (path[i] == '/') ret += "_";
      else ret += path[i];
   return ret;
}

/**
 * Create a directory.
 * Create a directory if it doesn't exist or validate that a directory exists
 * if there is an entry for that name.
 * @param:  dir   directory's name.
 * @param:  permissions  permissions for dir.
 * @return Return true if the directory is present/created.
 */
static bool mkDirHelper(const char *directory, mode_t permissions) {
   struct stat st;

   if (stat(directory, &st) != 0) {
      // Looks like the directory does not exist.  Let's create it.
      if (mkdir(directory, permissions) != 0) {
         int errnum = errno;
         error(ETWarn, "Can't create directory %s: %s", directory, strerror(errnum));
         return false;
      }
   }
   // There is already something with the name directory, let's make sure it is a directory.
   else if (!S_ISDIR(st.st_mode)) {
      error(ETWarn, "%s already exists and is not a directory", directory);
      return false;
   }

   return true;
}

bool Portage::mkDirectories(const char* const directory, const mode_t permissions)
{
    char* subDirectory = strdup(directory);
    assert(subDirectory);
    char* previousSlash = subDirectory;
    char* nextSlash;
    bool  status = true;

    while (status && (nextSlash = strchr(previousSlash, '/')) != NULL) {
       // skip over empty path like top/////lower.
       if (nextSlash != previousSlash) {
          *nextSlash = '\0';  // Creates a subPath.
          status = mkDirHelper(subDirectory, permissions);
          *nextSlash = '/';   // Restores the full Path.
       }
       previousSlash = nextSlash + 1;
    }
    if (status) {
        status = mkDirHelper(directory, permissions);
    }
    free(subDirectory);
    return status;
}

string Portage::swap_languages(
      const string& s, const string& separator,
      string* out_l1, string* out_l2, string* out_prefix, string* out_suffix
) {
   string prefix, l1, l2, suffix;
   const size_t len = s.length();
   const size_t sep_len = separator.length();
   const size_t sep_pos = s.rfind(separator);
   const size_t sep_end = sep_pos + sep_len;
   if ( sep_pos == string::npos ||      // separator not found
        sep_end + 1 > len ||            // no room for l2
        !isalnum(s[sep_end]) ||         // l2 doesn't start with an alnum
        sep_pos < 1 ||                  // no room for l1
        !isalnum(s[sep_pos-1]) )        // l1 doesn't end with an alnum
      return string();

   Uint l2_len = 1;
   while ( true ) {
      assert( sep_end + l2_len <= len);
      if ( sep_end + l2_len == len ) {
         l2 = s.substr(sep_end);
         suffix = "";
         break;
      } else if ( !isalnum(s[sep_end + l2_len]) ) {
         l2 = s.substr(sep_end, l2_len);
         suffix = s.substr(sep_end + l2_len);
         break;
      }
      ++l2_len;
   }
   assert(l2.length() == l2_len);

   Uint l1_len = 1;
   while ( true ) {
      assert(sep_pos >= l1_len);
      if ( sep_pos == l1_len ) {
         l1 = s.substr(0, l1_len);
         prefix = "";
         break;
      } else if ( !isalnum(s[sep_pos - l1_len - 1]) ) {
         l1 = s.substr(sep_pos - l1_len, l1_len);
         prefix = s.substr(0, sep_pos - l1_len);
         break;
      }
      ++l1_len;
   }
   assert(l1.length() == l1_len);

   if ( out_suffix ) *out_prefix = prefix;
   if ( out_l1 )     *out_l1     = l1;
   if ( out_l2 )     *out_l2     = l2;
   if ( out_suffix ) *out_suffix = suffix;
   return prefix + l2 + separator + l1 + suffix;
}

