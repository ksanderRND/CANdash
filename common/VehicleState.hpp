#pragma once

struct VehicleState {
    double speed;
    double rpm;
    double coolantTemp;
    double soc;
};

constexpr VehicleState DEFSTATE {60.0, 2000.0, 87.0, 80.0};