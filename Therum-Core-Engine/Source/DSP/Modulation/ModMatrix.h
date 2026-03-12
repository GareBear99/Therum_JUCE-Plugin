#pragma once
#include <vector>
#include "ModSourceTypes.h"

struct ModRoute
{
    ModSourceType sourceType {};
    int sourceIndex = 0;
    int targetId = 0;
    float amount = 0.0f;
    bool bipolar = false;
};

class ModMatrix
{
public:
    void addRoute(ModRoute route) { routes.push_back(route); }
    const std::vector<ModRoute>& getRoutes() const { return routes; }

private:
    std::vector<ModRoute> routes;
};
