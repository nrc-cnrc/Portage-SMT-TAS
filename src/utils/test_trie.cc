/**
 * @author Eric Joanis
 * @file test_trie.cc  Test harness for trie.h
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches Canada / Copyright 2006, National Research Council Canada
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

      float val;
      bool result = trie.find(key, tokens.size(), val);
      cout << "find(" << line << ") returned " << result;
      if ( result ) cout << " val = " << val;

      float internal_val = trie.get_internal_node_value(key, tokens.size());
      if ( internal_val != 0 )
         cout << " internal data = " << internal_val;

      cout << endl;
   }

   cout << endl << "Trie dump with traverse:" << endl;
   TriePrintVisitor v;
   trie.traverse(v);
   //trie.dump(os);

   cout << endl << "Trie dump with iterators:" << endl;
   vector<Uint> key_prefix;
   rec_dump_trie(key_prefix, trie.begin(), trie.end());

   //cout << "num entries = " << trie.size() << ", num bytes = " << trie.numBytes() << endl;
}

// arg processing

void getArgs(int argc, const char* const argv[])
{
   char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "infile", &isp, ifs);
   arg_reader.testAndSet(1, "outfile", &osp, ofs);
}

