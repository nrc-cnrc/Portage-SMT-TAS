/**
 * @author George Foster
 * @file vcb.h  Simple virtual constructor mechanism.
 * 
 * 
 * COMMENTS:
 *
 * This is useful when you have an interface and a set of derived classes, and
 * want to choose a particular derived class at runtime based on a string
 * specification. To use it, you need to define a string-based constructor for
 * each derived class, and initialize a table listing all derived classes. For
 * example, suppose the interface class is called "Model", and the derived
 * classes are "A", "B", and "C". Then the table would be initialized like
 * this (the NULL line at the end is mandatory; don't forget it!):
 *
 * VCB<Model>::TInfo VCB<Model>::tinfos[] = {
 *    {VCBSub<Model,A>::create, "A", "optional description of A's arg string"},
 *    {VCBSub<Model,B>::create, "B", "optional description of B's arg string"},
 *    {VCBSub<Model,C>::create, "C", "optional description of C's arg string"},
 *    {NULL, "", ""}
 * };
 *
 * Then given a string "name" specifying a derived class, and a string "args"
 * specifying its constructor arguments, do this to construct an instance:
 *
 * Model* m = VCB<Model>::create(name, args);
 *
 * To print subclass and argument descriptions for a user, either call help()
 * or access the tinfos[] table directly.
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / 
 * Copyright 2005, National Research Council of Canada
 */
#ifndef VCB_H
#define VCB_H

#include "errors.h"
#include <string>

namespace Portage {

/// Object creator.  This template allows to create a derived class and returns it.
template<class B, class S> class VCBSub 
{
public:
   /**
   * Creates an object of type S.
   * @param s  Creates an object using the string constructor
   * @return Returns a base class pointer on a new S
   */
   static B* create(const string& s) {
      return new S(s);
   }
};

/// A creation factory.
template<class T> class VCB
{
public:
   
   /// Creation function pointer.
   typedef T* (*PF)(const string& s);

   /// A unit containing all necessaire info to create an object.
   struct TInfo {
      PF pf;			///< pointer to create() function
      string tname;		///< name of derived class
      string help;		///< describes args for derived class constructor
   };

   static TInfo tinfos[];       ///< Our table of info unit

   /**
    * Create a new instance of class T. 
    *
    * @param tname identifier for derived type, by convention the class name
    * @param args argument string to pass to constructor
    * @param fail die with error message if true & tname is unknown
    * @return pointer to new class; free with delete
    */
   static T* create(const string& tname, const string& args, bool fail = true) {
      for (Uint i = 0; tinfos[i].pf; ++i)
	 if (tname == tinfos[i].tname)
	    return tinfos[i].pf(args);
      if (fail)
	 error(ETFatal, "unknown class name: " + tname);
      return NULL;
   }

   /**
    * Return a help string listing all derived types and their arguments.
    * @return help string
    */
   static string help() {
      string h;
      for (Uint i = 0; tinfos[i].pf; ++i)
	 h += tinfos[i].tname + " " + tinfos[i].help + "\n";
      return h;
   }

   /**
    * Return help argument for type tname.
    *
    * @param tname Name of hte subclass that we want help.
    * @return Returns an help string for tname
    */
   static string help(const string& tname) {
      for (Uint i = 0; tinfos[i].pf; ++i)
	 if (tname == tinfos[i].tname)
	    return tinfos[i].help;
      return "";
   }
};

}

#endif
