/**
 * @author Eric Joanis
 * @file test_trie.cc  Test harness for trie.h
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "file_utils.h"
#include "trie.h"
#include "arg_reader.h"
#include "errors.h"
#include <stdexcept>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
test_trie [-v] [infile [outfile]]\n\
\n\
Enter lines from <infile> into trie until a blank line is encountered.\n\
Then treat the rest of <infile> as find queries, with result to stderr\n\
Finally dump contents to <outfile> (should match infile, module reordering),\n\
and write total size in bytes to stderr.\n\
\n\
";

static bool verbose = false;
static iMagicStream ifs;
static oMagicStream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
void getArgs(int argc, const char* const argv[]);


// Visitor to print the test trie
class TriePrintVisitor {
 public:
   void operator()(const vector<Uint> &key, float value) {
      for ( vector<Uint>::const_iterator it = key.begin();
            it != key.end(); ++it )
         cout << *it << " ";
      cout << "= " << value << endl; 
   }
};

void rec_dump_trie(
   vector<Uint>& key_prefix,
   PTrie<float, Wrap<float>, false>::iterator begin,
   const PTrie<float, Wrap<float>, false>::iterator& end
) {
   for ( ; begin != end; ++begin ) {
      key_prefix.push_back(begin.get_key());
      if ( begin.is_leaf() ) {
         for ( vector<Uint>::const_iterator it = key_prefix.begin();
               it != key_prefix.end(); ++it )
            cout << *it << " ";
         cout << "= " << begin.get_value() << endl;
      }
      if ( begin.has_children() )
         rec_dump_trie(key_prefix, begin.begin_children(), begin.end_children());
      key_prefix.pop_back();
   }
}

bool filter_1(const vector<Uint>& key_stack) {
   return key_stack.back() != 1;
}

struct InvertMap {
   Uint operator()(Uint key) {
      if ( 100 <= key && key <= 103 ) return (200 + 1024 * (key % 100));
      if ( key % 1024 == 100 ) return (100 + key / 1024);
      return 4*1024 - key;
   }
};

int main(int argc, char** argv)
{
   getArgs(argc, argv);
   istream& is = *isp;
   //ostream& os = *osp;

   PTrie<float, Wrap<float>, false> trie;

   string line;
   // insert phase
   while (getline(is, line) && line != "") {
      vector<string> tokens;
      split(line,tokens);

      bool has_internal_val(false);
      float internal_val;
      if (tokens[0][0] == 'i' || tokens[0][0] == 'I') {
         conv(tokens[0].substr(1), internal_val);
         has_internal_val = true;
         tokens.erase(tokens.begin(), tokens.begin()+1);
      }

      if ( tokens.size() < 2 ) {
         error(ETWarn, "Not enough tokens on line '%s'--skipping",
               line.c_str());
         continue;
      }

      float val;
      size_t key_size = tokens.size() - 1;
      Uint key[key_size];
      conv(tokens[0], val);
      for (size_t i = 1; i < tokens.size(); ++i) {
         conv(tokens[i], key[i-1]);
      }
      trie.insert(key, key_size, val);
      cout << "Insert(" << line << ")";
      if (has_internal_val) {
         trie.set_internal_node_value(key, key_size, internal_val);
         cout << " (with internal node value)";
      }
      cout << endl;
   }

   cout << endl << "Queries:" << endl;

   while (getline(is, line)) {
      vector<string> tokens;
      split(line, tokens);

      Uint key[tokens.size()];
      for (size_t i = 0; i < tokens.size(); ++i) {
         conv(tokens[i], key[i]);
      }

      float val(0.0);
      bool result = trie.find(key, tokens.size(), val);
      cout << "find(" << line << ") returned " << result;
      if ( result ) cout << " val = " << val;

      float internal_val = trie.get_internal_node_value(key, tokens.size());
      if ( internal_val != 0 )
         cout << " internal data = " << internal_val;

      cout << endl;
      float * values[tokens.size()];
      Uint depth = trie.find_path(key, tokens.size(), values);
      cout << "find_path(" << line << ") returned depth " << depth 
           << " and values ";
      for ( Uint i(0); i < tokens.size(); ++i ) {
         if ( values[i] )
            cout << *(values[i]) << " ";
         else
            cout << "NULL ";
      }
      cout << ".";

      cout << endl;
   }

   cout << endl << "Trie dump with traverse:" << endl;
   TriePrintVisitor v;
   trie.traverse(v);
   //trie.dump(os);

   cout << endl << "Trie dump with iterators:" << endl;
   vector<Uint> key_prefix;
   rec_dump_trie(key_prefix, trie.begin_children(), trie.end_children());

   cout << endl << "Write binary" << endl;
   {
      ofstream ofs("test_trie.binary");
      trie.write_binary(ofs);
   }

   cout << endl << "Read binary with no filter no map:" << endl;
   {
      iSafeMagicStream ifs("test_trie.binary");
      PTrie<float, Wrap<float>, false> new_trie;
      new_trie.read_binary(ifs);
      new_trie.traverse(v);
   }

   cout << endl << "Read binary with filter 1 no map:" << endl;
   {
      iSafeMagicStream ifs("test_trie.binary");
      PTrie<float, Wrap<float>, false> new_trie;
      new_trie.read_binary(ifs, filter_1);
      new_trie.traverse(v);
   }

   cout << endl << "Read binary with no filter and object map:" << endl;
   {
      iSafeMagicStream ifs("test_trie.binary");
      PTrie<float, Wrap<float>, false> new_trie;
      InvertMap m;
      new_trie.read_binary(ifs, PTrieKeepAll, m);
      new_trie.traverse(v);
   }

   cout << endl << "Read binary with filter 1 and object map:" << endl;
   {
      iSafeMagicStream ifs("test_trie.binary");
      PTrie<float, Wrap<float>, false> new_trie;
      InvertMap m;
      new_trie.read_binary(ifs, filter_1, m);
      new_trie.traverse(v);
   }

   //cout << "num entries = " << trie.size() << ", num bytes = " << trie.numBytes() << endl;
   
   cerr << endl;
}

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "infile", &isp, ifs);
   arg_reader.testAndSet(1, "outfile", &osp, ofs);
}

