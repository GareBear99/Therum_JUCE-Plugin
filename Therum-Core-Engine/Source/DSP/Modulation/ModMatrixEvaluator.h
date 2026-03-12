#pragma once
#include "ModMatrix.h"

class ModMatrixEvaluator
{
public:
    float applyMacroToTarget(const ModMatrix& matrix, int targetId, float baseValue, float macro1, float macro2) const;
};
