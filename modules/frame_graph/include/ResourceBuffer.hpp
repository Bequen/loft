#pragma once

#include <string>
#include <optional>

#include "Resource.h"

class ResourceBuffer {
public:
	virtual Resource* get_resource(std::string name) = 0;
};
