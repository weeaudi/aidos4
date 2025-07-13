#pragma once

class Driver {
public:

    // Initialize the driver (e.g. hardware init)
    virtual bool initialize() = 0;

    // Reset or shutdown driver
    virtual void shutdown() = 0;

    ~Driver() {}
};