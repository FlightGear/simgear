// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief  Compare lists and get differences
 */

#ifndef SG_LISTDIFF_HXX_
#define SG_LISTDIFF_HXX_

#include <vector>

namespace simgear
{

  template<class T>
  struct ListDiff
  {
    typedef std::vector<T> List;
    typedef std::function<void (T)> Callback;

    /**
     * Perform list diff in-place (modifies both lists) and call cb_add for
     * every element in new_list not in old_list and cb_remove for every element
     * in old_list not in new_list.
     */
    static void inplace( List& old_list,
                         List& new_list,
                         Callback cb_add,
                         Callback cb_remove )
    {
      // Check which elements have been removed. (Removing first and adding
      // second should keep the memory usage lower - not for this function, but
      // probably for users of this function which use the callbacks to delete
      // and create objects)
      while( !old_list.empty() )
      {
        T& old_el = old_list.front();
        typename List::iterator new_el =
          std::find(new_list.begin(), new_list.end(), old_el);

        if( new_el == new_list.end() )
        {
          if( cb_remove )
            cb_remove(old_el);
        }
        else
        {
          // Element is in both lists -> just ignore
          *new_el = new_list.back();
          new_list.pop_back();
        }

        old_list.front() = old_list.back();
        old_list.pop_back();
      }

      // All remaing elements in new_list have not been in old_list, so call
      // the add callback for every element if required.
      if( cb_add )
      {
        for( typename List::iterator it = new_list.begin();
                                     it != new_list.end();
                                   ++it )
          cb_add(*it);
      }
    }
  };

} // namespace simgear

#endif /* SG_LISTDIFF_HXX_ */
