/**
 * \file commands.hxx
 * Interface definition for encapsulated commands.
 * Started Spring 2001 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * $Id$
 */

#include <simgear/compiler.h>
#include "SGBinding.hxx"

SGBinding::SGBinding()
  : _command(0),
    _arg(new SGPropertyNode),
    _setting(0)
{
}

SGBinding::SGBinding(const SGPropertyNode* node, SGPropertyNode* root)
  : _command(0),
    _arg(0),
    _setting(0)
{
  read(node, root);
}

SGBinding::~SGBinding()
{
  if(_arg && _arg->getParent())
    _arg->getParent()->removeChild(_arg->getName(), _arg->getIndex(), false);
}

void
SGBinding::read(const SGPropertyNode* node, SGPropertyNode* root)
{
  const SGPropertyNode * conditionNode = node->getChild("condition");
  if (conditionNode != 0)
    setCondition(sgReadCondition(root, conditionNode));

  _command_name = node->getStringValue("command", "");
  if (_command_name.empty()) {
    SG_LOG(SG_INPUT, SG_WARN, "No command supplied for binding.");
    _command = 0;
    return;
  }

  _arg = const_cast<SGPropertyNode*>(node);
  _setting = 0;
}

void
SGBinding::fire () const
{
  if (test()) {
    if (_command == 0)
      _command = SGCommandMgr::instance()->getCommand(_command_name);
    if (_command == 0) {
      SG_LOG(SG_INPUT, SG_WARN, "No command attached to binding");
    } else if (!(*_command)(_arg)) {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command "
             << _command_name);
    }
  }
}

void
SGBinding::fire (double offset, double max) const
{
  if (test()) {
    _arg->setDoubleValue("offset", offset/max);
    fire();
  }
}

void
SGBinding::fire (double setting) const
{
  if (test()) {
                                // A value is automatically added to
                                // the args
    if (_setting == 0)          // save the setting node for efficiency
      _setting = _arg->getChild("setting", 0, true);
    _setting->setDoubleValue(setting);
    fire();
  }
}
