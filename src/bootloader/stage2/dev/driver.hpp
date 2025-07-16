#pragma once

/**
 * @file driver.hpp
 * @brief Base interface for all drivers.
 */

/**
 * @brief Abstract base class for hardware or software drivers.
 */
class Driver {
public:

    /**
     * @brief Initialize the driver.
     *
     * This is typically used to perform any hardware or resource
     * initialization required by the driver.
     *
     * @return true on success, false on failure.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown or reset the driver.
     */
    virtual void shutdown() = 0;

    /** Virtual destructor. */
    virtual ~Driver() {}
};