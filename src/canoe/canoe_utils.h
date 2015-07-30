#ifndef __CANOE_UTILS____H
#define __CANOE_UTILS____H

#include "file_utils.h"
#include <string>

using namespace std;
using namespace Portage;

#ifdef NNJM_LOADCLASSES_SPLITZ
#pragma message "Using loadClasses splitZ"
   // splitZ
   template <class Tags>
   void loadClasses(Tags& tags, const string& infile) {
      cerr << "Loading classes " << infile << endl;
      iSafeMagicStream is(infile);
      string line;
      vector<string> sc;
      sc.reserve(4);
      while (getline(is, line)) {
         if (splitZ<string>(line, sc) != 2)
            error(ETFatal, "expected 'word\ttag' entries in <%s>", infile.c_str());
         tags[sc[0]] = sc[1];
      }
   }
#define NNJM_HAS_LOADCLASSES 1
#endif


#ifdef NNJM_LOADCLASSES_STRTOK
#pragma message "Using loadClasses strtok"
   // strtok
   template <class Tags>
   void loadClasses(Tags& tags, const string& infile) {
      cerr << "Loading classes " << infile << endl;
      iSafeMagicStream is(infile);
      string line;
      const char* sep = "\t";
      while (getline(is, line)) {
         const Uint len = strlen(line.c_str());
         char work[len+1];
         strcpy(work, line.c_str());
         assert(work[len] == '\0');

         char* strtok_state;
         const char* word = strtok_r(work, sep, &strtok_state);
         if (word == NULL)
            error(ETFatal, "expected 'word\ttag' entries in <%s>", infile.c_str());
         const char* tag = strtok_r(NULL, sep, &strtok_state);
         if (tag == NULL)
            error(ETFatal, "expected 'word\ttag' entries in <%s>", infile.c_str());

         tags[word] = tag;
      }
   }
#define NNJM_HAS_LOADCLASSES 1
#endif


#ifdef NNJM_LOADCLASSES_DOUBLEREAD
#pragma message "Using loadClasses with double read"
   // doubleRead
   template <class Tags>
   void loadClasses(Tags& tags, const string& infile) {
      cerr << "Loading classes " << infile << endl;
      iSafeMagicStream is(infile);
      char word[1024000];
      char tag[1024];
      while (is >> word >> tag) {
         tags[word] = tag;
         word[0] = tag[0] = '\0';
      }
   }
#define NNJM_HAS_LOADCLASSES 1
#endif


#ifdef NNJM_LOADCLASSES_BINARY
#include "binio.h"
#include "binio_maps.h"
#pragma message "Using loadClasses binary"
   template <class Tags>
   void loadClasses(Tags& tags, const string& infile) {
      cerr << "Loading classes " << infile << endl;
      iSafeMagicStream is(infile);
      Portage::BinIO::readbin(is, tags);
   }
#define NNJM_HAS_LOADCLASSES 1
#endif


#ifdef NNJM_TOKENINDEX
#pragma message "Using loadClasses Tightly Packed"
   template <class Tags>
   void loadClasses(Tags& tags, const string& infile) {
      cerr << "Loading classes " << infile + EXT << endl;
      tags.open(infile + EXT);
   }
#define NNJM_HAS_LOADCLASSES 1
#endif


#ifndef NNJM_HAS_LOADCLASSES
#pragma message "Using original loadClasses"
   // This is the original code.
   template <class Tags>
   void loadClasses(Tags& tags, const string& infile) {
      cerr << "Loading classes " << infile << endl;
      iSafeMagicStream is(infile);
      string line;
      while (getline(is, line)) {
         vector<string> sc = split<string>(line); // word\tclass
         if (sc.size() != 2)
            error(ETFatal, "expected 'word\ttag' entries in <%s>", infile.c_str());
         tags[sc[0]] = sc[1];
      }
   }
#define NNJM_HAS_LOADCLASSES 1
#endif

#endif // __CANOE_UTILS____H
