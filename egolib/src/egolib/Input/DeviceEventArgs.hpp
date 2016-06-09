#pragma

namespace Ego {
namespace Input {

struct DeviceEventArgs {
    enum class Kind {
        /**
         * @brief A device was added.
         * Canonical example is pluggin in a controller.
         */
        Added,
        /**
         * @brief A device was removed.
         * Canonical example is pluggin out a controller.
         */
        Removed,
    };
    /**
     * @brief The kind of a controller.
     */
    Kind kind;
}; // struct DeviceEventArgs
	
} // namespace Input
} // namespace Ego
