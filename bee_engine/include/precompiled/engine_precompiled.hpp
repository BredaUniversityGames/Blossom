#pragma once

// Engine precompiled header, containing commonly used files and defines
// Contains standard library, glm and macro shortcuts
// Avoid changing at all costs to not force full recompiles

//Standard Library

#include <cstdlib>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <optional>
#include <map>
#include <unordered_set>
#include <set>
#include <future>
#include <thread>
#include <fstream>
#include <string_view>
#include <deque>
#include <queue>
#include <stack>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <random>

//GLM
//Note: Do not place glm/ext.hpp here, makes builds much slower

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

//VisitStruct (Reflection)

#include <visit_struct/visit_struct.hpp>

//FMT

//Using header only mode because source uses modules (Not C++ 17 compliant)
#define FMT_HEADER_ONLY
#include <fmt/include/fmt/core.h>

//Cereal (Serialization)

#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/cereal.hpp>

//Macros

#include <code_utils/bee_utils.hpp>

//ECS

#include <entt/entity/registry.hpp>

//Operating system 

#if defined(BEE_PLATFORM_PC)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#endif