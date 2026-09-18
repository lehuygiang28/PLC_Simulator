/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef COMM_PROTOCOL_PLCACCESS_H
#define COMM_PROTOCOL_PLCACCESS_H

#include "Core/DeviceAddress.h"
#include <cstdint>
#include <vector>

enum class PlcUnit { Word, Bit };

struct PlcAccess {
    DeviceKind device = DeviceKind::D;
    PlcUnit unit = PlcUnit::Word;
    int start = 0;
    int count = 0;
    std::vector<int16_t> wordData;
    std::vector<uint8_t> bitData;
};

#endif
