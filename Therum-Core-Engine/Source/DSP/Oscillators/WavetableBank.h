#pragma once
#include <vector>

class WavetableBank
{
public:
    void buildDefaultSineTable(int size = 2048);
    const std::vector<float>& getTable() const { return table; }
    bool isReady() const { return !table.empty(); }

private:
    std::vector<float> table;
};
