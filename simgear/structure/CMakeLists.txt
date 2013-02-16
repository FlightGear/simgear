
include (SimGearComponent)

set(HEADERS
    OSGUtils.hxx
    OSGVersion.hxx
    SGAtomic.hxx
    SGBinding.hxx
    SGExpression.hxx
    SGReferenced.hxx
    SGSharedPtr.hxx
    SGSmplhist.hxx
    SGSmplstat.hxx
    SGWeakPtr.hxx
    SGWeakReferenced.hxx
    SGPerfMon.hxx
    singleton.hpp
    Singleton.hxx
    StringTable.hxx
    callback.hxx
    commands.hxx
    event_mgr.hxx
    exception.hxx
    intern.hxx
    subsystem_mgr.hxx
    )

set(SOURCES
    SGAtomic.cxx
    SGBinding.cxx
    SGExpression.cxx
    SGSmplhist.cxx
    SGSmplstat.cxx
    SGPerfMon.cxx
    StringTable.cxx
    commands.cxx
    event_mgr.cxx
    exception.cxx
    subsystem_mgr.cxx
    )

simgear_component(structure structure "${SOURCES}" "${HEADERS}")
