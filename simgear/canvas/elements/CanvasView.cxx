#include "CanvasView.hxx"

#include <simgear/canvas/Canvas.hxx>

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
    
    CanvasPtr canvas2 = canvas.lock();
    int width = canvas2->getViewWidth();
    int height = canvas2->getViewHeight();
    
    // these values seem to be +/- infinity.
    ElementPtr parent2 = parent.lock();
    osg::BoundingBox    bb = parent2->getBoundingBox();
    osg::BoundingBox    bbtight = parent2->getTightBoundingBox();
    SG_LOG(SG_GENERAL, SG_ALERT, "bb:     "
            << ' ' << bb.xMin()
            << ' ' << bb.yMin()
            << ' ' << bb.zMin()
            << ' ' << bb.xMax()
            << ' ' << bb.yMax()
            << ' ' << bb.zMax()
            );
    SG_LOG(SG_GENERAL, SG_ALERT, "bbtight:"
            << ' ' << bbtight.xMin()
            << ' ' << bbtight.yMin()
            << ' ' << bbtight.zMin()
            << ' ' << bbtight.xMax()
            << ' ' << bbtight.yMax()
            << ' ' << bbtight.zMax()
            );

    /* Maybe m_texture doesn't need to be a member - will be owned by m_sview's
    compositor. */
    m_texture = new osg::Texture2D;
    
    SG_LOG(SG_GENERAL, SG_ALERT, "" << " width=" << width << " height=" << height);
    m_texture->setTextureWidth(width);
    m_texture->setTextureHeight(height);
    m_texture->setDataVariance(osg::Object::DYNAMIC);    
    m_texture->setInternalFormat(GL_RGBA);
    m_texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    m_texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    
    m_sview = s_sview_factory("current", m_texture);
    
    m_quad = osg::createTexturedQuadGeometry(osg::Vec3(0.0,    0.0,     0.0),
                                            osg::Vec3(width, 0.0,     0.0),
                                            osg::Vec3(0.0,    height, 0.0),
                                            // In Canvas (0,0) is the top left corner, while in OSG/OpenGL
                                            // it's the bottom left corner.
                                            0.0, 1.0, 1.0, 0.0);
    m_quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, m_texture.get(), osg::StateAttribute::ON);

    setDrawable(m_quad.get());
}

void View::update(double dt)
{
    auto c = getCanvas().lock();
    c->enableRendering(true); // force a repaint

    Element::update(dt);
}


View::sview_factory_t   View::s_sview_factory = nullptr;
const std::string       View::TYPE_NAME = "canvasview";

void View::register_sview_factory(sview_factory_t fn)
{
    s_sview_factory = fn;
}


}

}
