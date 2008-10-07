/**
 * @author Nicola Ueffing
 * @file ngram_tree.cc   Abstract interface and a few simple instantiations
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include "ngram_tree.h"

using namespace Portage;
using namespace std;

//---------------------------------------------------------------------------------------
// NgramNode
//---------------------------------------------------------------------------------------
void NgramNode::clear() {
   for (set<NgramNode*>::iterator itr=kids.begin(); itr!=kids.end(); itr++) {
      (*itr)->clear();
      delete *itr;
   }
   kids.clear();
}

void NgramNode::resetPost() {
   c.reset();
}

NgramNode* NgramNode::getKid(const string &word) {
   for (set<NgramNode*>::const_iterator itr=kids.begin(); itr!=kids.end(); itr++)
      if ((*itr)->w == word)
         return *itr;
   return NULL;
}

void NgramNode::addKid(NgramNode* n) {
   kids.insert(n);
}

void NgramNode::update(double d, int n) {
   c.update(d,n);
}

ConfScore NgramNode::getPost() {
   return c;
}

void NgramNode::normalize(const ConfScore &cnf) {
   for (set<NgramNode*>::iterator itr=kids.begin(); itr!=kids.end(); itr++)
      (*itr)->normalize(cnf);
   c.normalize(cnf);
}

//---------------------------------------------------------------------------------------
// NgramTree
//---------------------------------------------------------------------------------------
void NgramTree::init() {
   if (root == NULL)
      root = new NgramNode();
   else
      root->resetPost();
   numNodes = 0;
}

void NgramTree::clear() {
   root->clear();
}

void NgramTree::update(const vector<string> &ngram, double d, int n){
   // Unigram
   const string& lastword = ngram.back();
   assert(root != NULL);
   NgramNode *lastw = root->getKid(lastword);
   if (lastw == NULL) {
      lastw = new NgramNode(lastword);
      root->addKid(lastw);
      numNodes++;
   }
   lastw->update(d,n);
   root->update(d,n);   // total prob. mass
   //    cerr << "total prob. mass: " << root->getPost().prob() << endl;

   // 2- to n-gram
   for (Uint i=0; i<ngram.size()-1; i++) {
      NgramNode* node = root->getKid(ngram[i]);
      for (Uint j=i+1; j<ngram.size()-1; j++) {
         assert(node != NULL);
         node = node->getKid(ngram[j]);
      }
      assert(node != NULL);
      lastw = node->getKid(lastword);
      if (lastw == NULL) {
         lastw = new NgramNode(lastword);
         node->addKid(lastw);
         numNodes++;
      } // if
      lastw->update(d,n);
      root->update(d,n);   // total prob. mass
      //	cerr << "total prob. mass: " << root->getPost().prob() << endl;
   } // for i
}

bool NgramTree::find(const vector<string> &ngram, int len) {

   int i = ngram.size()-len;
   vector<string>::const_iterator itr=ngram.begin() + i;
   assert(root != NULL);
   NgramNode *node = root;

   while (itr!=ngram.end()) {
      // cerr << "look for " << *itr << endl;
      node = node->getKid(*itr);
      if (node==NULL)
         return false;
      itr++;
   }
   return true;
}

int NgramTree::size() {
   return numNodes;
}

NgramNode* NgramTree::getRoot() {
   return root;
}

void NgramTree::normalize() {
   root->normalize(root->getPost());
}

ConfScore NgramTree::getPost(vector<string> &ng) {
   NgramNode* node = root;
   for (vector<string>::const_iterator itr=ng.begin(); itr!=ng.end(); itr++)
      node = node->getKid(*itr);
   assert(node != NULL);
   return node->getPost();
}

