#include <precompiled/editor_precompiled.hpp>
#include <level_editor/prop_builder.hpp>

#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <entt/entity/runtime_view.hpp>
#include <level/level.hpp>
#include <core/transform.hpp>
#include <terrain/terrain_chunk.hpp>
#include <tools/log.hpp>
#include <level/level.hpp>
#include <glm/gtx/norm.hpp>
#include <platform/opengl/shader_gl.hpp>
#include <resources/resource_handle.hpp>
#include <resources/resource_manager.hpp>

#include <platform/opengl/open_gl.hpp>
#include <resources/image/image_gl.hpp>

#define MAX_PROPS_IN_COMPUTE 1024 * 1024

// data needed from shader side
#define PROP_GROUP_SIZE 128

namespace bee {

//With great help from https://github.com/sevanetrebchenko/fast-poisson-disk-sampling/blob/master/fpds.hpp
class PoissonGrid {
public:
    PoissonGrid(const glm::vec2& dimensions, float spacing)
    {
        cellSize = spacing / sqrtf(DIMENSIONS);
        
        gridWidth = static_cast<int>(std::ceil(dimensions.x / cellSize));
        gridHeight = static_cast<int>(std::ceil(dimensions.y / cellSize));
        gridSize = gridWidth * gridHeight;

        gridData.resize(gridSize, INVALID_ID);
    }

    // 2D index into flattened array.
    int Get(int x, int y) const {
        return gridData[x + gridWidth * y];
    }

    void Set(int value, int x, int y) {
        gridData[x + gridWidth * y] = value;
    }

    std::pair<int, int> ConvertToGridCoords(const glm::vec2& coordinates) const {
        return { static_cast<int>(coordinates.x / cellSize),
                 static_cast<int>(coordinates.y / cellSize) };
    }

    int Width() const { return gridWidth; }
    int Height() const { return gridHeight; }

    static constexpr auto DIMENSIONS = 2;
    static constexpr auto INVALID_ID = -1;

private:

    float cellSize;

    int gridWidth;
    int gridHeight;
    int gridSize;

    std::vector<int> gridData;
};

struct prop_struct
{
    glm::vec3 position{};
    float scale = 1.0f;
    glm::vec3 surfaceNormal = { 0.0f, 0.0f, 1.0f };
    float percentDiscard = 1.0f;
};

bool TestSampleValidity(const PoissonGrid& grid, const std::vector<glm::vec2>& currentPoints, const glm::vec2& testCoords, float spacing) {

    auto [cellX, cellY] = grid.ConvertToGridCoords(testCoords);
    constexpr int CELLS_CHECKED = 2;

    for (int y = -CELLS_CHECKED; y <= CELLS_CHECKED; ++y) {
        for (int x = -CELLS_CHECKED; x <= CELLS_CHECKED; ++x) {
            if (x == 0 && y == 0) continue;

            int x_offset = cellX + x;
            int y_offset = cellY + y;

            // Ensure desired offset into the grid is in range.
            if (x_offset < 0 || x_offset >= grid.Width()) {
                continue;
            }
            if (y_offset < 0 || y_offset >= grid.Height()) {
                continue;
            }

            // Offset from current point.
            int testIndex = grid.Get(x_offset, y_offset);
            if (testIndex == PoissonGrid::INVALID_ID) continue;

            const glm::vec2& existing_sample = currentPoints.at(testIndex);

            // Ensure separation between current and test point is at least 'r'.
            if (glm::distance2(existing_sample, testCoords) < spacing * spacing) {
                return false;
            }
        }
    }

    return true;
}

std::vector<glm::vec2> PoissonDiskSampling(glm::vec2 dimensions, float spacing, pcg::RNGState& seed, int k = 30)
{
    PoissonGrid grid{ dimensions, spacing };

    std::vector<glm::vec2> outputPoints;
     std::vector<int> activePoints;

    glm::vec2 sampleCoords = glm::vec2(pcg::rand0_1(seed) * dimensions.x, pcg::rand0_1(seed) * dimensions.y);
    auto [startX, startY] = grid.ConvertToGridCoords(sampleCoords);

    int currentIndex = 0;

    grid.Set(currentIndex, startX, startY);
    activePoints.emplace_back(currentIndex);
    outputPoints.emplace_back(sampleCoords);

    while (!activePoints.empty()) {

        size_t selectedIndex = static_cast<size_t>(pcg::rand(seed)) % activePoints.size();
        sampleCoords = outputPoints.at(activePoints.at(selectedIndex));

        bool foundSample = false;

        for (int i = 0; i < k; ++i) {

            // Uniformly generate test points between 'r' and '2r' distance away around the chosen point.
            float radians = pcg::rand0_1(seed) * 2.0f * glm::pi<float>();
            float radius = pcg::rand0_1(seed) * spacing + spacing;

            auto testCoords = sampleCoords + glm::vec2 {
                radius * std::cosf(radians), radius * std::sinf(radians)
            };

            // Ensure offsetting point did not push it out of bounds.
            if (testCoords.x < 0.0f || testCoords.x >= dimensions.x) {
                continue;
            }
            if (testCoords.y < 0.0f || testCoords.y >= dimensions.y) {
                continue;
            }

            auto [testX, testY] = grid.ConvertToGridCoords(testCoords);
            auto cellOccupant = grid.Get(testX, testY);

            if (cellOccupant != PoissonGrid::INVALID_ID) continue;

            if (TestSampleValidity(grid, outputPoints, testCoords, spacing)) {

                currentIndex += 1;

                grid.Set(currentIndex, testX, testY);
                activePoints.emplace_back(currentIndex);

                outputPoints.emplace_back(testCoords);

                foundSample = true;
                break;
            }
        }

        if (!foundSample) {
            activePoints.erase(activePoints.begin() + selectedIndex);
        }
    }

    return outputPoints;
}

}

bee::PropBuilder::PropBuilder()
{
    m_adjustPropPosition =
        std::make_shared<bee::Shader>(bee::FileIO::Directory::Asset, "shaders/prop/prop_filter.comp");

    glGenBuffers(1, &m_positionSSBO);

    int bufferSize = MAX_PROPS_IN_COMPUTE * sizeof(prop_struct);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionSSBO);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT | GL_CLIENT_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_positionSSBO);
    BEE_DEBUG_ONLY(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}

bee::PropBuilder::~PropBuilder()
{
    glDeleteBuffers(1, &m_positionSSBO);
}

//Poisson Disk Helpers

std::vector<glm::mat4> bee::PropBuilder::GeneratePositions(const GenerationParams& params) const
{
    constexpr int reps = 30;
    auto poissonPositions = PoissonDiskSampling(params.terrainSize, params.propSpacing, params.rng, reps);
  
    std::vector<prop_struct> props;
    props.reserve(poissonPositions.size());

    for (auto& position : poissonPositions) {

        prop_struct new_prop;

        new_prop.position = { 
            position.x - params.terrainSize.x / 2,
            position.y - params.terrainSize.y / 2,
            0.0f
        };

        props.emplace_back(new_prop);
    }

    ComputeFilter(props, params.terrainSize, params.heightModifier, params.densityMap, params.heightMap);
    
    std::vector<glm::mat4> transforms;
    transforms.reserve(props.size());

    for (auto& prop : props)
    {
        if (prop.percentDiscard < pcg::rand0_1(params.rng))
            continue;

        glm::mat4 matrix = glm::mat4(1.0f);

        matrix = glm::translate(matrix, prop.position);
        matrix = glm::rotate(matrix, params.matchSlopePercentage * prop.surfaceNormal.x * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        matrix = glm::rotate(matrix, params.matchSlopePercentage * prop.surfaceNormal.y * -1.0f * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        matrix = glm::rotate(matrix, pcg::rand0_1(params.rng) * glm::radians(360.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        float scaleRange = params.maxScale - params.minScale;
        matrix = glm::scale(matrix, glm::vec3(pcg::rand0_1(params.rng) * scaleRange + params.minScale));

        transforms.push_back(matrix);
    }

    for (int i = 0; i < transforms.size(); i += 1)
    {
        int randIndex = static_cast<int>((pcg::rand0_1(params.rng) * transforms.size()) - 0.5f);

        glm::mat4 temp = transforms[i];
        transforms[i] = transforms[randIndex];
        transforms[randIndex] = temp;
    }
   
    return transforms;
}

void bee::PropBuilder::ComputeFilter(std::vector<prop_struct>& propData, const glm::vec2 terrainSize, float heightMod, ResourceHandle<Image> densityMap, ResourceHandle<Image> heightMap) const
{
    m_adjustPropPosition->Activate();

    glm::mat4 terrainMatrix = glm::identity<glm::mat4>();
    terrainMatrix *= glm::translate(glm::identity<glm::mat4>(), glm::vec3{ 0.5f,0.5f, 0.0f }); // Translate by half to center
    terrainMatrix *= glm::scale(glm::identity<glm::mat4>(), 1.0f / glm::vec3{ terrainSize.x, terrainSize.y, 1.0f }); // Scale by terrain size

    m_adjustPropPosition->GetParameter("u_terrainMatrix")->SetValue(terrainMatrix);
    m_adjustPropPosition->GetParameter("u_heightMod")->SetValue(heightMod);

    glUniform1i(m_adjustPropPosition->GetParameter("u_heightMap")->GetLocation(), 0);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, heightMap.Retrieve()->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(m_adjustPropPosition->GetParameter("u_densityMap")->GetLocation(), 1);

    glActiveTexture(GL_TEXTURE1);

    glBindTexture(GL_TEXTURE_2D, densityMap.Retrieve()->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_positionSSBO);

    int remainingProps = static_cast<int>(propData.size());
    size_t offset = 0;

    while (remainingProps > 0)
    {
        int numToUpdate = remainingProps < MAX_PROPS_IN_COMPUTE ? remainingProps : MAX_PROPS_IN_COMPUTE;
        int bufferSize = numToUpdate * sizeof(prop_struct);

        uint32_t numGroups = (static_cast<uint32_t>(numToUpdate) + PROP_GROUP_SIZE - 1) / PROP_GROUP_SIZE;

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, propData.data() + offset);
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, propData.data() + offset);

        remainingProps -= MAX_PROPS_IN_COMPUTE;
        offset += numToUpdate;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
