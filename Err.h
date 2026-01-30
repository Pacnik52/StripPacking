#pragma once
#include <string>
#include <iostream>

inline void ERROR(const std::string &msg) {
    std::cerr << "ERROR: " << msg << std::endl;
    exit(1);
}
