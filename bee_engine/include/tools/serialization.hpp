#pragma once

#include <code_utils/bee_utils.hpp>

#include <visit_struct/visit_struct.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include <istream>
#include <ostream>

namespace bee
{

namespace detail {

//Helper for else branch in constexpr
template <typename T>
struct False : std::bool_constant<false> { };

} //namespace detail

// Used to create JSON sections with cereal
struct Section {
public:
	Section(std::function<void(void)> archive)
		: m_invoke(archive) {}

	template<typename A>
	void serialize(A& ar) {
		m_invoke();
	}

private:
	std::function<void(void)> m_invoke;
};



/// <summary>
/// Visitor pattern for recursive serialization of reflected classes
/// </summary>
template<typename A>
class SaveVisitor {
public:

	SaveVisitor(std::ostream& stream) : m_archive(stream) {}

	//Named Objects (from visitstruct or user defined)
	template<typename T>
	void operator()(const char* name, const T& val) {

		if constexpr (cereal::traits::is_output_serializable<T, A>::value) {
			m_archive(cereal::make_nvp(name, val));
		}
		else if constexpr (visit_struct::traits::is_visitable<T>::value) {
			visit_struct::for_each(val, *this);
		}
		else {
			static_assert(detail::False<T>::value, "A class can only be saved if it has cereal save functions or is visitable");
		}

	}

	//Unnamed Objects
	template<typename T>
	void operator()(const T& val) {
		this->operator()(typeid(T).name(), val);
	}

	//Entities
	void operator()(entt::entity e) {
		m_archive(cereal::make_nvp("Entity", e));
	}

	//Size
	void operator()(std::underlying_type_t<entt::entity> s) {
		m_archive(cereal::make_nvp("Count", s));
	}

	//Sections
	void operator()(const char* name, Section&& s) {
		Section moved = std::move(s);
		m_archive(cereal::make_nvp(name, moved));
	}

private:
	A m_archive;
};

/// <summary>
/// Visitor pattern for recursive serialization of reflected classes
/// </summary>
template<typename A>
class LoadVisitor {
public:

	LoadVisitor(std::istream& stream) : m_archive(stream) {}

	//General objects
	template<typename T>
	void operator()(const char* name, T& val) {

		if constexpr (cereal::traits::is_input_serializable<T, A>::value) {
			m_archive(cereal::make_nvp(name, val));
		}
		else if constexpr (visit_struct::traits::is_visitable<T>::value) {
			visit_struct::for_each(val, *this);
		}
		else {
			static_assert(detail::False<T>::value, "A class can only be loaded if it has cereal load functions or is visitable");
		}
	}

	//Unnamed Objects
	template<typename T>
	void operator()(T& val) {
		this->operator()(typeid(T).name(), val);
	}

	//Entities
	void operator()(entt::entity& e) {
		m_archive(cereal::make_nvp("Entity", e));
	}

	//Size
	void operator()(std::underlying_type_t<entt::entity>& s) {
		m_archive(cereal::make_nvp("Count", s));
	}

	//Sections
	void operator()(const char* name, Section&& s) { 
		Section moved = std::move(s);
		m_archive(cereal::make_nvp(name, moved));
	}

private:
	A m_archive;
};

using JSONSaver = SaveVisitor<cereal::JSONOutputArchive>;
using JSONLoader = LoadVisitor<cereal::JSONInputArchive>;

}  // namespace Osm
