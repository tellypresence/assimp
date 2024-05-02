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
        std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    stringstream ss;
    std::map<size_t, tinyusdz::tydra::SkelNode> mapSkelNodes;
    skelNodes(render_scene, mapSkelNodes, nameWExt);
    ss.str("");
    ss << "setupBonesNAnim(): model: " << nameWExt << ", mapSkelNodes now has " << mapSkelNodes.size() << " items";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    meshesBonesNAnim(
            render_scene,
            pScene,
            meshNodes,
            mapSkelNodes,
            nameWExt);
    skeletons(render_scene, pScene, nameWExt);
    animations(render_scene, pScene, nameWExt);
}

/*aiNode * */void USDImporterImplTinyusdz::skelNodes(
        const tinyusdz::tydra::RenderScene &render_scene,
        std::map<size_t, tinyusdz::tydra::SkelNode> &mapSkelNodes,
        const std::string &nameWExt) {
    stringstream ss;
    const size_t numSkeletons{render_scene.skeletons.size()};
    ss.str("");
    ss << "skelNodes(): model" << nameWExt << ", numSkeletons: " << numSkeletons;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    for (size_t iSkel = 0; iSkel < numSkeletons; ++iSkel) {
        ss.str("");
        ss << "    skelHierarchy[" << iSkel << "]: prim_name: " << render_scene.skeletons[iSkel].prim_name <<
                ", anim_id: " << render_scene.skeletons[iSkel].anim_id <<
                ", disp name: " << render_scene.skeletons[iSkel].display_name <<
                ", abs_path: " << render_scene.skeletons[iSkel].abs_path <<
                ", root id: " << render_scene.skeletons[iSkel].root_node.joint_id;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        /*return*/ skelNodesRecursive(nullptr, render_scene.skeletons[iSkel].root_node, mapSkelNodes);
    }
}

/*aiNode * */void USDImporterImplTinyusdz::skelNodesRecursive(
        aiNode *pNodeParent,
        const tinyusdz::tydra::SkelNode &skelNode,
        std::map<size_t, tinyusdz::tydra::SkelNode> &mapSkelNodes) {
    stringstream ss;
//    aiNode *cNode = new aiNode();
//    cNode->mParent = pNodeParent;
//    cNode->mName.Set(node.prim_name);
//    cNode->mTransformation = tinyUsdzMat4ToAiMat4(node.local_matrix.m);
    ss.str("");
    ss << "        skelNodesRecursive(): joint_id " << skelNode.joint_id << ", joint_name |" <<
            skelNode.joint_name << "|, joint_path |" << skelNode.joint_path;
//    if (cNode->mParent != nullptr) {
//        ss << " (parent " << cNode->mParent->mName.C_Str() << ")";
//    }
    ss << " has " << skelNode.children.size() << " children";
//    if (skelNode.joint_id > -1) {
//        ss << "\n    node mesh id: " << node.id << " (node type: " << tinyusdzNodeTypeFor(node.nodeType) << ")";
//        mapSkelNodes[node.id] = node;
//    }
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
//    if (!skelNode.children.empty()) {
//        cNode->mNumChildren = node.children.size();
//        cNode->mChildren = new aiNode *[cNode->mNumChildren];
//    }

//    size_t i{0};
    for (const auto &childNode: skelNode.children) {
        /*cNode->mChildren[i] =*/ skelNodesRecursive(nullptr, childNode, mapSkelNodes);
//        ++i;
    }
//    return cNode;
}

void USDImporterImplTinyusdz::meshesBonesNAnim(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::map<size_t, tinyusdz::tydra::SkelNode> &mapSkelNodes,
        const std::string &nameWExt) {
    stringstream ss;

    size_t bonesCount{0};
    size_t bonesCountViaJointNWeights{0};
    for (size_t meshIdx = 0; meshIdx < pScene->mNumMeshes; meshIdx++) {
        bonesCountViaJointNWeights += jointAndWeightsForMesh(render_scene, pScene, meshIdx, meshNodes, nameWExt);
        // TODO: disabled to prevent disappearing models due to incorrect transformations
//        bonesCount += bonesForMesh(render_scene, pScene, meshIdx, meshNodes, nameWExt);
    }
    ss.str("");
    ss << "meshesBonesNAnim(): bonesCountViaJointNWeights: " << bonesCountViaJointNWeights <<
            ", bonesCount: " << bonesCount;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
}

size_t USDImporterImplTinyusdz::jointAndWeightsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    stringstream ss;
    ss.str("");
    ss << "jointAndWeightsForMesh(): mesh[" << meshIdx << "] skel_id: " << render_scene.meshes[meshIdx].skel_id;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    return 0;
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
            nbone->mOffsetMatrix = tinyUsdzMat4ToAiMat4(meshNodes.at(meshIdx).local_matrix.m);
//            nbone->mOffsetMatrix = tinyUsdzMat4ToAiMat4(meshNodes.at(meshIdx).global_matrix.m);
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
        nbone->mPositionKeys[i].mValue = tinyUsdzScaleOrPosToAssimp(translate.value);
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
        nbone->mRotationKeys[i].mValue = tinyUsdzQuatToAiQuat(rotIter.value);
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
        nbone->mScalingKeys[i].mValue = tinyUsdzScaleOrPosToAssimp(scaleIter.value);
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

void USDImporterImplTinyusdz::skeletons(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    stringstream ss;
    const size_t numSkeletons{render_scene.skeletons.size()};
    ss.str("");
    ss << "skeletons(): model" << nameWExt << ", numSkeletons: " << numSkeletons;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
}

using tinyusdz::tydra::AnimationChannel;
using ChannelType = AnimationChannel::ChannelType;
static const float kMillisecondsFromSeconds = 1000.f;
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
            nanim->mNumMorphMeshChannels = animation.blendshape_weights_map.size();
            nanim->mMorphMeshChannels = new aiMeshMorphAnim *[nanim->mNumMorphMeshChannels];
            ss.str("");
            ss << "    animation[" << i << "] has " << animation.blendshape_weights_map.size() << " blendshape weights";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        }

        auto bwMapIter = animation.blendshape_weights_map.begin();
        //std::map<std::string, std::vector<AnimationSample<float>>> blendshape_weights_map;
        size_t iBsCh = 0;
        for (; bwMapIter != animation.blendshape_weights_map.end(); ++bwMapIter) {
            const std::string &name{bwMapIter->first};
            const auto &vecAnimSamples{bwMapIter->second}; // std::vector<AnimationSample<float>>
            ss.str("");
            ss << "    anim[" << i << "] blend weight[" << iBsCh << "]: " <<
                    name << ", " << vecAnimSamples.size() << " samples (keys?)";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

            aiMeshMorphAnim *morphAnim = new aiMeshMorphAnim;
            nanim->mMorphMeshChannels[iBsCh] = morphAnim;
            morphAnim->mName.Set(name);
            morphAnim->mNumKeys = vecAnimSamples.size();
            morphAnim->mKeys = new aiMeshMorphKey[morphAnim->mNumKeys];
            for (size_t key = 0; key < morphAnim->mNumKeys; ++key) {
                morphAnim->mKeys[key].mNumValuesAndWeights = static_cast<unsigned int>(vecAnimSamples.size());
                morphAnim->mKeys[key].mValues = new unsigned int[vecAnimSamples.size()];
                morphAnim->mKeys[key].mWeights = new double[vecAnimSamples.size()];

                morphAnim->mKeys[key].mTime = vecAnimSamples[key].t * kMillisecondsFromSeconds;
                for (size_t valueIndex = 0; valueIndex < vecAnimSamples.size(); ++valueIndex) {
                    ss.str("");
                    ss << "        anim[" << i << "] blend weight[" << iBsCh << "] key[" << key << "] val idx [" <<
                            valueIndex << "]: vecAnimSamples[" << vecAnimSamples[key].value << "].value: " << vecAnimSamples[key].value;
                    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
                    morphAnim->mKeys[key].mValues[valueIndex] = valueIndex;
                    morphAnim->mKeys[key].mWeights[valueIndex] = vecAnimSamples[key].value;
                }
            }
            ss.str("");
            ss << "        blendshape_weights_map[" << i << "][" << iBsCh << "] blendshape name " << name << " has " <<
                    vecAnimSamples.size() << " samples";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
            ++iBsCh;
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

    // store all converted animations in the scene
    ss.str("");
    ss << "animations(): newAnims.size(): " << newAnims.size();
    newAnims.size() > 0 ? (ss << ", newAnims[0].mNumChannels: " << newAnims[0]->mNumChannels) : ss << "";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    if (newAnims.size() > 0  &&
            newAnims[0]->mNumChannels > 0) { // TODO: update for blend shapes
        pScene->mNumAnimations = newAnims.size();
        pScene->mAnimations = new aiAnimation *[pScene->mNumAnimations];
        for (unsigned int a = 0; a < newAnims.size(); a++)
            pScene->mAnimations[a] = newAnims[a];
        ss.str("");
        ss << "animations(): pScene->mNumAnimations: " << pScene->mNumAnimations;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    }

    ss.str("");
    ss << "animations(): anims done (i now " << i << ")...";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
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
        ss.str("");
        ss << "        channels_map[" << ich << "]: key: " << mapIter->first;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
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

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
