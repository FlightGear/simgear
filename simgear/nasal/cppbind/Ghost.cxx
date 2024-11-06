///@brief Expose C++ objects to Nasal as ghosts
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014  Thomas Geymayer <tomgey@gmail.com>

#include "Ghost.hxx"

namespace nasal
{
  namespace internal
  {
    //--------------------------------------------------------------------------
    GhostMetadata::DestroyList GhostMetadata::_destroy_list;

    //--------------------------------------------------------------------------
    void GhostMetadata::addNasalBase(const naRef& parent)
    {
      assert( naIsHash(parent) );
      _parents.push_back(parent);
    }

    //--------------------------------------------------------------------------
    bool GhostMetadata::isInstance(naGhostType* ghost_type, bool& is_weak) const
    {
      if( ghost_type == _ghost_type_strong_ptr )
      {
        is_weak = false;
        return true;
      }

      if( ghost_type == _ghost_type_weak_ptr )
      {
        is_weak = true;
        return true;
      }

      return false;
    }

    //--------------------------------------------------------------------------
    GhostMetadata::GhostMetadata( const std::string& name,
                                  const naGhostType* ghost_type_strong,
                                  const naGhostType* ghost_type_weak ):
      _name_strong(name),
      _name_weak(name + " (weak ref)"),
      _ghost_type_strong_ptr(ghost_type_strong),
      _ghost_type_weak_ptr(ghost_type_weak)
    {

    }

    //--------------------------------------------------------------------------
    void GhostMetadata::addDerived(const GhostMetadata* derived)
    {
      assert(derived);
    }

    //--------------------------------------------------------------------------
    naRef GhostMetadata::getParents(naContext c)
    {
      return nasal::to_nasal(c, _parents);
    }
  } // namespace internal

  //----------------------------------------------------------------------------
  void ghostProcessDestroyList()
  {
    using internal::GhostMetadata;
    typedef GhostMetadata::DestroyList::const_iterator destroy_iterator;
    for( destroy_iterator it = GhostMetadata::_destroy_list.begin();
                          it != GhostMetadata::_destroy_list.end();
                        ++it )
      it->first(it->second);
    GhostMetadata::_destroy_list.clear();
  }

} // namespace nasal

