#pragma once

namespace paddy {

// Result of classifying one paddy batch.
enum class BatchStatus {
  Unknown,      // classifier not implemented yet, or input data insufficient
  StoreSafely,
  DryMore,
  HighRisk,
};

} // namespace paddy
