#pragma once
#ifndef AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED
#define AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include "tinyusdz.hh"
#include "tydra/render-data.hh"

namespace Assimp {

std::string tinyusdzAnimChannelTypeFor(
        tinyusdz::tydra::AnimationChannel::ChannelType animChannel);
std::string tinyusdzNodeTypeFor(tinyusdz::tydra::NodeType type);
aiMatrix4x4 tinyUsdzMat4ToAiMat4(const double matIn[4][4]);

} // namespace Assimp
#endif // AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED
