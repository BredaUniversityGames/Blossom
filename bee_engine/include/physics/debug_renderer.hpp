#pragma once

#include "jolt/Jolt.h"

#if defined(JPH_DEBUG_RENDERER)

#include <jolt/Renderer/DebugRenderer.h>

class PhysicsRendererImpl final : public JPH::DebugRenderer
{
public:

    PhysicsRendererImpl();

    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;

    virtual Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;

    virtual Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override;

    virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) override;
    
    virtual void DrawText3D(JPH::RVec3Arg inPosition, const JPH::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
};

#endif

#if defined(JPH_DEBUG_RENDERER) && defined(BEE_PLATFORM_PC)

#define EDITOR_ONLY(call) call

#else

#define EDITOR_ONLY(call) 

#endif
