// file: ContentRegistryIO.h
#pragma once

#include <string>

class ContentRegistry;

namespace ContentRegistryIO
{
void load(ContentRegistry& reg, std::string_view path);
void save(const ContentRegistry& reg, std::string_view path);
} // namespace ContentRegistryIO
