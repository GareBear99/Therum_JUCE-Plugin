#include "ModMatrixEvaluator.h"
#include <algorithm>

float ModMatrixEvaluator::applyMacroToTarget(const ModMatrix& matrix, int targetId, float baseValue, float macro1, float macro2) const
{
    float out = baseValue;

    for (const auto& r : matrix.getRoutes())
    {
        if (r.targetId != targetId)
            continue;

        float src = 0.0f;
        if (r.sourceType == ModSourceType::Macro && r.sourceIndex == 0) src = macro1;
        if (r.sourceType == ModSourceType::Macro && r.sourceIndex == 1) src = macro2;

        if (r.bipolar)
            src = src * 2.0f - 1.0f;

        out += src * r.amount;
    }

    return std::clamp(out, 0.0f, 20000.0f);
}
