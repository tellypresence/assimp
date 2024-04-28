/**
* EXPERIMENTAL
* All code in this source/header pair should be considered suspect/untested,
* trying (and struggling) to make sense of nodes/bones/animations for USD as
* provided by "tinyusdz" project
*/

#ifndef ASSIMP_BUILD_NO_USD_IMPORTER
#include <memory>
#include <sstream>

// internal headers
#include <assimp/ai_assert.h>
#include <assimp/anim.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/fast_atof.h>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/IOSystem.hpp>
#include <assimp/StringUtils.h>
#include <assimp/StreamReader.h>

#include "io-util.hh" // namespace tinyusdz::io
#include "tydra/scene-access.hh"
#include "tydra/shader-network.hh"
#include "USDLoaderImplTinyusdzHelper.h"
#include "USDLoaderImplTinyusdz.h"
#include "USDLoaderUtil.h"

#include "../../../contrib/tinyusdz/assimp_tinyusdz_logging.inc"

namespace {
const char *const TAG = "tusdz EXPERIMENTAL";
}

namespace Assimp {
using namespace std;

void USDImporterImplTinyusdz::setupBonesNAnim(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    stringstream ss;
    std::map<size_t, tinyusdz::tydra::Node> meshNodes;
    pScene->mRootNode = nodes(render_scene, meshNodes, nameWExt);
    ss.str("");
    ss << "setupBonesNAnim(): model" << nameWExt << ", meshNodes now has " << meshNodes.size() << " items";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    meshesBonesNAnim(
            render_scene,
            pScene,
            meshNodes,
            nameWExt);
    animations(render_scene, pScene, nameWExt);
}

aiNode *USDImporterImplTinyusdz::nodes(
        const tinyusdz::tydra::RenderScene &render_scene,
        std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    const size_t numNodes{render_scene.nodes.size()};
    (void) numNodes; // Ignore unused variable when -Werror enabled
    stringstream ss;
    ss.str("");
    ss << "nodes(): model" << nameWExt << ", numNodes: " << numNodes;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    return nodesRecursive(nullptr, render_scene.nodes[0], meshNodes);
}

using Assimp::tinyusdzNodeTypeFor;
using Assimp::tinyUsdzMat4ToAiMat4;
using tinyusdz::tydra::NodeType;
aiNode *USDImporterImplTinyusdz::nodesRecursive(
        aiNode *pNodeParent,
        const tinyusdz::tydra::Node &node,
        std::map<size_t, tinyusdz::tydra::Node> &meshNodes) {
    stringstream ss;
    aiNode *cNode = new aiNode();
    cNode->mParent = pNodeParent;
    cNode->mName.Set(node.prim_name);
    cNode->mTransformation = tinyUsdzMat4ToAiMat4(node.local_matrix.m);
    ss.str("");
    ss << "nodesRecursive(): node " << cNode->mName.C_Str() <<
            " type: |" << tinyusdzNodeTypeFor(node.nodeType) <<
            "|, disp " << node.display_name << ", abs " << node.abs_path;
    if (cNode->mParent != nullptr) {
        ss << " (parent " << cNode->mParent->mName.C_Str() << ")";
    }
    ss << " has " << node.children.size() << " children";
    if (node.nodeType == NodeType::Mesh) {
        meshNodes[node.id] = node;
        ss << "\n    node mesh id: " << node.id;
    }
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    if (!node.children.empty()) {
        cNode->mNumChildren = node.children.size();
        cNode->mChildren = new aiNode *[cNode->mNumChildren];
    }

    size_t i{0};
    for (const auto &childNode: node.children) {
        cNode->mChildren[i] = nodesRecursive(cNode, childNode, meshNodes);
        ++i;
    }
    return cNode;
}

void USDImporterImplTinyusdz::sanityCheckNodesRecursive(
        aiNode *cNode) {
    stringstream ss;
    ss.str("");
    ss << "sanityCheckNodesRecursive(): node " << cNode->mName.C_Str();
    if (cNode->mParent != nullptr) {
        ss << " (parent " << cNode->mParent->mName.C_Str() << ")";
    }
    ss << " has " << cNode->mNumChildren << " children";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    for (size_t i = 0; i < cNode->mNumChildren; ++i) {
        sanityCheckNodesRecursive(cNode->mChildren[i]);
    }
}

void USDImporterImplTinyusdz::meshesBonesNAnim(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    stringstream ss;
    pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
    pScene->mRootNode->mMeshes = new unsigned int[pScene->mRootNode->mNumMeshes];
    ss.str("");
    ss << "meshesBonesNAnim(): pScene->mNumMeshes: " << pScene->mNumMeshes << ", mRootNode->mNumMeshes: " << pScene->mRootNode->mNumMeshes;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

    size_t bonesCount{0};
    for (size_t meshIdx = 0; meshIdx < pScene->mNumMeshes; meshIdx++) {
        bonesCount += bonesForMesh(render_scene, pScene, meshIdx, meshNodes, nameWExt);
        blendShapesForMesh(render_scene, pScene, meshIdx, nameWExt);
        pScene->mRootNode->mMeshes[meshIdx] = static_cast<unsigned int>(meshIdx);
    }
    ss.str("");
    ss << "meshesBonesNAnim(): bonesCount: " << bonesCount;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
}

size_t USDImporterImplTinyusdz::bonesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    stringstream ss;

    //    render_scene.meshes[meshIdx].joint_and_weights.jointIndices.size() << " jointIndices, " <<
    //    render_scene.meshes[meshIdx].joint_and_weights.jointWeights.size() << " jointWeights, elementSize: " <<
    //    render_scene.meshes[meshIdx].joint_and_weights.elementSize
    size_t numWeights{render_scene.meshes[meshIdx].joint_and_weights.jointWeights.size()};
    std::vector<aiVertexWeight> newWeights;
    newWeights.reserve(numWeights);
    std::vector<aiBone *> newBones;
    // TODO: support > 1 bone per mesh
    for (size_t iBone = 0; iBone < 1; ++iBone) {
        for (unsigned int d = 0; d < numWeights; ++d) {
            size_t vertIdx = render_scene.meshes[meshIdx].joint_and_weights.jointIndices[d];
            ai_real w = render_scene.meshes[meshIdx].joint_and_weights.jointWeights[vertIdx];
            if (w > 0.0) {
                newWeights.emplace_back(vertIdx, w);
            }
        }
        if (newWeights.empty()) {
            ss.str("");
            ss << "bonesForMesh(): mesh[" << meshIdx << "] has no weights";
            TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
            return newWeights.size();
        }
        ss.str("");
        ss << "bonesForMesh(), " << nameWExt << ": bones for mesh[" << meshIdx << "]: " << newWeights.size() << " weights";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        aiBone *nbone = new aiBone;
        if (meshNodes.find(meshIdx) != meshNodes.end()) {
            nbone->mName.Set(meshNodes.at(meshIdx).prim_name);
            //            nbone->mOffsetMatrix = tinyUsdzMat4ToAiMat4(meshNodes.at(meshIdx).local_matrix.m);
            nbone->mOffsetMatrix = tinyUsdzMat4ToAiMat4(meshNodes.at(meshIdx).global_matrix.m);
            //            nbone->mOffsetMatrix = aiMatrix4x4(); // TODO: sanity check
            ss.str("");
            ss << "bonesForMesh(): mesh[" << meshIdx << "] nbone->mName: " << nbone->mName.C_Str();
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        } else {
            ss.str("");
            ss << "bonesForMesh(): NOTICE: no node for mesh[" << meshIdx << "]!";
            TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
        }
        nbone->mNumWeights = newWeights.size();
        ss.str("");
        ss << "bonesForMesh(): mesh[" << meshIdx << "] nbone->mNumWeights: " << nbone->mNumWeights;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        nbone->mWeights = new aiVertexWeight[nbone->mNumWeights];
        for (unsigned int d = 0; d < newWeights.size(); ++d) {
            nbone->mWeights[d] = newWeights[d];
        }
        newBones.push_back(nbone);
    }

    // store the bones in the mesh
    pScene->mMeshes[meshIdx]->mNumBones = newBones.size();
    if (!newBones.empty()) {
        pScene->mMeshes[meshIdx]->mBones = new aiBone *[pScene->mMeshes[meshIdx]->mNumBones];
        std::copy(newBones.begin(), newBones.end(), pScene->mMeshes[meshIdx]->mBones);
    }
    ss.str("");
    ss << "bonesForMesh(): mesh[" << meshIdx << "] mNumBones: " << pScene->mMeshes[meshIdx]->mNumBones;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    for (size_t i = 0; i < pScene->mMeshes[meshIdx]->mNumBones; ++i) {
        ss.str("");
        ss << "    mesh[" << meshIdx << "] bone[" << i << "]: " << pScene->mMeshes[meshIdx]->mBones[i]->mName.C_Str();
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    }
    return pScene->mMeshes[meshIdx]->mNumBones;
}

//tinyusdz::tydra::Node USDImporterImplTinyusdz::bonesRecursive(
//        const tinyusdz::tydra::Node &node) {
//
//}

using AnimSamplerTransOrScale = tinyusdz::tydra::AnimationSampler<std::array<float, 3>>;
using AnimSamplerRot = tinyusdz::tydra::AnimationSampler<std::array<float, 4>>;

static void addBoneTranslations(
        aiAnimation *nanim,
        aiNodeAnim *nbone, AnimSamplerTransOrScale translations) {
    stringstream ss;
    nbone->mNumPositionKeys = translations.samples.size();
    nbone->mPositionKeys = new aiVectorKey[nbone->mNumPositionKeys];
    size_t i{0};
    for (const auto translate : translations.samples) {
        aiVector3D pos(translate.value[0], translate.value[1], translate.value[2]);
        nbone->mPositionKeys[i].mValue = pos;
        nbone->mPositionKeys[i].mTime = translate.t;
//        if (nbone->mPositionKeys[i].mTime > nanim->mDuration) {
//            ss.str("");
//            ss << "        addBoneTranslations(): updating mDuration (pos samp " << i << ") from " <<
//                    nanim->mDuration << " to " << nbone->mPositionKeys[i].mTime;
//            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
//        }
        nanim->mDuration = std::max(nanim->mDuration, nbone->mPositionKeys[i].mTime);
        ++i;
    }
}

static void addBoneRotations(
        aiAnimation *nanim,
        aiNodeAnim *nbone, AnimSamplerRot rotations) {
    stringstream ss;
    nbone->mNumRotationKeys = rotations.samples.size();
    nbone->mRotationKeys = new aiQuatKey[nbone->mNumRotationKeys];
    size_t i{0};
    for (const auto rotIter : rotations.samples) {
        nbone->mRotationKeys[i].mValue = aiQuaternion(
                rotIter.value[0], rotIter.value[1], rotIter.value[2], rotIter.value[3]);
        nbone->mRotationKeys[i].mTime = rotIter.t;
//        if (nbone->mRotationKeys[i].mTime > nanim->mDuration) {
//            ss.str("");
//            ss << "        addBoneRotations(): updating mDuration (rot samp " << i << ") from " <<
//                    nanim->mDuration << " to " << nbone->mRotationKeys[i].mTime;
//            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
//        }
        nanim->mDuration = std::max(nanim->mDuration, nbone->mRotationKeys[i].mTime);
        ++i;
    }
}

static void addBoneScales(
        aiAnimation *nanim,
        aiNodeAnim *nbone, AnimSamplerTransOrScale scales) {
    stringstream ss;
    nbone->mNumScalingKeys = scales.samples.size();
    nbone->mScalingKeys = new aiVectorKey[nbone->mNumScalingKeys];
    size_t i{0};
    for (const auto scaleIter : scales.samples) {
        aiVector3D scale(scaleIter.value[0], scaleIter.value[1], scaleIter.value[2]);
        nbone->mScalingKeys[i].mValue = scale;
        nbone->mScalingKeys[i].mTime = scaleIter.t;
//        if (nbone->mScalingKeys[i].mTime > nanim->mDuration) {
//            ss.str("");
//            ss << "        addBoneScales(): updating mDuration (sca samp " << i << ") from " <<
//                    nanim->mDuration << " to " << nbone->mScalingKeys[i].mTime;
//            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
//        }
        nanim->mDuration = std::max(nanim->mDuration, nbone->mScalingKeys[i].mTime);
        ++i;
    }
}

using tinyusdz::tydra::AnimationChannel;
using ChannelType = AnimationChannel::ChannelType;
void USDImporterImplTinyusdz::animations(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numAnimations{render_scene.animations.size()};
    (void) numAnimations; // Ignore unused variable when -Werror enabled
    stringstream ss;
    ss.str("");
    ss << "animations(): model" << nameWExt << ", numAnimations: " << numAnimations;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    size_t i = 0;
    std::vector<aiAnimation *> newAnims;
    aiAnimation *nanim = nullptr;
    // TODO: currently believe only a single animation supported
    if (!render_scene.animations.empty()) {
        nanim = new aiAnimation;
        newAnims.push_back(nanim);
        nanim->mName.Set(render_scene.animations[0].display_name.c_str());
        nanim->mDuration = 0;
        nanim->mTicksPerSecond = 24; // TODO: fix this
    }
    for (const auto &animation : render_scene.animations) {

        ss.str("");
        ss << "    animation[" << i << "]: name: |" << animation.display_name << "|";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

        if (!animation.blendshape_weights_map.empty()) {
            ss.str("");
            ss << "    animation[" << i << "] has " << animation.blendshape_weights_map.size() << " blendshape weights";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        }

        auto bwMapIter = animation.blendshape_weights_map.begin();
        //std::map<std::string, std::vector<AnimationSample<float>>> blendshape_weights_map;
        size_t ich = 0;
        for (; bwMapIter != animation.blendshape_weights_map.end(); ++bwMapIter) {
            const std::string &name{bwMapIter->first};
            const auto &vecAnimSamples{bwMapIter->second};
            ss.str("");
            ss << "        blendshape_weights_map[" << i << "][" << ich << "] blendshape name " << name << " has " <<
                    vecAnimSamples.size() << " samples";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
            ++ich;
        }

        nanim->mNumChannels = animation.channels_map.size();
        nanim->mChannels = new aiNodeAnim *[nanim->mNumChannels];

        ss.str("");
        ss << "    animation[" << i << "] has " << animation.channels_map.size() << " channels";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        parseMapKeyJointToValAnimChannelsMap(nanim, animation.channels_map);
        ss.str("");
        ss << "    animation[" << i << "] done...";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ++i;
    }
    ss.str("");
    ss << "animations(): anims done (i now " << i << ")...";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

    // store all converted animations in the scene
    if (newAnims.size() > 0) {
        pScene->mNumAnimations = (unsigned int)newAnims.size();
        pScene->mAnimations = new aiAnimation *[pScene->mNumAnimations];
        for (unsigned int a = 0; a < newAnims.size(); a++)
            pScene->mAnimations[a] = newAnims[a];
    }
}

void USDImporterImplTinyusdz::parseMapKeyJointToValAnimChannelsMap(
        aiAnimation *nanim,
        const std::map<std::string,
                std::map<ChannelType, AnimationChannel>> &mapKeyJointToValAnimChannelsMap
) {
    stringstream ss;
    auto mapIter = mapKeyJointToValAnimChannelsMap.begin();
    size_t ich{0};
    for (; mapIter != mapKeyJointToValAnimChannelsMap.end(); ++mapIter) {
        aiNodeAnim *nbone = new aiNodeAnim;
        nbone->mNodeName.Set(mapIter->first.c_str());
        nanim->mChannels[ich] = nbone;
        //            nbone->mNumPositionKeys = mapIter->second.size();
        //            nbone->mPositionKeys = new aiVectorKey[nbone->mNumPositionKeys];
        //            nbone->mNumRotationKeys = mapIter->second.size();
        //            nbone->mRotationKeys = new aiQuatKey[nbone->mNumRotationKeys];
        //            nbone->mNumScalingKeys = mapIter->second.size();
        //            nbone->mScalingKeys = new aiVectorKey[nbone->mNumScalingKeys];

        ss.str("");
        ss << "        channels_map[" << ich << "]: key: " << mapIter->first;
        //            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        parseMapKeyTypeToValAnimChannel(nanim, nbone, mapIter->second);
        ++ich;
    }
}

void USDImporterImplTinyusdz::parseMapKeyTypeToValAnimChannel(
        aiAnimation *nanim,
        aiNodeAnim *nbone,
        const std::map<ChannelType, AnimationChannel> &typeToChannelMap
) {
    auto tToCMapIter = typeToChannelMap.begin();
    for (; tToCMapIter != typeToChannelMap.end(); ++tToCMapIter) {
        parseAnimChannel(nanim, nbone, tToCMapIter->first, tToCMapIter->second);
    }
}

void USDImporterImplTinyusdz::parseAnimChannel(
        aiAnimation *nanim,
        aiNodeAnim *nbone,
        const AnimationChannel &animChannel
) {
    parseAnimChannel(
            nanim,
            nbone,
            animChannel.type,
            animChannel
    );
}

void USDImporterImplTinyusdz::parseAnimChannel(
        aiAnimation *nanim,
        aiNodeAnim *nbone,
        ChannelType type,
        const AnimationChannel &animChannel
) {
    stringstream ss;
    size_t i_trs = 0; // 0, 1, 2 for translate, rotate, scale: nothing useful here
    if (type != animChannel.type) {
        ss.str("");
        ss << "parseAnimChannel(): NOTICE: channel type mismatch; type (map key) " << tinyusdzAnimChannelTypeFor(type) << " but animChannel.type (from map value) " <<
                tinyusdzAnimChannelTypeFor(animChannel.type);
        TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
    }

    //                ss << "            types map[" << i << "][" << ich << "][" << i_trs << "][" <<
    //                   tinyusdzAnimChannelTypeFor(type) << "]: ";
    switch (type) {
        case ChannelType::Translation: {
            addBoneTranslations(nanim, nbone, animChannel.translations);
            //                        std::array<float, 3> translate = animChannel.translations.samples[i_trs].value;
            //                        aiVector3D pos(translate[0], translate[1], translate[2]);
            //                        nbone->mPositionKeys[i_trs].mTime = animChannel.translations.samples[i_trs].t;
            //                        nbone->mPositionKeys[i_trs].mValue = pos;
            //                        ss << animChannel.translations.samples.size() << " samples";
            //                        ss << "[" << pos[0] << ", " << pos[1] << ", " << pos[2] << "]";
            break;
        }
        case ChannelType::Rotation: {
            addBoneRotations(nanim, nbone, animChannel.rotations);
            //                        ss << animChannel.rotations.samples.size() << " samples";
            //                        ss << "[" <<
            //                                animChannel.rotations.samples[0].value[0] << ", " <<
            //                                animChannel.rotations.samples[0].value[1] << ", " <<
            //                                animChannel.rotations.samples[0].value[2] << ", " <<
            //                                animChannel.rotations.samples[0].value[3] << "]";
            break;
        }
        case ChannelType::Scale: {
            addBoneScales(nanim, nbone, animChannel.scales);
            //                        ss << animChannel.scales.samples.size() << " samples";
            //                        ss << "[" <<
            //                                animChannel.scales.samples[0].value[0] << ", " <<
            //                                animChannel.scales.samples[0].value[1] << ", " <<
            //                                animChannel.scales.samples[0].value[2] << "]";
            break;
        }
        case ChannelType::Weight: {
            //                        addBoneScales(nbone, animChannel.scales);
            ss << "            types map[" << i_trs << "][" <<
                    tinyusdzAnimChannelTypeFor(type) << "]: ";
            ss << animChannel.weights.samples.size() << " samples";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
            //                        ss << "[" <<
            //                                animChannel.scales.samples[0].value[0] << ", " <<
            //                                animChannel.scales.samples[0].value[1] << ", " <<
            //                                animChannel.scales.samples[0].value[2] << "]";
            break;
        }
        default:
            break;
    }
}

void USDImporterImplTinyusdz::blendShapes(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    stringstream ss;
    const size_t numBuffers{render_scene.buffers.size()};
    (void) numBuffers; // Ignore unused variable when -Werror enabled
    ss.str("");
    ss << "blendShapes(): model" << nameWExt << ", numBuffers: " << numBuffers;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    size_t i = 0;
    for (const auto &buffer : render_scene.buffers) {
        ss.str("");
        ss << "    buffer[" << i << "]: count: " << buffer.data.size() << ", type: " << to_string(buffer.componentType);
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ++i;
    }
}

void USDImporterImplTinyusdz::blendShapesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    stringstream ss;
    const size_t numBlendShapeTargets{render_scene.meshes[meshIdx].targets.size()};
    (void) numBlendShapeTargets; // Ignore unused variable when -Werror enabled
    if (numBlendShapeTargets > 0) {
        ss.str("");
        ss << "blendShapesForMesh(): mesh[" << meshIdx << "], model" << nameWExt << ", numBlendShapeTargets: " << numBlendShapeTargets;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    }
    auto mapIter = render_scene.meshes[meshIdx].targets.begin();
    size_t i{0};
    for (; mapIter != render_scene.meshes[meshIdx].targets.end(); ++mapIter) {
        // mapIter: std::map<std::string, ShapeTarget>
        const std::string name{mapIter->first};
        const tinyusdz::tydra::ShapeTarget shapeTarget{mapIter->second};
        ss.str("");
        ss << "    target[" << i << "]: name: " << name << ", prim_name: " <<
                shapeTarget.prim_name << ", abs_path: " << shapeTarget.abs_path <<
                ", display_name: " << shapeTarget.display_name << ", " << shapeTarget.pointIndices.size() <<
                " pointIndices, " << shapeTarget.pointOffsets.size() << " pointOffsets, " <<
                shapeTarget.normalOffsets.size() << " normalOffsets, " << shapeTarget.inbetweens.size() <<
                " inbetweens";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ++i;
    }
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
