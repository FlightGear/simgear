/**
 * \file commands.hxx
 * Interface definition for encapsulated commands.
 * Started Spring 2001 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <boost/foreach.hpp>
#include <simgear/compiler.h>
#include "SGBinding.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

SGBinding::SGBinding()
  : _command(0),
    _arg(new SGPropertyNode),
    _setting(0)
{
}

SGBinding::SGBinding(const std::string& commandName)
    : _command(0),
    _arg(0),
    _setting(0)
{
    _command_name = commandName;
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
SGBinding::clear()
{
    _command = NULL;
    _arg.clear();
    _setting.clear();
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
  }

  _arg = const_cast<SGPropertyNode*>(node);
  _setting = 0;
}

void
SGBinding::fire() const
{
  if (test()) {
    innerFire();
  }
}

void
SGBinding::innerFire () const
{
  if (_command == 0)
    _command = SGCommandMgr::instance()->getCommand(_command_name);
  if (_command == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No command attached to binding:" << _command_name);
  } else {
      try {
          if (!(*_command)(_arg)) {
                SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command "
                       << _command_name);
          }
      } catch (sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_ALERT, "command '" << _command_name << "' failed with exception\n"
          << "\tmessage:" << e.getMessage() << " (from " << e.getOrigin() << ")");
      }
  }
}

void
SGBinding::fire (SGPropertyNode* params) const
{
  if (test()) {
    if (params != NULL) {
      copyProperties(params, _arg);
    }
    
    innerFire();
  }
}

void
SGBinding::fire (double offset, double max) const
{
  if (test()) {
    _arg->setDoubleValue("offset", offset/max);
    innerFire();
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
    innerFire();
  }
}

void fireBindingList(const SGBindingList& aBindings, SGPropertyNode* params)
{
    BOOST_FOREACH(SGBinding_ptr b, aBindings) {
        b->fire(params);
    }
}

void fireBindingListWithOffset(const SGBindingList& aBindings, double offset, double max)
{
    BOOST_FOREACH(SGBinding_ptr b, aBindings) {
        b->fire(offset, max);
    }
}

SGBindingList readBindingList(const simgear::PropertyList& aNodes, SGPropertyNode* aRoot)
{
    SGBindingList result;
    BOOST_FOREACH(SGPropertyNode* node, aNodes) {
        result.push_back(new SGBinding(node, aRoot));
    }
    
    return result;
}

void clearBindingList(const SGBindingList& aBindings)
{
    BOOST_FOREACH(SGBinding_ptr b, aBindings) {
        b->clear();
    }
}

