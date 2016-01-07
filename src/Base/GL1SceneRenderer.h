/*!
  @file
  @author Shin'ichiro Nakaoka
*/

#ifndef CNOID_BASE_GL1_SCENE_RENDERER_H
#define CNOID_BASE_GL1_SCENE_RENDERER_H

#include <cnoid/GLSceneRenderer>
#include <boost/function.hpp>
#include "exportdecl.h"

namespace cnoid {

class GL1SceneRendererImpl;
class SgCustomGLNode;
class Mapping;
    
class CNOID_EXPORT GL1SceneRenderer : public GLSceneRenderer
{
public:
    GL1SceneRenderer();
    GL1SceneRenderer(SgGroup* root);
    virtual ~GL1SceneRenderer();

    virtual const Affine3& currentModelTransform() const;
    virtual const Matrix4& projectionMatrix() const;
        
    bool initializeGL();

    // The following functions cannot be called bofore calling the initializeGL() function.
    bool setSwapInterval(int interval);
    int getSwapInterval() const;

    /**
       This function does the same things as beginRendering() except that
       actual GL commands are not executed.
       This should only be called when you want to initialize
       the rendering without doing any GL rendering commands.
       For example, you can obtain cameras without rendering, and you can render the scene
       after selecting the current camera.
    */
    virtual void initializeRendering();
        
    virtual void beginRendering();
    virtual void endRendering();
    virtual void render();

    bool pick(int x, int y);
    const Vector3& pickedPoint() const;
    const SgNodePath& pickedNodePath() const;

    void setDefaultLighting(bool on);
    void setHeadLightLightingFromBackEnabled(bool on);
    void setDefaultSmoothShading(bool on);
    virtual SgMaterial* defaultMaterial();
    void enableTexture(bool on);
    void setDefaultPointSize(double size);
    void setDefaultLineWidth(double width);

    void setNewDisplayListDoubleRenderingEnabled(bool on);

    void showNormalVectors(double length);

    void requestToClearCache();

    /**
       If this is enabled, OpenGL resources such as display lists, vertex buffer objects
       are checked if they are still used or not, and the unused resources are released
       when finalizeRendering() is called. The default value is true.
    */
    virtual void enableUnusedCacheCheck(bool on);

    virtual void visitGroup(SgGroup* group);
    virtual void visitInvariantGroup(SgInvariantGroup* group);
    virtual void visitTransform(SgTransform* transform);
    virtual void visitUnpickableGroup(SgUnpickableGroup* group);
    virtual void visitShape(SgShape* shape);
    virtual void visitPointSet(SgPointSet* pointSet);        
    virtual void visitLineSet(SgLineSet* lineSet);        
    virtual void visitPreprocessed(SgPreprocessed* preprocessed);
    virtual void visitLight(SgLight* light);
    virtual void visitOverlay(SgOverlay* overlay);
    virtual void visitOutlineGroup(SgOutlineGroup* outline);

    bool isPicking();

    void setColor(const Vector4f& color);
    void enableColorMaterial(bool on);
    void setDiffuseColor(const Vector4f& color);
    void setAmbientColor(const Vector4f& color);
    void setEmissionColor(const Vector4f& color);
    void setSpecularColor(const Vector4f& color);
    void setShininess(float shininess);
    void enableCullFace(bool on);
    void setFrontCCW(bool on);
    void enableLighting(bool on);
    void setLightModelTwoSide(bool on);
    void enableBlend(bool on);
    void enableDepthMask(bool on);
    void setPointSize(float size);
    void setLineWidth(float width);

  protected:
    virtual void onImageUpdated(SgImage* image);    
        
private:
    GL1SceneRendererImpl* impl;
    friend class GL1SceneRendererImpl;
    friend class SgCustomGLNode;
};

class CNOID_EXPORT SgCustomGLNode : public SgGroup
{
public:
    typedef boost::function<void(GL1SceneRenderer& renderer)> RenderingFunction;

    SgCustomGLNode() { }
    SgCustomGLNode(RenderingFunction f) : renderingFunction(f) { }
    virtual SgObject* clone(SgCloneMap& cloneMap) const;
    virtual void accept(SceneVisitor& visitor);
    virtual void render(GL1SceneRenderer& renderer);
    void setRenderingFunction(RenderingFunction f);

protected:
    SgCustomGLNode(const SgCustomGLNode& org, SgCloneMap& cloneMap) : SgGroup(org, cloneMap) { }

private:
    RenderingFunction renderingFunction;
};
typedef ref_ptr<SgCustomGLNode> SgCustomGLNodePtr;

}

#endif
