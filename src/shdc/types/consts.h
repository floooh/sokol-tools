#pragma once

namespace shdc {

// keep these in sync with:
//  - SG_MAX_VIEW_BINDSLOTS
//  - SG_MAX_UNIFORMBLOCKS
//  - SG_MAX_TEXTURE_BINDINGS_PER_STAGE
//  - SG_MAX_STORAGEBUFFER_BINDINGS_PER_STAGE
//  - SG_MAX_STORAGEIMAGE_BINDINGS_PER_STAGE
//  - SG_MAX_SAMPLER_BINDSLOTS
//  - SG_MAX_TEXTURE_SAMPLERS_PAIRS
//
inline static const int MaxUniformBlocks = 8;
inline static const int MaxSamplers = 16;
inline static const int MaxTextureBindingsPerStage = 16;
inline static const int MaxStorageBufferBindingsPerStage = 8;
inline static const int MaxStorageImageBindingsPerStage = 4;
inline static const int MaxViews = 28;
inline static const int MaxTextureSamplers = 16;
static_assert(MaxViews == MaxTextureBindingsPerStage + MaxStorageBufferBindingsPerStage + MaxStorageImageBindingsPerStage);

} // namespace shdc
