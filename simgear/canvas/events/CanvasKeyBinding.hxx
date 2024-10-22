#pragma once

// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 James Turner <james@flightgear.org>

#include <vector>

#include <simgear/canvas/CanvasEvent.hxx>
#include <simgear/misc/simgear_optional.hxx>
#include <simgear/structure/SGBinding.hxx>

namespace simgear::canvas {

class KeyBinding : public SGReferenced
{
public:
    void setKey(const std::string& k);
    void setKeyCode(uint32_t code);

    void setModifiers(int modifiers);
    void setEventType(Event::Type ty);

    uint32_t keyCode() const;

    std::string key() const
    {
        return _key;
    }

    int modifiers() const
    {
        return _modifiers;
    }

    bool apply(const KeyboardEvent* ev) const;

    void addBinding(SGAbstractBinding_ptr b);

private:
    bool allModifiersMatch(const KeyboardEvent* ev) const;

    // check if one modifier (ctrl, shift, etc) matches in this binding
    // this allows for either left or right version of a modifier to be
    // pressed with the same result
    bool modifierMatch(const uint32_t evMods, uint32_t mask) const;

    std::string _key;
    simgear::optional<uint32_t> _keyCode;
    int _modifiers = 0;
    Event::Type _eventType = Event::KEY_PRESS;
    SGBindingList _bindings;
};

using KeyBindingRef = SGSharedPtr<KeyBinding>;

class FocusScope
{
public:
    void addKeyBinding(KeyBindingRef keyRef);

    bool handleEvent(const EventPtr& event);
    bool handleKeyboardEvent(const KeyboardEventPtr ev);

    bool empty() const;

private:
    std::vector<KeyBindingRef> _keys;
};

} // namespace simgear::canvas