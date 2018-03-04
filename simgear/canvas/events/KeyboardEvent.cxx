///@file
/// Keyboard event
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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

#include <simgear_config.h>

#include "KeyboardEvent.hxx"

#include <osgGA/GUIEventAdapter>

#include <boost/version.hpp>
#if BOOST_VERSION >= 104800
# include <boost/container/flat_map.hpp>
# include <boost/container/flat_set.hpp>
#else
# include <map>
# include <set>
#endif

#include <iterator>

namespace simgear
{
namespace canvas
{
  typedef osgGA::GUIEventAdapter EA;

  // TODO check Win/Mac keycode for altgr/ISO Level3 Shift
  const uint32_t KEY_AltGraph = 0xfe03;
  

  //----------------------------------------------------------------------------
  KeyboardEvent::KeyboardEvent():
    _key(0),
    _unmodified_key(0),
    _repeat(false),
    _location(DOM_KEY_LOCATION_STANDARD)
  {

  }

  //----------------------------------------------------------------------------
  KeyboardEvent::KeyboardEvent(const osgGA::GUIEventAdapter& ea):
    DeviceEvent(ea),
    _key(ea.getKey()),
    _unmodified_key(ea.getUnmodifiedKey()),
    _repeat(false),
    _location(DOM_KEY_LOCATION_STANDARD)
  {
    if( ea.getEventType() == EA::KEYDOWN )
      type = KEY_DOWN;
    else if( ea.getEventType() == EA::KEYUP )
      type = KEY_UP;
//    else
//      // TODO what to do with wrong event type?
  }

  //----------------------------------------------------------------------------
  KeyboardEvent* KeyboardEvent::clone(int type) const
  {
    auto event = new KeyboardEvent(*this);
    event->type = type;
    return event;
  }

  //----------------------------------------------------------------------------
  void KeyboardEvent::setKey(uint32_t key)
  {
    _name.clear();
    _key = key;
  }

  //----------------------------------------------------------------------------
  void KeyboardEvent::setUnmodifiedKey(uint32_t key)
  {
    _name.clear();
    _unmodified_key = key;
  }

  //----------------------------------------------------------------------------
  void KeyboardEvent::setRepeat(bool repeat)
  {
    _repeat = repeat;
  }

  //----------------------------------------------------------------------------
  std::string KeyboardEvent::key() const
  {
    if( !_name.empty() )
      return _name;

    // We need to make sure only valid const char* pointers are passed. The best
    // way is just to use string constants.
    // Use an empty string ("") to just use the value reported by the operating
    // system.
    typedef std::pair<const char*, uint8_t> InternalKeyInfo;

#if BOOST_VERSION >= 104800
    typedef boost::container::flat_map<int, InternalKeyInfo> InternalKeyMap;
    typedef boost::container::flat_set<int> KeyList;
#else
#   warning "Use Boost >= 1.48 for faster and more memory efficient key lookup"
    typedef std::map<int, InternalKeyInfo> InternalKeyMap;
    typedef std::set<int> KeyList;
#endif

    static InternalKeyMap key_map;
    static KeyList num_pad_keys;

    if( key_map.empty() )
    {
      const uint8_t S = DOM_KEY_LOCATION_STANDARD,
                    L = DOM_KEY_LOCATION_LEFT,
                    R = DOM_KEY_LOCATION_RIGHT,
                    N = DOM_KEY_LOCATION_NUMPAD;

      key_map[ EA::KEY_BackSpace    ] = std::make_pair("Backspace", S);
      key_map[ EA::KEY_Tab          ] = std::make_pair("Tab", S);
      key_map[ EA::KEY_Linefeed     ] = std::make_pair("Linefeed", S);
      key_map[ EA::KEY_Clear        ] = std::make_pair("Clear", S);
      key_map[ EA::KEY_Return       ] = std::make_pair("Enter", S);
      key_map[ EA::KEY_Pause        ] = std::make_pair("Pause", S);
      key_map[ EA::KEY_Scroll_Lock  ] = std::make_pair("ScrollLock", S);
      key_map[ EA::KEY_Sys_Req      ] = std::make_pair("SystemRequest", S);
      key_map[ EA::KEY_Escape       ] = std::make_pair("Escape", S);
      key_map[ EA::KEY_Delete       ] = std::make_pair("Delete", S);

      key_map[ EA::KEY_Home         ] = std::make_pair("Home", S);
      key_map[ EA::KEY_Left         ] = std::make_pair("Left", S);
      key_map[ EA::KEY_Up           ] = std::make_pair("Up", S);
      key_map[ EA::KEY_Right        ] = std::make_pair("Right", S);
      key_map[ EA::KEY_Down         ] = std::make_pair("Down", S);
      key_map[ EA::KEY_Page_Up      ] = std::make_pair("PageUp", S);
      key_map[ EA::KEY_Page_Down    ] = std::make_pair("PageDown", S);
      key_map[ EA::KEY_End          ] = std::make_pair("End", S);
      key_map[ EA::KEY_Begin        ] = std::make_pair("Begin", S);

      key_map[ EA::KEY_Select       ] = std::make_pair("Select", S);
      key_map[ EA::KEY_Print        ] = std::make_pair("PrintScreen", S);
      key_map[ EA::KEY_Execute      ] = std::make_pair("Execute", S);
      key_map[ EA::KEY_Insert       ] = std::make_pair("Insert", S);
      key_map[ EA::KEY_Undo         ] = std::make_pair("Undo", S);
      key_map[ EA::KEY_Redo         ] = std::make_pair("Redo", S);
      key_map[ EA::KEY_Menu         ] = std::make_pair("ContextMenu", S);
      key_map[ EA::KEY_Find         ] = std::make_pair("Find", S);
      key_map[ EA::KEY_Cancel       ] = std::make_pair("Cancel", S);
      key_map[ EA::KEY_Help         ] = std::make_pair("Help", S);
      key_map[ EA::KEY_Break        ] = std::make_pair("Break", S);
      key_map[ EA::KEY_Mode_switch  ] = std::make_pair("ModeChange", S);
      key_map[ EA::KEY_Num_Lock     ] = std::make_pair("NumLock", S);

      key_map[ EA::KEY_KP_Space     ] = std::make_pair(" ", N);
      key_map[ EA::KEY_KP_Tab       ] = std::make_pair("Tab", N);
      key_map[ EA::KEY_KP_Enter     ] = std::make_pair("Enter", N);
      key_map[ EA::KEY_KP_F1        ] = std::make_pair("F1", N);
      key_map[ EA::KEY_KP_F2        ] = std::make_pair("F2", N);
      key_map[ EA::KEY_KP_F3        ] = std::make_pair("F3", N);
      key_map[ EA::KEY_KP_F4        ] = std::make_pair("F4", N);
      key_map[ EA::KEY_KP_Home      ] = std::make_pair("Home", N);
      key_map[ EA::KEY_KP_Left      ] = std::make_pair("Left", N);
      key_map[ EA::KEY_KP_Up        ] = std::make_pair("Up", N);
      key_map[ EA::KEY_KP_Right     ] = std::make_pair("Right", N);
      key_map[ EA::KEY_KP_Down      ] = std::make_pair("Down", N);
      key_map[ EA::KEY_KP_Page_Up   ] = std::make_pair("PageUp", N);
      key_map[ EA::KEY_KP_Page_Down ] = std::make_pair("PageDown", N);
      key_map[ EA::KEY_KP_End       ] = std::make_pair("End", N);
      key_map[ EA::KEY_KP_Begin     ] = std::make_pair("Begin", N);
      key_map[ EA::KEY_KP_Insert    ] = std::make_pair("Insert", N);
      key_map[ EA::KEY_KP_Delete    ] = std::make_pair("Delete", N);
      key_map[ EA::KEY_KP_Equal     ] = std::make_pair("=", N);
      key_map[ EA::KEY_KP_Multiply  ] = std::make_pair("*", N);
      key_map[ EA::KEY_KP_Add       ] = std::make_pair("+", N);
      key_map[ EA::KEY_KP_Separator ] = std::make_pair("", N);
      key_map[ EA::KEY_KP_Subtract  ] = std::make_pair("-", N);
      key_map[ EA::KEY_KP_Decimal   ] = std::make_pair("", N);
      key_map[ EA::KEY_KP_Divide    ] = std::make_pair("/", N);

      key_map[ EA::KEY_KP_0 ] = std::make_pair("0", N);
      key_map[ EA::KEY_KP_1 ] = std::make_pair("1", N);
      key_map[ EA::KEY_KP_2 ] = std::make_pair("2", N);
      key_map[ EA::KEY_KP_3 ] = std::make_pair("3", N);
      key_map[ EA::KEY_KP_4 ] = std::make_pair("4", N);
      key_map[ EA::KEY_KP_5 ] = std::make_pair("5", N);
      key_map[ EA::KEY_KP_6 ] = std::make_pair("6", N);
      key_map[ EA::KEY_KP_7 ] = std::make_pair("7", N);
      key_map[ EA::KEY_KP_8 ] = std::make_pair("8", N);
      key_map[ EA::KEY_KP_9 ] = std::make_pair("9", N);

      key_map[ EA::KEY_F1   ] = std::make_pair("F1", S);
      key_map[ EA::KEY_F2   ] = std::make_pair("F2", S);
      key_map[ EA::KEY_F3   ] = std::make_pair("F3", S);
      key_map[ EA::KEY_F4   ] = std::make_pair("F4", S);
      key_map[ EA::KEY_F5   ] = std::make_pair("F5", S);
      key_map[ EA::KEY_F6   ] = std::make_pair("F6", S);
      key_map[ EA::KEY_F7   ] = std::make_pair("F7", S);
      key_map[ EA::KEY_F8   ] = std::make_pair("F8", S);
      key_map[ EA::KEY_F9   ] = std::make_pair("F9", S);
      key_map[ EA::KEY_F10  ] = std::make_pair("F10", S);
      key_map[ EA::KEY_F11  ] = std::make_pair("F11", S);
      key_map[ EA::KEY_F12  ] = std::make_pair("F12", S);
      key_map[ EA::KEY_F13  ] = std::make_pair("F13", S);
      key_map[ EA::KEY_F14  ] = std::make_pair("F14", S);
      key_map[ EA::KEY_F15  ] = std::make_pair("F15", S);
      key_map[ EA::KEY_F16  ] = std::make_pair("F16", S);
      key_map[ EA::KEY_F17  ] = std::make_pair("F17", S);
      key_map[ EA::KEY_F18  ] = std::make_pair("F18", S);
      key_map[ EA::KEY_F19  ] = std::make_pair("F19", S);
      key_map[ EA::KEY_F20  ] = std::make_pair("F20", S);
      key_map[ EA::KEY_F21  ] = std::make_pair("F21", S);
      key_map[ EA::KEY_F22  ] = std::make_pair("F22", S);
      key_map[ EA::KEY_F23  ] = std::make_pair("F23", S);
      key_map[ EA::KEY_F24  ] = std::make_pair("F24", S);
      key_map[ EA::KEY_F25  ] = std::make_pair("F25", S);
      key_map[ EA::KEY_F26  ] = std::make_pair("F26", S);
      key_map[ EA::KEY_F27  ] = std::make_pair("F27", S);
      key_map[ EA::KEY_F28  ] = std::make_pair("F28", S);
      key_map[ EA::KEY_F29  ] = std::make_pair("F29", S);
      key_map[ EA::KEY_F30  ] = std::make_pair("F30", S);
      key_map[ EA::KEY_F31  ] = std::make_pair("F31", S);
      key_map[ EA::KEY_F32  ] = std::make_pair("F32", S);
      key_map[ EA::KEY_F33  ] = std::make_pair("F33", S);
      key_map[ EA::KEY_F34  ] = std::make_pair("F34", S);
      key_map[ EA::KEY_F35  ] = std::make_pair("F35", S);

      key_map[ KEY_AltGraph       ] = std::make_pair("AltGraph", S);
      key_map[ EA::KEY_Shift_L    ] = std::make_pair("Shift", L);
      key_map[ EA::KEY_Shift_R    ] = std::make_pair("Shift", R);
      key_map[ EA::KEY_Control_L  ] = std::make_pair("Control", L);
      key_map[ EA::KEY_Control_R  ] = std::make_pair("Control", R);
      key_map[ EA::KEY_Caps_Lock  ] = std::make_pair("CapsLock", S);
      key_map[ EA::KEY_Shift_Lock ] = std::make_pair("ShiftLock", S);
      key_map[ EA::KEY_Meta_L     ] = std::make_pair("Meta", L);
      key_map[ EA::KEY_Meta_R     ] = std::make_pair("Meta", R);
      key_map[ EA::KEY_Alt_L      ] = std::make_pair("Alt", L);
      key_map[ EA::KEY_Alt_R      ] = std::make_pair("Alt", R);
      key_map[ EA::KEY_Super_L    ] = std::make_pair("Super", L);
      key_map[ EA::KEY_Super_R    ] = std::make_pair("Super", R);
      key_map[ EA::KEY_Hyper_L    ] = std::make_pair("Hyper", L);
      key_map[ EA::KEY_Hyper_R    ] = std::make_pair("Hyper", R);

      num_pad_keys.insert(EA::KEY_KP_Home     );
      num_pad_keys.insert(EA::KEY_KP_Left     );
      num_pad_keys.insert(EA::KEY_KP_Up       );
      num_pad_keys.insert(EA::KEY_KP_Right    );
      num_pad_keys.insert(EA::KEY_KP_Down     );
      num_pad_keys.insert(EA::KEY_KP_Page_Up  );
      num_pad_keys.insert(EA::KEY_KP_Page_Down);
      num_pad_keys.insert(EA::KEY_KP_End      );
      num_pad_keys.insert(EA::KEY_KP_Begin    );
      num_pad_keys.insert(EA::KEY_KP_Insert   );
      num_pad_keys.insert(EA::KEY_KP_Delete   );
    }

    _location = DOM_KEY_LOCATION_STANDARD;

    InternalKeyMap::const_iterator it = key_map.find(_key);
    if( it != key_map.end())
    {
      _name = it->second.first;
      _location = it->second.second;
    }

    // Empty or no mapping -> convert UTF-32 key value to UTF-8
    if( _name.empty() )
    {
      if (( _key >= 0xd800u && _key <= 0xdfffu ) || _key > 0x10ffffu )
        _name = "Unidentified";
      else
        if ( _key <= 0x7f ) {
            _name.push_back(static_cast<uint8_t>(_key));
        } else if ( _key <= 0x7ff ) {
            _name.push_back(static_cast<uint8_t>((_key >> 6) | 0xc0));
            _name.push_back(static_cast<uint8_t>((_key & 0x3f) | 0x80));
        } else if ( _key <= 0xffff ) {
            _name.push_back(static_cast<uint8_t>((_key >> 12) | 0xe0));
            _name.push_back(static_cast<uint8_t>(((_key >> 6) & 0x3f) | 0x80));
            _name.push_back(static_cast<uint8_t>((_key & 0x3f) | 0x80));
        } else {
            _name.push_back(static_cast<uint8_t>((_key >> 18) | 0xf0));
            _name.push_back(static_cast<uint8_t>(((_key >> 12) & 0x3f) | 0x80));
            _name.push_back(static_cast<uint8_t>(((_key >> 6) & 0x3f) | 0x80));
            _name.push_back(static_cast<uint8_t>((_key & 0x3f) | 0x80));
        }
    }

    // Keys on the numpad with NumLock enabled are reported just like their
    // equivalent keys in the standard key block. Using the unmodified key value
    // we can detect such keys and set the location accordingly.
    if( num_pad_keys.find(_unmodified_key) != num_pad_keys.end() )
      _location = DOM_KEY_LOCATION_NUMPAD;

    return _name;
  }

  //----------------------------------------------------------------------------
  KeyboardEvent::DOMKeyLocation KeyboardEvent::location() const
  {
    key(); // ensure location is up-to-date
    return static_cast<DOMKeyLocation>(_location);
  }

  //----------------------------------------------------------------------------
  bool KeyboardEvent::isModifier() const
  {
    return (  _key >= EA::KEY_Shift_L
           && _key <= EA::KEY_Hyper_R
           )
        || _key == KEY_AltGraph;
  }

  //----------------------------------------------------------------------------
  bool KeyboardEvent::isPrint() const
  {
    const std::string& key_name = key();
    if( key_name.empty() )
      return false;

    // Convert the key name to the corresponding code point by checking the
    // sequence length (the first bits of the first byte) and performing the
    // conversion accordingly.
    uint32_t cp = key_name[0] & 0xff;
    size_t len;
    if (cp < 0x80) {
      len = 1;
    } else if ((cp >> 5) == 0x6) {
      cp = ((cp << 6) & 0x7ff) + (key_name[1] & 0x3f);
      len = 2;
    } else if ((cp >> 4) == 0xe) {
      cp = ((cp << 12) & 0xffff) + (((key_name[1] & 0xff) << 6) & 0xfff)
        + (key_name[2] & 0x3f);
      len = 3;
    } else if ((cp >> 3) == 0x1e) {
      cp = ((cp << 18) & 0x1fffff) + (((key_name[1] & 0xff) << 12) & 0x3ffff)
        + (((key_name[2] & 0xff) << 6) & 0xfff) + (key_name[3] & 0x3f);
      len = 4;
    } else {
      return false;
    }

    // Check if _name contains exactly one (UTF-8 encoded) character.
    if (key_name.length() > len)
      return false;

    // C0 and C1 control characters are not printable.
    if( cp <= 0x1f || (0x7f <= cp && cp <= 0x9f) )
      return false;

    return true;
  }

} // namespace canvas
} // namespace simgear
