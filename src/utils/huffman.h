
#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include "portage_defs.h"
#include "lazy_stl.h"
#include "errors.h"
#include "str_utils.h"
#include "binio.h"
#include "compact_phrase.h"
#include <tr1/unordered_map>
#include <boost/dynamic_bitset.hpp>

namespace Portage {

template <class T>
class HuffmanCoder {

   static const string MagicNumber;

   struct MemNode {
      MemNode* left;
      MemNode* right;
      double cost;
      size_t tp_size;
      string packed_encoding;
      T value;
      bool is_leaf;
      /// Recursive destructor
      ~MemNode() { delete left; delete right; }
      /// Constructor for readbin()
      MemNode() : left(NULL), right(NULL), tp_size(0) {}
      /// Constructor for a leaf node
      MemNode(const T& value, double cost) :
         left(NULL), right(NULL), cost(cost), tp_size(0), value(value), is_leaf(true) {}
      /// Constructor for an internal node
      MemNode(MemNode* left, MemNode* right, double cost) :
         left(left), right(right), cost(cost), tp_size(0), value(T()), is_leaf(false) {}
      /// No-op for in-memory nodes - they never need to be read from disk
      void initSelf() {}
      /// Serialize a node
      void writebin(ostream& os) const {
         BinIO::writebin(os, is_leaf);
         BinIO::writebin(os, cost);
         if (is_leaf) {
            BinIO::writebin(os, value);
         } else {
            left->writebin(os);
            right->writebin(os);
         }
      }
      /// Deserialize a node
      void readbin(istream& is) {
         BinIO::readbin(is, is_leaf);
         BinIO::readbin(is, cost);
         if (is_leaf) {
            BinIO::readbin(is, value);
         } else {
            left = new MemNode;
            left->readbin(is);
            right = new MemNode;
            right->readbin(is);
         }
      }
      /// Write a node in tightly-packed form
      void writeTP(ostream& os) const {
         if (is_leaf) {
            BinIO::writebin(os, value);
         } else {
            assert(left && left->tp_size);
            assert(right && right->tp_size);
            assert(!packed_encoding.empty());
            os << packed_encoding;
            left->writeTP(os);
            right->writeTP(os);
         }
      }
      /// Calculate the size of the node in tightly-packed form
      void precalcTP() {
         if (is_leaf) {
            tp_size = sizeof(T);
         } else {
            assert(left);
            assert(right);
            left->precalcTP();
            right->precalcTP();
            assert(left->tp_size);
            size_t encoded = (left->tp_size << 2) +
                             (left->is_leaf ? 2 : 0) +
                             (right->is_leaf ? 1 : 0);
            packed_encoding = CompactPhrase::packNumber(encoded);
            tp_size = packed_encoding.size() + left->tp_size + right->tp_size;
         }
      }
      /// Calculate the total cost of encoding the training distribution
      double totalCost(Uint depth) const {
         if (is_leaf)
            return depth * cost;
         else
            return left->totalCost(depth+1) + right->totalCost(depth+1);
      }
   }; 

   struct TPNode {
      TPNode* left;
      TPNode* right;
      const char* pos;
      T value;
      bool is_leaf;
      bool init;
      explicit TPNode(const char* pos, bool is_leaf)
         : left(NULL), right(NULL), pos(pos), is_leaf(is_leaf), init(false) {}
      ~TPNode() { delete left; delete right; }
      void initSelf() {
         if (!init) {
            if (is_leaf) {
               const char* pos_copy = pos;
               memcpy(&value, pos_copy, sizeof(value));
            } else {
               const char* pos_copy = pos;
               size_t encoded = CompactPhrase::unpackNumber(pos_copy);
               size_t left_size = encoded >> 2;
               bool left_is_leaf = encoded & 2;
               bool right_is_leaf = encoded & 1;
               left = new TPNode(pos_copy, left_is_leaf);
               right = new TPNode(pos_copy + left_size, right_is_leaf);
            }
            init = true;
         }
      }
   };

   MemNode* root;
   TPNode* tp_root;

   struct HigherFreq {
      bool operator()(const MemNode* x, const MemNode* y) {
         return x->cost > y->cost;
      }
   };

   /**
    * Run Huffman's algorithm to determine the optimal prefix code for
    * values [begin,end) given their frequencies/probabilities.
    * 
    * Type PairIterator has to have value type pair<T,some_numerical_type>
    * representing each value and its frequency or probability.
    *
    * @return  root of the decoding tree
    */
   template <class PairIterator>
   MemNode* Huffman(PairIterator begin, PairIterator end) {
      Uint n = 0;
      vector<MemNode*> Q;
      assert(begin!=end);
      for (PairIterator it = begin; it != end; ++it) {
         ++n;
         Q.push_back(new MemNode(it->first, it->second));
      }
      HigherFreq cmp;
      make_heap(Q.begin(), Q.end(), cmp);
      for (Uint i = 1; i < n; ++i) {
         MemNode* x = lazy::pop_heap(Q, cmp);
         MemNode* y = lazy::pop_heap(Q, cmp);
         MemNode* z = new MemNode(x, y, x->cost + y->cost);
         lazy::push_heap(Q, z, cmp);
      }
      assert(Q.size() == 1);
      return Q[0];
   }

   typedef tr1::unordered_map<T,boost::dynamic_bitset<> > EncodingMapT;
   EncodingMapT encodingMap;

   /// Recursive helper for buildEncodingMap
   template <class Node>
   void buildEncodingMapRec(Node* node, boost::dynamic_bitset<> prefix) {
      node->initSelf(); // Needed if Node==TPNode
      if (node->is_leaf) {
         encodingMap[node->value] = prefix;
      } else {
         assert(node->left);
         prefix.push_back(0);
         buildEncodingMapRec(node->left, prefix);
         assert(node->right);
         prefix[prefix.size()-1] = 1;
         buildEncodingMapRec(node->right, prefix);
         prefix.resize(prefix.size()-1);
      }
   }

   /**
    * Given a Huffman decoding tree, build a map from regular values to their encoding.
    */
   void buildEncodingMap() {
      encodingMap.clear();
      boost::dynamic_bitset<> prefix;
      if (tp_root) {
         tp_root->initSelf();
         assert(!tp_root->is_leaf);
         buildEncodingMapRec(tp_root, prefix);
      } else {
         assert(root);
         assert(!root->is_leaf);
         buildEncodingMapRec(root,prefix);
      }
   }

   /// Recursive helper for dumpTree()
   void dumpTreeRec(ostream& out, MemNode* node, string prefix) {
      if (node->is_leaf) {
         out << node->value << endl;
      } else {
         out << "0  ";
         dumpTreeRec(out, node->left, prefix + "   ");
         out << prefix << "1  ";
         dumpTreeRec(out, node->right, prefix + "   ");
      }
   }

public:

   /**
    * Write Huffman-encoded value to string dest.
    * @param dest    string holding the Huffman-encoded stream
    * @param offset  offset of the next free bit in the last byte of dest
    * @param value   value to encode and append
    * @return  offset of the next free bit in the last byte of dest, after
    *          having written value.
    */
   Uchar appendEncoded(string& dest, Uchar offset, T value) {
      if (encodingMap.empty())
         buildEncodingMap();
      assert(!encodingMap.empty());

      typename EncodingMapT::iterator it = encodingMap.find(value);
      if (it == encodingMap.end())
         error(ETFatal, "Trying to encode value not in Huffman dictionary.");
      const boost::dynamic_bitset<>& encoding(it->second);
      char* last_char = dest.empty() ? NULL : &dest[dest.size()-1];
      if (offset == 0) offset = 8;
      for (Uint i = 0; i < encoding.size(); ++i) {
         if (offset == 8) {
            dest.push_back(char(0));
            last_char = &dest[dest.size()-1];
            offset = 0;
         }
         assert(offset<8);
         if (encoding[i])
            *last_char |= (1 << (7-offset));
         ++offset;
      }
      return offset;
   }

   template <class Node>
   pair<const char*, Uchar> readEncoded(const char* src, Uchar offset, Node* node, T& dest) {
      node->initSelf(); // Needed when Node==TPNode
      while (!node->is_leaf) {
         assert(offset<8);
         bool bit = *src & (1 << (7-offset));
         node = bit ? node->right : node->left;
         assert(node);
         node->initSelf();
         ++offset;
         if (offset == 8) {
            ++src;
            offset = 0;
         }
      }
      assert(node->is_leaf);
      dest = node->value;
      return make_pair(src,offset);
   }

   /**
    * Read Huffman-encoded value from string src
    * @param src        encoded string holding a stream of values
    * @param offset     offset in first byte of src where to start reading
    * @param[out] dest  where to store the value read
    * @return pair<src,offset> to use for next read operation
    */
   pair<const char*, Uchar> readEncoded(const char* src, Uchar offset, T& dest) {
      if (tp_root) {
         assert(!tp_root->is_leaf);
         return readEncoded(src, offset, tp_root, dest);
      } else {
         assert(root && !root->is_leaf);
         return readEncoded(src, offset, root, dest);
      }
   }

   /// Write the Huffman encoding tree in human-readable form.
   void dumpTree(ostream& out) {
      out << endl;
      string prefix;
      dumpTreeRec(out, root, prefix);
   }

   /// Save the HuffmanCoder to disk for later use
   void writeCoder(ostream& out) {
      out << MagicNumber << endl;
      assert(root);
      assert(!root->is_leaf);
      root->writebin(out);
      out << endl << "End of " << MagicNumber << endl;
   }

   /// Save the HuffmanCode to disk in tightly packed format
   void writeTPCoder(ostream& out) {
      assert(movable_trait<T>::safe == true); // We can only TP code safe-movable types
      out << "Tightly Packed " << MagicNumber << endl;
      assert(root);
      assert(!root->is_leaf);
      root->precalcTP();
      out << CompactPhrase::packNumber(root->tp_size);
      root->writeTP(out);
      out << endl << "End of Tightly Packed " << MagicNumber << endl;
   }

   /// Open a tightly packed coder starting at address pos (typically linked to disk via memory-mapped IO)
   /// @param pos       starting position of the TP HuffmanCoder
   /// @param filename  name to use in error messages; not used in any other way
   void openTPCoder(const char* pos, const char* filename) {
      clear();
      string tpMagicString = "Tightly Packed " + MagicNumber + "\n";
      if (0 != tpMagicString.compare(0, tpMagicString.size(), pos, tpMagicString.size()))
         error(ETFatal, "Error reading tightly packed HuffmanCoder %s: does not start with expected header.", filename);
      pos += tpMagicString.size();
      Uint64 tree_size = CompactPhrase::unpackNumber(pos);
      string endString = "\nEnd of " + tpMagicString;
      if (0 != endString.compare(0, endString.size(), pos+tree_size, endString.size()))
         error(ETFatal, "Corrupted tightly packed HuffmanCoder %s: trailer missing or corrupted.\nExpected:<%s>\nGot:<%s>\n", filename, endString.c_str(), string(pos+tree_size, endString.size()).c_str());
      tp_root = new TPNode(pos, false);
   }

   /**
    * Read the HuffmanCoder from disk
    * @param in         stream to read from
    * @param filename   name of the actual input file, for error messages only
    */
   void readCoder(istream& in, const char* filename) {
      clear();

      string line;
      if (!getline(in, line) || line != MagicNumber) {
         error(ETFatal, "Error reading HuffmanCoder %s: does not start with expected header.", filename);
         return; // pointless, I know, but needed for unit tests to work.
      }
      root = new MemNode;
      root->readbin(in);
      if (!in)
         error(ETFatal, "Error reading HuffmanCoder %s: corrupted stream, ends too soon.", filename);
      if (!getline(in,line) ||
          line != "" ||
          !getline(in, line) ||
          line != "End of " + MagicNumber)
         error(ETFatal, "Error reading HuffmanCoder %s: trailer missing or corrupted.", filename);
   }

   /**
    * Constructor reading the HuffmanCoder from disk
    * @param in         stream to read from
    * @param filename   name of the actual input file, for error messages only
    */
   explicit HuffmanCoder(istream& in, const char* filename)
      : root(NULL), tp_root(NULL)
   {
      readCoder(in,filename);
   }

   /**
    * Build a coder from the distribution of values represented by [begin,end).
    *
    * Type PairIterator has to have value type pair<T,some_numerical_type>
    * representing each value and its frequency or probability.
    */
   template <class PairIterator>
   void buildCoder(PairIterator begin, PairIterator end) {
      clear();
      root = Huffman(begin,end);
   }

   /**
    * Return the total cost of encoding the distribution of values provided
    * when the coder was built.
    * Warning: cannot be called if *this was opened using openTPCoder().
    */
   double totalCost() const {
      assert(root);
      //cerr << root->cost << endl;
      return root->totalCost(0);
   }

   /**
    * This constructor initializes itself from the distribution of values
    * represented by [begin, end).
    *
    * Type PairIterator has to have value type pair<T,some_numerical_type>
    * representing each value and its frequency or probability.
    */
   template <class PairIterator>
   explicit HuffmanCoder(PairIterator begin, PairIterator end)
      : root(NULL), tp_root(NULL)
   {
      root = Huffman(begin,end);
   }

   /// Default constructor initializes an empty coder object; don't use without
   /// initializing, either by calling readCoder() or buildCoder().
   explicit HuffmanCoder() : root(NULL), tp_root(NULL) {}

   void clear() {
      delete root;    root    = NULL;
      delete tp_root; tp_root = NULL;
      encodingMap.clear();
   }

   /// Destructor
   ~HuffmanCoder() { delete root; delete tp_root; }

}; // class HuffmanCoder

template <class T>
const string HuffmanCoder<T>::MagicNumber = "Portage::HuffmanCoder binary format v1.0 with T=" + typeName<T>();

} // namespace Portage

#endif // _HUFFMAN_H_
