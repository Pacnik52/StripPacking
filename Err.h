#pragma once
#include <string>
#include <iostream>

// A minimal error handling header to satisfy dependencies.
inline void ERROR(const std::string& msg) {
    std::cerr << "ERROR: " << msg << std::endl;
    exit(1);
}
