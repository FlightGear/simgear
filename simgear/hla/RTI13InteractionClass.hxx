// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef RTI13InteractionClass_hxx
#define RTI13InteractionClass_hxx

#include <map>
#include <vector>

#ifndef RTI_USES_STD_FSTREAM
#define RTI_USES_STD_FSTREAM
#endif

#include <RTI.hh>

#include "RTIInteractionClass.hxx"

#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear {

class RTI13Ambassador;

class RTI13InteractionClass : public RTIInteractionClass {
public:
    RTI13InteractionClass(HLAInteractionClass* interactionClass, const RTI::InteractionClassHandle& handle, RTI13Ambassador* ambassador);
    virtual ~RTI13InteractionClass();

    virtual bool resolveParameterIndex(const std::string& name, unsigned index);

    virtual bool publish();
    virtual bool unpublish();

    virtual bool subscribe(bool);
    virtual bool unsubscribe();

private:
    RTI::InteractionClassHandle _handle;
    SGSharedPtr<RTI13Ambassador> _ambassador;

    typedef std::map<RTI::ParameterHandle, unsigned> ParameterHandleIndexMap;
    ParameterHandleIndexMap _parameterHandleIndexMap;

    typedef std::vector<RTI::ParameterHandle> ParameterHandleVector;
    ParameterHandleVector _parameterHandleVector;
};

}

#endif
