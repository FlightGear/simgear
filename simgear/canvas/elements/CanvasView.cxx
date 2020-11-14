#include "CanvasView.hxx"

struct SviewView;

namespace simgear
{

namespace canvas
{

void View::staticInit()
{
    if (isInit<View>()) {
        return;
    }
    // Do some initialization if needed...
}

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
    SG_LOG(SG_GENERAL, SG_ALERT, "Creating canvas view...");
    int width = node->getIntValue("width", 100);
    int height = node->getIntValue("height", 100);
    osg::GraphicsContext::Traits*   gc_traits = new osg::GraphicsContext::Traits;
    gc_traits->x = 0;
    gc_traits->y = 0;
    gc_traits->width    = width;
    gc_traits->height   = height;
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
    
    /* Maybe m_texture doesn't need to be a member - will be owned by m_sview's
    compositor. */
    m_texture = new osg::Texture2D;
    
    m_texture->setTextureWidth(width);
    m_texture->setTextureHeight(height);
    m_texture->setDataVariance(osg::Object::DYNAMIC);    
    m_texture->setInternalFormat(GL_RGBA);
    m_texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    m_texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    
    m_sview = s_sview_factory("current", gc, m_texture);
    
    m_quad = osg::createTexturedQuadGeometry(osg::Vec3(0.0,    0.0,     0.0),
                                            osg::Vec3(width, 0.0,     0.0),
                                            osg::Vec3(0.0,    height, 0.0),
                                            // In Canvas (0,0) is the top left corner, while in OSG/OpenGL
                                            // it's the bottom left corner.
                                            0.0, 1.0, 1.0, 0.0);
    m_quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, m_texture.get(), osg::StateAttribute::ON);

    setDrawable(m_quad.get());
}


View::sview_factory_t   View::s_sview_factory = nullptr;
const std::string       View::TYPE_NAME = "view";

void View::register_sview_factory(sview_factory_t fn)
{
    s_sview_factory = fn;
}


}

}
