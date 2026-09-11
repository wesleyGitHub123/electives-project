#pragma once

namespace paddy {

// Gates when a user session starts (e.g. a slide switch the operator flips
// to begin a sensing session). Business logic only reads the level -- it
// never touches GPIO directly.
class IStartTrigger {
 public:
  virtual ~IStartTrigger() = default;

  virtual bool begin() = 0;

  // True while the operator is requesting a session to start/continue.
  virtual bool isSessionRequested() const = 0;
};

} // namespace paddy
