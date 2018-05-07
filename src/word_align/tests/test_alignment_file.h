/**
 * @author Eric Joanis
 * @file test_alignment_file.h  Test suite for AlignmentFile
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "alignment_file.h"
#include "tp_alignment.h"
#include <stdlib.h>
#include "file_utils.h"

using namespace Portage;

namespace Portage {

class TestAlignmentFile : public CxxTest::TestSuite 
{
   string tmpfile;

public:
   void setUp() {
      char tmpfilename[] = "/tmp/testAlignmentFile.XXXXXX";
      int fd = mkstemp(tmpfilename);
      FOR_ASSERT(fd);
      tmpfile = tmpfilename;
      assert(fd != -1);

      oSafeMagicStream out(tmpfile);
      out << "\
0,1 - 3\n\
- 1,2,3 -\n\
- - - - - -\n\
0,1 8 2 4,5,17 7 6,10 6,9 2,3 11 12 13 14 17 15,16 18\n\
0,1 2 1 2 3 4,5 6 7,10 9,11 13 8,13 12,14,15 13,16,17,18,19 24 21 20,21,23 22 24 25\n\
0,1,2,3 2 4 5 6,7 3 10 13 12,14,15 11 10 9,16,17,19 18,20,21 22 23 26 26 25,28,30 8,24,29,30 26,27 31\n\
4,5 1 30 4 8,30 6 14 0,14 13,18 10 11 9,12 21 30 16,17,19 21 30 20,25,29,40 21,27 30 7,28 14 28 32 41 34 34 2,3,22,23,24,34 31,34 15,26,33,38 35,44 36 37,48 41,42 39 43 51 53 47 45,46,49,64 54 51 52 55,56 50,55 58 57,59 60 68 64 61,64,71 62,63 65 64 66,67 90 64,72 70 74,76 75 73,77 82 78,79 89,90 82,85 69,81,82 83 74 80,82,97 84,93 74 86 87 88,91 92 94,95 96 96 98\n\
0 1 2 3 4 16 6 7 8 9 19 11 12 13 14 15 5,16 17 18 10,19 20 21 21 23 24 22,25 26 27 28 29 30 31 32 33 35 34,35 37 36,37 38 39 40 41 42 43 44 45 46 47 48 49 54 54 52 53 50,51,54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74,80 75 76 77 78 79 74 81 82 83 103 85 86 87 88,93 89 90 91 92 88 94 101 102 97 107 99 100 101 96,102 84,95,103 104 105 106 98,125 108 116 110 111,118 112 113 114,120 115 109,116 117 111 119 114 121 122 123 124 131 126 127 128 129 147 146 132 142 132 135 136 137 138 139 140 141 133,154 143 144 158 146 130,147 148 149 134,150,151 150 152 153,157 169 155 171 153 145,158 159 160 161 162 163 164 165 166 167 168,170 182 168 156,171 172 173 174,184 175,177 176 177 178 179 180 181 182 183 174 185 186 187 188 189 190 191,197 192 193,194 193 195 196 197 198 199 200 201 202 203 204 205 206 207 208 209 210 211,215,221 212 213,217 214 211 216 213 218 219 219 211 220,222 223 224 225 226,232 227 228 229 230 231 226 233\n\
";
      out.close();
      if ( system(("./tp_alignment_build " + tmpfile + " " + tmpfile + ".tpa").c_str()) != 0 ) {
         error( ETWarn, "tp_alignment_build command failed" );
      }
   }
   void tearDown() {
      unlink(tmpfile.c_str());
      unlink((tmpfile + ".tpa").c_str());
   }
   void testTPvsNotTP() {
      AlignmentFile* textFile = AlignmentFile::create(tmpfile);
      TS_ASSERT(textFile);
      TS_ASSERT_EQUALS(textFile->size(), 8u);
      TS_ASSERT(dynamic_cast<GreenAlignmentFile*>(textFile));

      AlignmentFile* tpFile = AlignmentFile::create(tmpfile + ".tpa");
      TS_ASSERT(tpFile);
      TS_ASSERT_EQUALS(tpFile->size(), 8u);
      TS_ASSERT(dynamic_cast<TPAlignment*>(tpFile));

      Uint lengths[] = { 3, 3, 6, 15, 19, 21, 79, 234 };
      Uint first_word_al_count[] = { 2, 0, 0, 2, 2, 4, 2, 1 };
      vector<vector<Uint> > sets1, sets2;
      for (Uint i = 0; i < 8; ++i) {
         //cerr << endl << "Test case " << i << endl;
         TS_ASSERT(textFile->get(i, sets1));
         TS_ASSERT(tpFile->get(i, sets2));
         TS_ASSERT_EQUALS(sets1.size(), lengths[i]);
         TS_ASSERT_EQUALS(sets2.size(), lengths[i]);
         TS_ASSERT_EQUALS(sets1[0].size(), first_word_al_count[i]);
         TS_ASSERT_EQUALS(sets2[0].size(), first_word_al_count[i]);
         TS_ASSERT_EQUALS(sets1, sets2);
      }
      cerr << endl;

      TS_ASSERT(!textFile->get(8, sets1));
      TS_ASSERT(!tpFile->get(8, sets2));
   }

}; // class testAlignmentFile

} // namespace Portage
