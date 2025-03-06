#pragma once

#include <level/level.hpp>
#include <jolt/Jolt.h>
#include <jolt/Physics/Collision/Shape/Shape.h>
#include <jolt/Physics/Collision/CollisionCollector.h>

namespace bee {

class CollectableCollisionCollector : public JPH::CollideShapeCollector
{
public:

    virtual void AddHit(const ResultType& inResult) override;
    bool Collided();

    std::vector<JPH::BodyID> collidedBodies;
};

class BodyCollisionCollector : public JPH::CollideShapeCollector
{
public:

    struct Data
    {
        glm::vec3 normal;
        float magnitude;
    };

    virtual void AddHit(const ResultType& inResult) override;
    bool Collided();

    std::vector<Data> results;

    float totalMagnitude = 0;
};

void PhysicsTest();
void TerrainCollisionHandlingSystem(std::shared_ptr<Level> level, float dt);

}