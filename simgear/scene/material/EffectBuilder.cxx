#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/math/SGMath.hxx>

#include "EffectBuilder.hxx"
#include "Effect.hxx"

namespace simgear
{

// Given a property node from a pass, get its value either from it or
// from the effect parameters.
const SGPropertyNode* getEffectPropertyNode(Effect* effect,
                                            const SGPropertyNode* prop)
{
    if (!prop)
        return 0;
    if (prop->nChildren() > 0) {
        const SGPropertyNode* useProp = prop->getChild("use");
        if (!useProp || !effect->parametersProp)
            return prop;
        return effect->parametersProp->getNode(useProp->getStringValue());
    }
    return prop;
}

// Get a named child property from pass parameters or effect
// parameters.
const SGPropertyNode* getEffectPropertyChild(Effect* effect,
                                             const SGPropertyNode* prop,
                                             const char* name)
{
    const SGPropertyNode* child = prop->getChild(name);
    if (!child)
        return 0;
    else
        return getEffectPropertyNode(effect, child);
}

BuilderException::BuilderException()
{
}

BuilderException::BuilderException(const char* message, const char* origin)
  : sg_exception(message, origin)
{
}

BuilderException::BuilderException(const std::string& message,
                                   const std::string& origin)
  : sg_exception(message, origin)
{
}

BuilderException::~BuilderException() throw()
{
}
}
