#pragma once

#include "RenderContext.hpp"

class Drawable {
public:
	virtual void draw(RenderContext* pContext) = 0;
};
