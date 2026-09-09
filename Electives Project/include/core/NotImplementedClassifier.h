#pragma once
#include "interfaces/IClassifier.h"

namespace paddy {

// Placeholder IClassifier. Deliberately NOT a fake Random Forest -- it makes
// no attempt to imitate real model output. It always reports Unknown so
// BatchController's decisions (e.g. whether to start drying) can never
// accidentally act on a bogus prediction.
//
// TBD: replace with the real embedded classifier once a model is trained
// and converted for on-device inference. See docs/ARCHITECTURE.md.
class NotImplementedClassifier : public IClassifier {
 public:
  BatchStatus classify(const BatchFeatures& features) override;
};

} // namespace paddy
