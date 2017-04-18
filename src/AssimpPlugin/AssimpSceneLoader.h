/*!
  @file
  @author Shizuko Hattori, Shin'ichiro Nakaoka
*/

#ifndef CNOID_ASSIMP_ASSIMP_SCENE_LOADER_H
#define CNOID_ASSIMP_ASSIMP_SCENE_LOADER_H

#include <cnoid/AbstractSceneLoader>

namespace cnoid {

class AssimpSceneLoaderImpl;

class AssimpSceneLoader : public AbstractSceneLoader
{
public:
    AssimpSceneLoader();
    ~AssimpSceneLoader();
    void setMessageSink(std::ostream& os) override;
    virtual SgNodePtr load(const std::string& fileName) override;

private:
    AssimpSceneLoaderImpl* impl;
};

};

#endif
