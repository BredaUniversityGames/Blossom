#pragma once

#include "resources/resource_handle.hpp"

namespace bee
{

class WindMap
{
public:
	WindMap();
	~WindMap();

	struct WindAmbient
    {
        float direction;
        float strength;
        float speed;
    };

	ResourceHandle<Image3D> GetWindImage();
	WindAmbient GetWindParameters();

private:
	class Impl;
    std::unique_ptr<Impl> m_impl;

	WindAmbient m_ambientWind;
	ResourceHandle<Image3D> m_wind;
};


}