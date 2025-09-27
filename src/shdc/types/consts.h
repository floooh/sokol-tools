#pragma once

namespace shdc {

// keep these in sync with:
//  - SG_MAX_UNIFORMBLOCKS
//  - SG_MAX_VIEW_BINDSLOTS
//  - SG_MAX_SAMPLER_BINDSLOTS
//
inline static const int MaxUniformBlocks = 8;
inline static const int MaxViews = 32;
inline static const int MaxSamplers = 12;
inline static const int MaxTextureSamplers = MaxViews;

} // namespace shdc
