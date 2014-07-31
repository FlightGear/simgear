///@file
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SG_NASAL_HASH_HXX_
#define SG_NASAL_HASH_HXX_

#include "from_nasal.hxx"
#include "to_nasal.hxx"

#include <simgear/structure/map.hxx>
#include <boost/iterator/iterator_facade.hpp>

namespace nasal
{

  /**
   * Wrapper class for Nasal hashes.
   */
  class Hash
  {
    public:

      template<bool> class Entry;
      template<bool> class Iterator;

      typedef Entry<false>      reference;
      typedef Entry<true>       const_reference;
      typedef Iterator<false>   iterator;
      typedef Iterator<true>    const_iterator;

      /**
       * Create a new Nasal Hash
       *
       * @param c   Nasal context for creating the hash
       */
      explicit Hash(naContext c);

      /**
       * Initialize from an existing Nasal Hash
       *
       * @param hash  Existing Nasal Hash
       * @param c     Nasal context for creating new Nasal objects
       */
      Hash(naRef hash, naContext c);

      iterator begin();
      iterator end();
      const_iterator begin() const;
      const_iterator end() const;

      /**
       * Set member
       *
       * @param name    Member name
       * @param ref     Reference to Nasal object (naRef)
       */
      void set(const std::string& name, naRef ref);

      /**
       * Set member to anything convertible using to_nasal
       *
       * @param name    Member name
       * @param val     Value (has to be convertible with to_nasal)
       */
      template<class T>
      void set(const std::string& name, const T& val)
      {
        set(name, to_nasal(_context, val));
      }

      /**
       * Get member
       *
       * @param key    Member key
       */
      naRef get(naRef key) const;

      /**
       * Get member
       *
       * @param name    Member name
       */
      naRef get(const std::string& name) const;

      /**
       * Get member converted to given type
       *
       * @tparam T      Type to convert to (using from_nasal)
       * @param name    Member name
       */
      template<class T, class Key>
      T get(const Key& name) const
      {
        BOOST_STATIC_ASSERT(( boost::is_convertible<Key, naRef>::value
                           || boost::is_convertible<Key, std::string>::value
                           ));

        return from_nasal<T>(_context, get(name));
      }

      /**
       * Get member converted to callable object
       *
       * @tparam Sig    Function signature
       * @param name    Member name
       */
      template<class Sig, class Key>
      typename boost::enable_if< boost::is_function<Sig>,
                                 boost::function<Sig>
                               >::type
      get(const Key& name) const
      {
        BOOST_STATIC_ASSERT(( boost::is_convertible<Key, naRef>::value
                           || boost::is_convertible<Key, std::string>::value
                           ));

        return from_nasal_helper(_context, get(name), static_cast<Sig*>(0));
      }

      /**
       * Returns the number of entries in the hash
       */
      int size() const;

      /**
       * Get a list of all keys
       */
      std::vector<std::string> keys() const;

      /**
       * Create a new child hash (module)
       *
       * @param name  Name of the new hash inside this hash
       */
      Hash createHash(const std::string& name);

      /**
       * Set a new Nasal context. Eg. in FlightGear the context changes every
       * frame, so if using the same Hash instance for multiple frames you have
       * to update the context before using the Hash object.
       */
      void setContext(naContext context);

      /**
       * Get Nasal representation of Hash
       */
      naRef get_naRef() const;

      /**
       * Get Nasal vector of keys
       */
      naRef get_naRefKeys() const;

      /// Hash entry
      template<bool is_const>
      class Entry
      {
        public:
          typedef typename boost::mpl::if_c<
            is_const,
            Hash const*,
            Hash*
          >::type HashPtr;

          Entry(HashPtr hash, naRef key):
            _hash(hash),
            _key(key)
          {
            assert(hash);
            assert(naIsScalar(key));
          }

          std::string getKey() const
          {
            if( !_hash || naIsNil(_key) )
              return std::string();

            return from_nasal<std::string>(_hash->_context, _key);
          }

          template<class T>
          T getValue() const
          {
            if( !_hash || naIsNil(_key) )
              return T();

            return _hash->template get<T>(_key);
          }

        private:
          HashPtr _hash;
          naRef _key;

      };

      /// Hash iterator
      template<bool is_const>
      class Iterator:
        public boost::iterator_facade<
          Iterator<is_const>,
          Entry<is_const>,
          boost::bidirectional_traversal_tag,
          Entry<is_const>
        >
      {
        public:
          typedef typename Entry<is_const>::HashPtr HashPtr;
          typedef Entry<is_const>                   value_type;

          Iterator():
            _hash(NULL),
            _index(0)
          {}

          Iterator(HashPtr hash, int index):
            _hash(hash),
            _index(index)
          {}

          /**
           * Convert from iterator to const_iterator or copy within same type
           */
          template<bool is_other_const>
          Iterator( Iterator<is_other_const> const& other,
                    typename boost::enable_if_c< is_const || !is_other_const,
                                                 void*
                                               >::type = NULL ):
            _hash(other._hash),
            _index(other._index)
          {}

        private:
          friend class boost::iterator_core_access;
          template <bool> friend class Iterator;

          HashPtr _hash;
          int _index;

          template<bool is_other_const>
          bool equal(Iterator<is_other_const> const& other) const
          {
            return _hash == other._hash
                && _index == other._index;
          }

          void increment() { ++_index; }
          void decrement() { --_index; }

          value_type dereference() const
          {
            return value_type(_hash, naVec_get(_hash->get_naRefKeys(), _index));
          }
      };

    protected:

      naRef _hash;
      naContext _context;

      mutable naRef _keys; //< Store vector of keys (for iterators)

  };

} // namespace nasal

template<class Value>
simgear::Map<std::string, Value>
from_nasal_helper( naContext c,
                   naRef ref,
                   const simgear::Map<std::string, Value>* )
{
  nasal::Hash hash = from_nasal_helper(c, ref, static_cast<nasal::Hash*>(0));

  simgear::Map<std::string, Value> map;
  for(nasal::Hash::const_iterator it = hash.begin(); it != hash.end(); ++it)
    map[ it->getKey() ] = it->getValue<Value>();

  return map;
}

#endif /* SG_NASAL_HASH_HXX_ */
