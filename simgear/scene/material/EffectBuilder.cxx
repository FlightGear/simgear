#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

#include "EffectBuilder.hxx"
#include "Effect.hxx"

using std::string;

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

string getGlobalProperty(const SGPropertyNode* prop,
                         const SGReaderWriterOptions* options)
{
    if (!prop)
        return string();
    const SGPropertyNode* useProp = prop->getChild("use");
    if (!useProp)
        return string();
    string propName = useProp->getStringValue();
    SGPropertyNode_ptr propRoot;
    if (propName[0] == '/') {
        return propName;
    } else if ((propRoot = options->getPropertyNode())) {
        string result = propRoot->getPath();
        result.append("/");
        result.append(propName);
        return result;
    } else {
        throw effect::
            BuilderException("No property root to use with relative name "
                             + propName);
    }
        
    return useProp->getStringValue();
}

namespace effect
{
BuilderException::BuilderException()
{
}

BuilderException::BuilderException(const char* message, const char* origin)
    : sg_exception(message, origin, {}, false)
{
}

BuilderException::BuilderException(const std::string& message,
                                   const std::string& origin)
    : sg_exception(message, origin, {}, false)
{
}

BuilderException::~BuilderException()
{

}
}

bool isAttributeActive(Effect* effect, const SGPropertyNode* prop)
{
    const SGPropertyNode* activeProp
        = getEffectPropertyChild(effect, prop, "active");
    return !activeProp || activeProp->getValue<bool>();
}

namespace effect
{
const char* colorFields[] = {"red", "green", "blue", "alpha"};
}
  
PassAttributeBuilder::~PassAttributeBuilder()
{
}
  
} // of namespace simgear





