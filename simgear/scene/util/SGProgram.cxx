////
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
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA  02110-1301, USA.


#include "simgear_config.h"

#include "SGProgram.hxx"

#include <osg/State>
#include <sstream>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/logstream.hxx>

SGProgram::SGProgram()
{
}

SGProgram::SGProgram(const SGProgram& rhs, const osg::CopyOp& copyop) : osg::Program(rhs, copyop),
                                                                        _effectFilePath(rhs._effectFilePath)
{
}

void SGProgram::setEffectFilePath(const SGPath& p)
{
    _effectFilePath = p;
}

void SGProgram::apply(osg::State& state) const
{
    osg::Program::apply(state);

    if (_checkState != NotApplied) {
        return; // already done
    }

    auto pcp = state.getLastAppliedProgramObject();
    if (!pcp) {
        std::string infoLog;
        _checkState = FailedToApply;
        getPCP(state)->getInfoLog(infoLog);
        if (!infoLog.empty()) {
            // log all the shader source file names, to help in debugging link errors
            std::ostringstream os;
            for (int i = 0; i < getNumShaders(); ++i) {
                const auto shader = getShader(i);
                os << "\t" << shader->getFileName() << "\n";
            }

            simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::LoadEffectsShaders,
                                   "Shader program errors: " + infoLog +
                                       "\n\nShader sources:\n" + os.str(),
                                   _effectFilePath);
        }

        for (int i = 0; i < getNumShaders(); ++i) {
            const auto shader = getShader(i);
            auto pcs = shader->getPCS(state);

            std::string shaderLog;
            pcs->getInfoLog(shaderLog);
            if (!shaderLog.empty()) {
                simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::LoadEffectsShaders,
                                       "Shader source errors: " + shaderLog, sg_location(shader->getFileName()));
            }
        }
    } else {
        _checkState = AppliedOk;
    }
}
