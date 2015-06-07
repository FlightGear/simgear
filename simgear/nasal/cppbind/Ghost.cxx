// Expose C++ objects to Nasal as ghosts
//
// Copyright (C) 2014 Thomas Geymayer <tomgey@gmail.com>
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

