#pragma once
#include <code_utils/bee_utils.hpp>
#include <cstdint>
#include <menu_interface.hpp>

namespace bee {

class MainViewport : public IMenu {
public:

	MainViewport();
	~MainViewport() = default;

	virtual void Show(BlossomGame& game) override;

	NON_COPYABLE(MainViewport);
	NON_MOVABLE(MainViewport);

private:

	uint32_t renderer_colour_buffer{};
};

}