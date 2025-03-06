#include <precompiled/engine_precompiled.hpp>
#include "physics/debug_renderer.hpp"

#include "core/engine.hpp"
#include "rendering/debug_render.hpp"

#include "physics/helpers.hpp"

#if defined(JPH_DEBUG_RENDERER)

class BatchImpl : public JPH::RefTargetVirtual
{
public:
    JPH_OVERRIDE_NEW_DELETE

    virtual void AddRef() override { ++m_refCount; }
    virtual void Release() override { if (--m_refCount == 0) delete this; }

    JPH::Array<JPH::DebugRenderer::Triangle> m_triangles;

private:
    JPH::atomic<JPH::uint32> m_refCount = 0;
};

PhysicsRendererImpl::PhysicsRendererImpl()
{
    Initialize();
}

void PhysicsRendererImpl::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    bee::Engine.DebugRenderer().AddLine(bee::DebugCategory::Enum::Physics, JoltToGlm(inFrom), JoltToGlm(inTo), JoltToGlm(inColor));
}

void PhysicsRendererImpl::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off)
{
    DrawLine(inV1, inV2, inColor);
    DrawLine(inV2, inV3, inColor);
    DrawLine(inV3, inV1, inColor);
}

JPH::DebugRenderer::Batch PhysicsRendererImpl::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
{
    BatchImpl* batch = new BatchImpl;
    if (inTriangles == nullptr || inTriangleCount == 0)
    {
        return batch;
    }

    batch->m_triangles.assign(inTriangles, inTriangles + inTriangleCount);
    return batch;
}

JPH::DebugRenderer::Batch PhysicsRendererImpl::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount)
{
    BatchImpl* batch = new BatchImpl;
    if (inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0)
    {
        return batch;
    }

    // Convert indexed triangle list to triangle list
    batch->m_triangles.resize(inIndexCount / 3);
    for (size_t i = 0; i < batch->m_triangles.size(); i += 1)
    {
 	    Triangle& triangle = batch->m_triangles[i];
 	    triangle.mV[0] = inVertices[inIndices[i * 3 + 0]];
 	    triangle.mV[1] = inVertices[inIndices[i * 3 + 1]];
 	    triangle.mV[2] = inVertices[inIndices[i * 3 + 2]];
    }

    return batch;
}

void PhysicsRendererImpl::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On, EDrawMode inDrawMode = EDrawMode::Solid)
{
    const LOD* lod = inGeometry->mLODs.data();
    
    const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());

    for (const Triangle& triangle : batch->m_triangles)
    {
        JPH::RVec3 v0 = inModelMatrix * JPH::Vec3(triangle.mV[0].mPosition);
        JPH::RVec3 v1 = inModelMatrix * JPH::Vec3(triangle.mV[1].mPosition);
        JPH::RVec3 v2 = inModelMatrix * JPH::Vec3(triangle.mV[2].mPosition);
        JPH::Color color = inModelColor * triangle.mV[0].mColor;

        DrawTriangle(v0, v1, v2, color, inCastShadow);
    }
}
    
void PhysicsRendererImpl::DrawText3D(JPH::RVec3Arg inPosition, const JPH::string_view& inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f)
{

}

#endif