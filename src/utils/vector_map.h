/**
 * @author Aaron Tikuisis
 * @file vector_map.h A replacement for hash_map in particular cases.
 * $Id$
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 *
 * An extension of stl's vector, with part of the Associative Container interface
 * implemented.  This may be preferable to a hash_map when the table won't be very big and
 * space is a concern.  Note that the find() function returns a normal vector iterator
 * that iterates through ALL elements in the vector following the one found.
 */

#ifndef VECTOR_MAP_H
#define VECTOR_MAP_H

#include <vector>
#include <utility>

using namespace std;

namespace Portage
{
    /**
     * A replacement for hash_map in particular cases.  An extension of stl's
     * vector, with part of the Associative Container interface implemented.
     * This may be preferable to a hash_map when the table won't be very big
     * and space is a concern.  Note that the find() function returns a normal
     * vector iterator that iterates through ALL elements in the vector
     * following the one found.
     */
    template <class Key, class Data>
    class vector_map: public vector< pair<Key, Data> >
    {
    public:
        //@{
        /// Needed to compile with g++ 3.4.3
        using vector< pair<Key, Data> >::begin;
        using vector< pair<Key, Data> >::back;
        using vector< pair<Key, Data> >::end;
        //@}

        //@{
        /// type definition for vector map.
        typedef Key key_type;
        typedef Data data_type;
        //@}

        /**
         * Finds a key.
         * @param key the key to be found
         * @return Returns a iterator that points to the key value or
         *         the end iterator if not found
         */
        typename vector< pair<key_type, data_type> >::const_iterator find(const
                key_type &key) const
        {
            typename vector< pair<key_type, data_type> >::const_iterator it = begin();
            for (; it != end(); it++)
            {
                if (key == it->first) break;
            } // for
            return it;
        } // find

        /**
         * Finds a key.
         * @param key the key to be found
         * @return Returns a iterator that points to the key value or
         *         the end iterator if not found
         */
        typename vector< pair<key_type, data_type> >::iterator find(const key_type &key)
        {
            typename vector< pair<key_type, data_type> >::iterator it = begin();
            for (; it != end(); it++)
            {
                if (key == it->first) break;
            } // for
            return it;
        } // find

        /**
         * Finds a key and returns a reference on its value or adds a key.
         * @param key the key to be found
         * @return Returns a reference on the key's value or add a new key
         */
        data_type &operator[](const key_type &key)
        {
            typename vector< pair<key_type, data_type> >::iterator it = find(key);
            if (it == end())
            {
                push_back(make_pair(key, Data()));
                return back().second;
            } else
            {
                return it->second;
            } // if
        } // operator[]
    }; // vector_map
} // Portage

#endif // VECTOR_MAP_H

