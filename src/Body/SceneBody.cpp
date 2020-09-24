/**
   @author Shin'ichiro Nakaoka
*/

#include "SceneBody.h"
#include <cnoid/SceneNodeClassRegistry>
#include <cnoid/SceneDrawables>
#include <cnoid/SceneEffects>
#include <cnoid/SceneUtil>
#include <cnoid/SceneRenderer>
#include <cnoid/CloneMap>
#include "gettext.h"

using namespace std;
using namespace cnoid;

namespace {

class LinkShapeGroup : public SgGroup
{
public:
    SgNodePtr visualShape;
    SgNodePtr collisionShape;
    ScopedConnection collisionShapeUpdateConnection;
    bool isVisible;
    bool hasClone;
    
    LinkShapeGroup(Link* link)
        : SgGroup(findClassId<LinkShapeGroup>())
    {
        visualShape = link->visualShape();
        if(visualShape){
            addChild(visualShape);
        }
        collisionShape = link->collisionShape();
        resetCollisionShapeUpdateConnection();

        isVisible = true;
        hasClone = false;
    }

    void setVisible(bool on)
    {
        isVisible = on;
    }

    void cloneShapes(CloneMap& cloneMap)
    {
        if(!hasClone){
            bool sameness = (visualShape == collisionShape);
            if(visualShape){
                removeChild(visualShape);
                visualShape = visualShape->cloneNode(cloneMap);
                addChild(visualShape);
            }
            if(collisionShape){
                if(sameness){
                    collisionShape = visualShape;
                } else {
                    collisionShape = collisionShape->cloneNode(cloneMap);
                }
            }
            resetCollisionShapeUpdateConnection();
            hasClone = true;
            notifyUpdate(SgUpdate::REMOVED | SgUpdate::ADDED);
        }
    }

    void resetCollisionShapeUpdateConnection()
    {
        if(collisionShape && collisionShape != visualShape){
            collisionShapeUpdateConnection.reset(
                collisionShape->sigUpdated().connect(
                    [this](const SgUpdate& u){
                        SgUpdate update(u);
                        notifyUpdate(update);
                    }));
        } else {
            collisionShapeUpdateConnection.disconnect();
        }
    }

    void render(SceneRenderer* renderer)
    {
        renderer->renderCustomGroup(
            this, [=](){ traverse(renderer, renderer->renderingFunctions()); });
    }

    void traverse(SceneRenderer* renderer, SceneRenderer::NodeFunctionSet* functions)
    {
        int visibility = 0;
        if(isVisible){
            static const SceneRenderer::PropertyKey key("collisionDetectionModelVisibility");
            visibility = renderer->property(key, 1);
        }
        for(auto p = cbegin(); p != cend(); ++p){
            SgNode* node = *p;
            if(node == visualShape){
                if(!(visibility & 1)){
                    continue;
                }
            }
            functions->dispatch(node);
        }
        if((visibility & 2) && (collisionShape != visualShape) && collisionShape){
            functions->dispatch(collisionShape);
        }
    }
};

typedef ref_ptr<LinkShapeGroup> LinkShapeGroupPtr;

struct NodeClassRegistration {
    NodeClassRegistration(){
        SceneNodeClassRegistry::instance().registerClass<LinkShapeGroup, SgGroup>();
        SceneRenderer::addExtension(
            [](SceneRenderer* renderer){
                renderer->renderingFunctions()->setFunction<LinkShapeGroup>(
                    [renderer](LinkShapeGroup* node){ node->render(renderer); });
            });
    }
} registration;

}

namespace cnoid {

class SceneLinkImpl
{
public:
    SceneLink* self;
    LinkShapeGroupPtr mainShapeGroup;
    SgGroupPtr topShapeGroup;
    bool isVisible;
    std::vector<SceneDevicePtr> sceneDevices;
    SgGroupPtr deviceGroup;
    SgTransparentGroupPtr transparentGroup;

    SceneLinkImpl(SceneLink* self, Link* link);
    void insertEffectGroup(SgGroup* group, bool doNotify);
    bool removeEffectGroup(SgGroup* parent, SgGroupPtr effectGroup, bool doNotify);
    void cloneShape(CloneMap& cloneMap);
    void setTransparency(float transparency, bool doNotify);
};

class SceneBodyImpl
{
public:
    SceneBody* self;
    SgGroupPtr sceneLinkGroup;
    std::vector<SceneDevicePtr> sceneDevices;
    std::function<SceneLink*(Link*)> sceneLinkFactory;

    SceneBodyImpl(SceneBody* self, std::function<SceneLink*(Link*)> sceneLinkFactory);
    void cloneShape(CloneMap& cloneMap);
};

}


SceneLink::SceneLink(Link* link)
{
    link_ = link;
    setName(link->name());
    impl = new SceneLinkImpl(this, link);
}


SceneLinkImpl::SceneLinkImpl(SceneLink* self, Link* link)
    : self(self)
{
    mainShapeGroup = new LinkShapeGroup(link);
    topShapeGroup = mainShapeGroup;
    self->addChild(topShapeGroup);
}


SceneLink::SceneLink(const SceneLink& org)
    : SgPosTransform(org)
{

}


SceneLink::~SceneLink()
{
    delete impl;
}


const SgNode* SceneLink::visualShape() const
{
    return impl->mainShapeGroup->visualShape;
}


SgNode* SceneLink::visualShape()
{
    return impl->mainShapeGroup->visualShape;
}


const SgNode* SceneLink::collisionShape() const
{
    return impl->mainShapeGroup->collisionShape;
}


SgNode* SceneLink::collisionShape()
{
    return impl->mainShapeGroup->collisionShape;
}


void SceneLink::insertEffectGroup(SgGroup* group, bool doNotify)
{
    impl->insertEffectGroup(group, doNotify);
}


void SceneLinkImpl::insertEffectGroup(SgGroup* group, bool doNotify)
{
    self->removeChild(topShapeGroup);
    group->addChild(topShapeGroup);
    self->addChild(group);
    topShapeGroup = group;
    if(doNotify){
        self->notifyUpdate(SgUpdate::ADDED | SgUpdate::REMOVED);
    }
}


void SceneLink::removeEffectGroup(SgGroup* group, bool doNotify)
{
    impl->removeEffectGroup(this, group, doNotify);
}


bool SceneLinkImpl::removeEffectGroup(SgGroup* parent, SgGroupPtr effectGroup, bool doNotify)
{
    if(parent == mainShapeGroup){
        return false;
    }
    if(parent->removeChild(effectGroup)){
        SgGroup* childGroup = 0;
        for(auto child : *effectGroup){
            childGroup = dynamic_cast<SgGroup*>(child.get());
            if(childGroup){
                parent->addChild(childGroup);
                break;
            }
        }
        if(topShapeGroup == effectGroup){
            if(childGroup){
                topShapeGroup = childGroup;
            } else {
                topShapeGroup = mainShapeGroup;
            }
        }
        effectGroup->clearChildren();
        if(doNotify){
            parent->notifyUpdate(SgUpdate::ADDED | SgUpdate::REMOVED);
        }
        return true;
    } else {
        for(auto child : *parent){
            if(auto childGroup = dynamic_cast<SgGroup*>(child.get())){
                if(removeEffectGroup(childGroup, effectGroup, doNotify)){
                    return true;
                }
            }
        }
    }
    return false;
}


void SceneLinkImpl::cloneShape(CloneMap& cloneMap)
{
    mainShapeGroup->cloneShapes(cloneMap);
}


void SceneLink::setVisible(bool on)
{
    impl->mainShapeGroup->setVisible(on);
}


float SceneLink::transparency() const
{
    if(!impl->transparentGroup && impl->transparentGroup->hasParents()){
        return impl->transparentGroup->transparency();
    }
    return 0.0f;
}


void SceneLink::setTransparency(float transparency, bool doNotify)
{
    impl->setTransparency(transparency, doNotify);
}


void SceneLinkImpl::setTransparency(float transparency, bool doNotify)
{
    if(!transparentGroup){
        transparentGroup = new SgTransparentGroup;
        transparentGroup->setTransparency(transparency);
    } else if(transparency != transparentGroup->transparency()){
        transparentGroup->setTransparency(transparency);
        if(doNotify){
            transparentGroup->notifyUpdate();
        }
    }

    if(transparency > 0.0f){
        if(!transparentGroup->hasParents()){
            insertEffectGroup(transparentGroup, doNotify);
        }
    } else {
        if(transparentGroup->hasParents()){
            self->removeEffectGroup(transparentGroup, doNotify);
        }
    }
}


void SceneLink::makeTransparent(float transparency)
{
    setTransparency(transparency, true);
}


void SceneLink::addSceneDevice(SceneDevice* sdev)
{
    if(!impl->deviceGroup){
        impl->deviceGroup = new SgGroup();
        addChild(impl->deviceGroup);
    }
    impl->sceneDevices.push_back(sdev);
    impl->deviceGroup->addChild(sdev);
}


SceneDevice* SceneLink::getSceneDevice(Device* device)
{
    auto& devices = impl->sceneDevices;
    for(size_t i=0; i < devices.size(); ++i){
        SceneDevice* sdev = devices[i];
        if(sdev->device() == device){
            return sdev;
        }
    }
    return 0;
}


SceneBody::SceneBody(Body* body)
    : SceneBody(body, [](Link* link){ return new SceneLink(link); })
{

}


SceneBody::SceneBody(Body* body, std::function<SceneLink*(Link*)> sceneLinkFactory)
    : body_(body)
{
    impl = new SceneBodyImpl(this, sceneLinkFactory);
    addChild(impl->sceneLinkGroup);
    updateModel();
}


SceneBodyImpl::SceneBodyImpl(SceneBody* self, std::function<SceneLink*(Link*)> sceneLinkFactory)
    : self(self),
      sceneLinkFactory(sceneLinkFactory)
{
    sceneLinkGroup = new SgGroup;
}


SceneBody::SceneBody(const SceneBody& org)
    : SgPosTransform(org)
{

}


SceneBody::~SceneBody()
{
    delete impl;
}


void SceneBody::updateModel()
{
    setName(body_->name());

    if(sceneLinks_.empty()){
        impl->sceneLinkGroup->clearChildren();
        sceneLinks_.clear();
    }
    impl->sceneDevices.clear();
        
    const int n = body_->numLinks();
    for(int i=0; i < n; ++i){
        Link* link = body_->link(i);
        SceneLink* sLink = impl->sceneLinkFactory(link);
        impl->sceneLinkGroup->addChild(sLink);
        sceneLinks_.push_back(sLink);
    }

    const DeviceList<Device>& devices = body_->devices();
    for(size_t i=0; i < devices.size(); ++i){
        Device* device = devices[i];
        SceneDevice* sceneDevice = SceneDevice::create(device);
        if(sceneDevice){
            sceneLinks_[device->link()->index()]->addSceneDevice(sceneDevice);
            impl->sceneDevices.push_back(sceneDevice);
        }
    }

    updateLinkPositions();
    updateSceneDevices(0.0);
    notifyUpdate(SgUpdate::REMOVED | SgUpdate::ADDED | SgUpdate::MODIFIED);
}


void SceneBody::cloneShapes(CloneMap& cloneMap)
{
    for(size_t i=0; i < sceneLinks_.size(); ++i){
        sceneLinks_[i]->impl->cloneShape(cloneMap);
    }
}


void SceneBody::updateLinkPositions()
{
    const int n = sceneLinks_.size();
    for(int i=0; i < n; ++i){
        SceneLinkPtr& sLink = sceneLinks_[i];
        sLink->setRotation(sLink->link()->R());
        sLink->setTranslation(sLink->link()->translation());
    }
}


void SceneBody::updateLinkPositions(SgUpdate& update)
{
    const int n = sceneLinks_.size();
    for(int i=0; i < n; ++i){
        SceneLinkPtr& sLink = sceneLinks_[i];
        sLink->setRotation(sLink->link()->R());
        sLink->setTranslation(sLink->link()->translation());
        sLink->notifyUpdate(update);
    }
}


SceneDevice* SceneBody::getSceneDevice(Device* device)
{
    const int linkIndex = device->link()->index();
    if(linkIndex >= 0 && linkIndex < static_cast<int>(sceneLinks_.size())){
        return sceneLinks_[linkIndex]->getSceneDevice(device);
    }
    return 0;
}


void SceneBody::setSceneDeviceUpdateConnection(bool on)
{
    auto& sceneDevices = impl->sceneDevices;
    for(size_t i=0; i < sceneDevices.size(); ++i){
        sceneDevices[i]->setSceneUpdateConnection(on);
    }
}


void SceneBody::updateSceneDevices(double time)
{
    auto& sceneDevices = impl->sceneDevices;
    for(size_t i=0; i < sceneDevices.size(); ++i){
        sceneDevices[i]->updateScene(time);
    }
}


void SceneBody::setTransparency(float transparency)
{
    for(size_t i=0; i < sceneLinks_.size(); ++i){
        sceneLinks_[i]->impl->setTransparency(transparency, false);
    }
    notifyUpdate();
}


void SceneBody::makeTransparent(float transparency, CloneMap&)
{
    setTransparency(transparency);
}


void SceneBody::makeTransparent(float transparency)
{
    setTransparency(transparency);
}
