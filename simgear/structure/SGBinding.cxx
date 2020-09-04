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

#include <simgear/compiler.h>
#include "SGBinding.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

SGBinding::SGBinding()
    : _arg(new SGPropertyNode)
{
}

SGBinding::SGBinding(const std::string& commandName)
{
    _command_name = commandName;
}

SGBinding::SGBinding(const SGPropertyNode* node, SGPropertyNode* root)
{
  read(node, root);
}

void
SGBinding::clear()
{
    _arg.clear();
    _root.clear();
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
      SG_LOG(SG_INPUT, SG_DEV_ALERT, "No command supplied for binding.");
  }

  _arg = const_cast<SGPropertyNode*>(node);
  _root = const_cast<SGPropertyNode*>(root);
  _setting.clear();
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
    auto cmd = SGCommandMgr::instance()->getCommand(_command_name);
    if (!cmd) {
        SG_LOG(SG_INPUT, SG_WARN, "No command found for binding:" << _command_name);
        return;
    }

    try {
        if (!(*cmd)(_arg, _root)) {
            SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command " << _command_name);
        }
    } catch (sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_ALERT, "command '" << _command_name << "' failed with exception\n"
                                                 << "\tmessage:" << e.getMessage() << " (from " << e.getOrigin() << ")");
    }
}

void
SGBinding::fire (SGPropertyNode* params) const
{
  if (test()) {
      if (params != nullptr) {
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
                                if (!_setting) { // save the setting node for efficiency
                                    _setting = _arg->getChild("setting", 0, true);
                                }
    _setting->setDoubleValue(setting);
    innerFire();
  }
}

void fireBindingList(const SGBindingList& aBindings, SGPropertyNode* params)
{
    for (auto b : aBindings) {
        b->fire(params);
    }
}

void fireBindingListWithOffset(const SGBindingList& aBindings, double offset, double max)
{
    for (auto b : aBindings) {
        b->fire(offset, max);
    }
}

SGBindingList readBindingList(const simgear::PropertyList& aNodes, SGPropertyNode* aRoot)
{
    SGBindingList result;
    for (auto node : aNodes) {
        result.push_back(new SGBinding(node, aRoot));
    }

    return result;
}

void clearBindingList(const SGBindingList& aBindings)
{
    for (auto b : aBindings) {
        b->clear();
    }
}

bool anyBindingEnabled(const SGBindingList& aBindings)
{
    if (aBindings.empty()) {
        return false;
    }

    for (auto b : aBindings) {
        if (b->test()) {
            return true;
        }
    }

    return false;
}
