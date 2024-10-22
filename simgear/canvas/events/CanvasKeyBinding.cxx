// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 James Turner <james@flightgear.org>

#include "CanvasKeyBinding.hxx"

// OSG
#include <osgGA/GUIEventAdapter>


#include <simgear/canvas/events/KeyboardEvent.hxx>

#include <simgear/structure/exception.hxx>

namespace simgear::canvas {

void KeyBinding::setKey(const std::string& k)
{
    _key = k;
}

void KeyBinding::setModifiers(int modifiers)
{
    _modifiers = modifiers;
}

void KeyBinding::setKeyCode(uint32_t code)
{
    _keyCode = code;
}


void KeyBinding::setEventType(Event::Type ty)
{
    if ((ty != Event::KEY_UP) && (ty != Event::KEY_DOWN) && (ty != Event::KEY_PRESS)) {
        throw sg_exception("Invalid event type set for key binding");
    }

    _eventType = ty;
}

uint32_t KeyBinding::keyCode() const
{
    return _keyCode.has_value() ? _keyCode.value() : 0;
}

bool KeyBinding::apply(const KeyboardEvent* ev) const
{
    // check type and modifiers
    if ((ev->getType() != _eventType) || !allModifiersMatch(ev)) {
        return false;
    }

    // key-code based matching
    if (_keyCode.has_value() && (_keyCode.value() == ev->keyCode())) {
        fireBindingList(_bindings);
        return true;
    } else if (ev->key() == _key) {
        // string-based matching
        fireBindingList(_bindings);
        return true;
    }

    return false;
}

// note this overlaps with FGEventHandler::translateModifiers,
// but sharing with that would be a pain.
bool KeyBinding::allModifiersMatch(const KeyboardEvent* ev) const
{
    const uint32_t eventModifiers = ev->getModifiers();
    if (!modifierMatch(eventModifiers, osgGA::GUIEventAdapter::MODKEY_CTRL)) return false;
    if (!modifierMatch(eventModifiers, osgGA::GUIEventAdapter::MODKEY_ALT)) return false;
    if (!modifierMatch(eventModifiers, osgGA::GUIEventAdapter::MODKEY_SHIFT)) return false;

    return modifierMatch(eventModifiers, osgGA::GUIEventAdapter::MODKEY_META);
}

bool KeyBinding::modifierMatch(const uint32_t evMods, uint32_t mask) const
{
    const bool modIsPressed = (evMods & mask);
    const bool modIsNeeded = _modifiers & mask;
    return modIsPressed == modIsNeeded;
}

void KeyBinding::addBinding(SGAbstractBinding_ptr b)
{
    _bindings.push_back(b);
}

bool FocusScope::handleEvent(const EventPtr& event)
{
    if (event->isPropagationStopped()) {
        return false;
    }

    auto key_event = dynamic_pointer_cast<KeyboardEvent>(event);
    if (key_event) {
        return handleKeyboardEvent(key_event);
    }

    return false;
}

void FocusScope::addKeyBinding(KeyBindingRef keyRef)
{
    _keys.push_back(keyRef);
}

bool FocusScope::handleKeyboardEvent(const KeyboardEventPtr ev)
{
    for (const auto& kb : _keys) {
        const auto b = kb->apply(ev);
        if (b) {
            // mark the event as handled, don't bubble up further
            ev->stopPropagation();

            // early return here implies only the first registered binding on this scope
            // will fire, i.e you can't bind the same key multiple times.

            return true;
        }
    }

    return false;
}

bool FocusScope::empty() const
{
    return _keys.empty();
}

} // namespace simgear::canvas
