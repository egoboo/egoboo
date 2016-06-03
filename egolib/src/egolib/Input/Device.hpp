#pragma once

#include "egolib/platform.h"

struct InputSystem;

/**
 * @brief A device can be connected or not connected to a machine.
 * The input system tracks the connectivity state of devices.
 * If a device is connected, then the input system will open it.
 * If an open device is disconnected, then it will also be closed.
 * An open device receives input from the backend.
 * Lastly, a device can be marked as surpessed: A surpressed device
 * will receive input but it should be ignored by the clients.
 */
struct Device {
    /**
     * @brief The different kinds of devices.
     */
    enum class Kind {
        /**
         * @brief A joystick.
         */
        Joystick,
        /**
         * @brief A keyboard.
         */
        Keyboard,
        /**
         * @brief A mouse.
         */
        Mouse,
    };
    /**
     * @brief Is this device enabled?
     */
    bool enabled;
    /**
     * @brief The kind of this device.
     */
    Kind kind;

protected:
    /**
     * @brief Construct this device.
     * @param kind the kind of this device
     */
    Device(Kind kind) : kind(kind), enabled(false), isConnected(false) {}

public:
    /**
     * @brief Destruct this device.
     */
    virtual ~Device() {}

protected:
    /**
     * @brief Update this device.
     */
    virtual void update() = 0;

private:
    bool isConnected;

public:
    /**
     * @brief Set if this device is connected.
     * @param isConnected @a true if the device is connected, @a false otherwise
     */
    void setConnected(bool isConnected) {
        this->isConnected = isConnected;
    }

    /**
     * @brief Get if this device is connected.
     * @return @a true if the device is connected in, @a false otherwise
     */
    bool getConnected() const {
        return this->isConnected;
    }
protected:
    friend struct InputSystem;
    /**
     * @brief Try to open this device.
     * @pre The device must be connected.
     * @remark If the device is already opened, a call to this method is a no-op.
     */
    virtual void open() = 0;
    /**
     * @brief Ensure that the device is closed.
     */
    virtual void close() = 0;
public:
    /**
     * @brief Get if this device is open.
     * @return @a true if this device is open, @a false otherwise
     */
    virtual bool getOpen() const = 0;
};
