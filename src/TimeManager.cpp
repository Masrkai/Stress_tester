#include "../include/TimeManager.hpp"

// Static member definitions
TimeManager* TimeManager::instance = nullptr;
std::mutex TimeManager::instanceMutex;