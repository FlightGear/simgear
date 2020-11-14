#pragma once

#include "CanvasElement.hxx"

#include <memory>


struct SviewView;

namespace simgear
{

namespace canvas
{

struct View : simgear::canvas::Element
{
    /* See flightgear/src/Viewer/sview.hxx:SviewCreate(). */
    View(
            const CanvasWeakPtr& canvas,
            const SGPropertyNode_ptr& node,
            const Style& parent_style,
            ElementWeakPtr parent
            );
    
    std::shared_ptr<SviewView>  m_sview;
    
    
    typedef std::shared_ptr<SviewView> (*sview_factory_t)(
                const std::string& type,
                osg::ref_ptr<osg::GraphicsContext> gc,
                osg::ref_ptr<osg::Texture2D> texture
                );
    
    static void register_sview_factory(sview_factory_t fn);
    static sview_factory_t s_sview_factory; 
};


}

}
