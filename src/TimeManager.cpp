#include "../include/TimeManager.h"

// Static member definitions
TimeManager* TimeManager::instance = nullptr;
std::mutex TimeManager::instanceMutex;