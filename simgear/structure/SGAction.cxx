

#if 0

set bindings for action seperate from defintion ?
    - in XML, especially aircraft XML

define actions from command / Nasal
add behaviours from Nasal 

define keymapping
    - manager of keybindings defined against actions?
    - for a toggle or enum, define behaviour
        - each key repeat cycles
        - alternate key to go the other way (G/shift-G)

release bindings for momentary actions:
    button up / key-up

send activate, release

    send deactivate, release for 'alternate' action

#endif


void SGAction::setValueExpression()
{
    // watch all the properties
}

void SGAction::setValueCondition()
{
    // 
}

void SGAction::updateProperties()
{
    //
    _node->setBoolValue("enabled", isEnabled());
    switch (_type) {
    case Momentary:
    case Toggle:
        _node->setBoolValue("value", getValue());
        break;

    case Enumerated: 
        if (!_valueEnumeration.empty()) {
            // map to the string value
            _node->setStringValue("value", _valueEnumeration.at(getValue()));
        } else {
            // set as an integer
            _node->setIntValue("value", getValue());
        }
    }

    // set description
}

bool SGAction::isEnabled()
{
    if (_enableCondition) {

    } else {
        return _enabled;
    }

    updateProperties();
}

int SGAction::getValue()
{
    if (type == Enumerated) {
        if (_valueExpression) {
            // invoke it
        }
    } else {
        if (_valueCondition) {
            return _valueCondition.test();
        }
    }

    return _value;
}

// commands enable-action, disable-action
