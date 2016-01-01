#include "egolib/Renderer/RendererInfo.hpp"

namespace Ego {

RendererInfo::RendererInfo(const std::string& renderer, const std::string& vendor, const std::string& version)
	: renderer(renderer), vendor(vendor), version(version) {
}

RendererInfo::RendererInfo(const RendererInfo& other)
    : renderer(other.getRenderer()), vendor(other.getVendor()), version(other.getVersion()) {
}

const RendererInfo& RendererInfo::operator=(const RendererInfo& other) {
    renderer = other.getRenderer();
    vendor = other.getVendor();
    version = other.getVersion();
    return *this;
}

std::string RendererInfo::getRenderer() const {
	return renderer;
}

std::string RendererInfo::getVendor() const {
	return vendor;
}

std::string RendererInfo::getVersion() const {
	return version;
}

#if 0
std::ostream& operator<<(std::ostream& target, const RendererInfo& source) {
	target << "renderer = " << source.getRenderer() << std::endl;
	target << "vendor = " << source.getVendor() << std::endl;
	target << "version = " << source.getVersion() << std::endl;
	return target;
}
#endif

} // namespace Ego
