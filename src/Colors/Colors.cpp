// file: Colors.cpp
// Color pair initialization has moved to Renderer::init_color_pairs()
// This file is kept for backward compatibility with code that instantiates Colors.
#include "Colors.h"

void Colors::my_init_pair() noexcept
{
    // No-op: color pairs are now managed by Renderer::init_color_pairs()
}
