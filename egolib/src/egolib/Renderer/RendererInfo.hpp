
#pragma once

#include "egolib/Log/_Include.hpp"

namespace Ego {
	
/**
 * @brief Information about the backend of the subsystem.
 */
struct RendererInfo {

private:
	/**
	 * @brief The name of the renderer.
	 */
	std::string renderer;

    /**
	 * @brief The name of the vendor of the renderer.
	 */
	std::string vendor;

	/**
	 * @brief The version of the renderer.
	 */
	std::string version;
	
public:

	/**
	 * @brief Construct this device information.
	 * @param renderer the name of the renderer
	 * @param vendor the name of the vendor of the renderer
	 * @param version the version of the renderer
	 */
    RendererInfo(const std::string& renderer, const std::string& vendor, const std::string& version);
	
public:
	/**
	 * @brief Construct this device information.
	 * @param other the other device information
	 */
    RendererInfo(const RendererInfo& other);
	
	/** 
	 * @brief Assign this device information.
	 * @param other the other device information
	 */
	const RendererInfo& operator=(const RendererInfo& other);
	
public:
    /**
     * @brief
     *  Get the name of the renderer.
     * @return
     *  the name of the renderer
     */
    std::string getRenderer() const;

    /**
     * @brief
     *  Get the name of the vendor of the renderer.
     * @return
     *  the name of the vendor of the renderer
     */
    std::string getVendor() const;

    /**
     * @brief
     *  Get the version of the renderer.
     * @return
     *  the version of the renderer
     */
    std::string getVersion() const;

}; // struct RendererInfo

#if 0
/**
 * @brief Write a device information to an output stream.
 * @param target the target, the output stream
 * @param source the source, the device information
 * @return the target
 */
std::ostream& operator<<(std::ostream& target, const RendererInfo& source);
#endif
} // namespace Ego
