#pragma once

// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.
#include "stdafx.h"
#include <cmath>
#include <array>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>
//#include "HighResTimer.h"

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>


//#include <mutex>
//std::mutex global_mutex;


unsigned int elapsed();

int LogMyoArmband(std::string file_name);