#include "CanvasView.hxx"

struct SviewView;

namespace simgear
{

namespace canvas
{

View::View(
            const CanvasWeakPtr& canvas,
            const SGPropertyNode_ptr& node,
            const Style& parent_style,
            ElementWeakPtr parent
            )
:
Element(canvas, node, parent_style, parent)
{
    assert(s_sview_factory);
    
    osg::GraphicsContext::Traits*   gc_traits = new osg::GraphicsContext::Traits;
    gc_traits->x = 0;
    gc_traits->y = 0;
    gc_traits->width    = node->getDoubleValue("width");
    gc_traits->height   = node->getDoubleValue("height");
    gc_traits->windowDecoration = false;
    gc_traits->doubleBuffer = true;
    gc_traits->sharedContext = 0;

    int bpp = node->getIntValue("bits-per-pixel", 24);
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 24;
    gc_traits->red = cbits;
    gc_traits->red = cbits;
    gc_traits->red = cbits;
    gc_traits->depth = zbits;
    
    gc_traits->mipMapGeneration = true;
    //gc_traits->windowName = "Flightgear";
    gc_traits->sampleBuffers = node->getIntValue("multi-sample-buffers", 1);
    gc_traits->samples = node->getIntValue("multi-samples", 2);
    gc_traits->vsync = node->getBoolValue("vsync-enable", false);
    gc_traits->stencil = 8;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(gc_traits);
    assert(gc.valid());
    
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setTextureWidth(gc_traits->width);
    texture->setTextureHeight(gc_traits->height);
    
    m_sview = s_sview_factory("current", gc, texture);
}

View::sview_factory_t View::s_sview_factory = nullptr;

void View::register_sview_factory(sview_factory_t fn)
{
    s_sview_factory = fn;
}


}

}
