#include <unity.h>
#include "core/BatchController.h"
#include "../mocks/MockMoistureSensorArray.h"
#include "../mocks/MockTemperatureSensor.h"
#include "../mocks/MockDisplay.h"
#include "../mocks/MockStatusIndicator.h"
#include "../mocks/MockDryingActuator.h"
#include "../mocks/MockClassifier.h"
#include "../mocks/MockClock.h"
#include "../mocks/MockStartTrigger.h"

using namespace paddy;
using namespace paddy_test;

void setUp() {}
void tearDown() {}

namespace {

struct Fixture {
  MockMoistureSensorArray moisture;
  MockTemperatureSensor temperature;
  MockDisplay display;
  MockStatusIndicator statusIndicator;
  MockDryingActuator dryingActuator;
  MockClassifier classifier;
  MockClock clock;
  MockStartTrigger startTrigger;
  BatchController controller;

  explicit Fixture(BatchController::Config cfg = BatchController::Config{})
      : controller(moisture, temperature, display, statusIndicator,
                    dryingActuator, classifier, clock, startTrigger, cfg) {}
};

} // namespace

void test_begin_enters_idle_when_all_adapters_succeed() {
  Fixture f;
  f.controller.begin();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));
}

void test_begin_enters_fault_when_an_adapter_fails() {
  Fixture f;
  f.display.beginResult = false;
  f.controller.begin();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Fault), static_cast<int>(f.controller.state()));
}

void test_full_cycle_without_drying_returns_to_idle() {
  Fixture f;
  f.classifier.nextResult = BatchStatus::StoreSafely;
  f.controller.begin();

  f.controller.update(); // Idle -> AcquiringBatch
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::AcquiringBatch), static_cast<int>(f.controller.state()));

  f.controller.update(); // AcquiringBatch -> ExtractingFeatures
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ExtractingFeatures), static_cast<int>(f.controller.state()));

  f.controller.update(); // ExtractingFeatures -> Classifying
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Classifying), static_cast<int>(f.controller.state()));

  f.controller.update(); // Classifying -> ReportingResult
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ReportingResult), static_cast<int>(f.controller.state()));
  TEST_ASSERT_EQUAL(static_cast<int>(BatchStatus::StoreSafely), static_cast<int>(f.controller.lastResult()));

  f.controller.update(); // ReportingResult -> Idle (StoreSafely => no drying)
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));
  TEST_ASSERT_EQUAL(0, f.dryingActuator.startCallCount);
}

void test_dry_more_result_triggers_drying_then_completes_on_safe_condition() {
  Fixture f;
  f.classifier.nextResult = BatchStatus::DryMore;
  f.controller.begin();

  for (int i = 0; i < 4; ++i) f.controller.update(); // Idle -> ... -> ReportingResult
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ReportingResult), static_cast<int>(f.controller.state()));

  f.controller.update(); // ReportingResult -> DryingActive (state set; handler not run yet)
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::DryingActive), static_cast<int>(f.controller.state()));

  f.controller.update(); // handleDryingActive: starts actuator, immediately -> DryingReSampling
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::DryingReSampling), static_cast<int>(f.controller.state()));
  TEST_ASSERT_EQUAL(1, f.dryingActuator.startCallCount);
  TEST_ASSERT_TRUE(f.dryingActuator.isRunning());

  // Still not safe (classifier still returns DryMore) -> back to DryingActive
  f.controller.update();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::DryingActive), static_cast<int>(f.controller.state()));
  TEST_ASSERT_TRUE(f.dryingActuator.isRunning());

  // Now the batch reports safe -> the next resample completes drying
  f.classifier.nextResult = BatchStatus::StoreSafely;
  f.controller.update(); // DryingActive -> DryingReSampling (start not called again)
  TEST_ASSERT_EQUAL(1, f.dryingActuator.startCallCount);

  f.controller.update(); // DryingReSampling -> DryingComplete
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::DryingComplete), static_cast<int>(f.controller.state()));

  f.controller.update(); // DryingComplete -> Idle, stop actuator
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));
  TEST_ASSERT_EQUAL(1, f.dryingActuator.stopCallCount);
  TEST_ASSERT_FALSE(f.dryingActuator.isRunning());
}

void test_result_hold_delays_transition_out_of_reporting_result() {
  BatchController::Config cfg;
  cfg.resultDisplayHoldMs = 100;
  Fixture f(cfg);
  f.classifier.nextResult = BatchStatus::StoreSafely;
  f.controller.begin();

  for (int i = 0; i < 4; ++i) f.controller.update(); // -> ReportingResult
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ReportingResult), static_cast<int>(f.controller.state()));

  f.controller.update(); // not enough time elapsed yet
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ReportingResult), static_cast<int>(f.controller.state()));

  f.clock.advance(100);
  f.controller.update();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));
}

void test_idle_stays_in_idle_while_session_not_requested() {
  Fixture f;
  f.startTrigger.sessionRequested = false;
  f.controller.begin();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));

  f.controller.update();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));
  TEST_ASSERT_EQUAL(0, f.dryingActuator.startCallCount);
}

void test_idle_transitions_to_acquiring_once_session_requested() {
  Fixture f;
  f.startTrigger.sessionRequested = false;
  f.controller.begin();

  f.controller.update();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));

  f.startTrigger.sessionRequested = true;
  f.controller.update();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::AcquiringBatch), static_cast<int>(f.controller.state()));
}

void test_switch_off_mid_session_does_not_abort_in_progress_batch() {
  Fixture f;
  f.classifier.nextResult = BatchStatus::StoreSafely;
  f.controller.begin();

  f.controller.update(); // Idle -> AcquiringBatch (session requested)
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::AcquiringBatch), static_cast<int>(f.controller.state()));

  // Operator flips the switch off while a batch is in flight...
  f.startTrigger.sessionRequested = false;
  f.controller.update(); // AcquiringBatch -> ExtractingFeatures (not aborted)
  f.controller.update(); // -> Classifying
  f.controller.update(); // -> ReportingResult
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ReportingResult), static_cast<int>(f.controller.state()));
  f.controller.update(); // -> back to Idle (gate takes effect again)
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));

  // ...and the next update() stays in Idle because the switch is still off.
  f.controller.update();
  TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(f.controller.state()));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_begin_enters_idle_when_all_adapters_succeed);
  RUN_TEST(test_begin_enters_fault_when_an_adapter_fails);
  RUN_TEST(test_full_cycle_without_drying_returns_to_idle);
  RUN_TEST(test_dry_more_result_triggers_drying_then_completes_on_safe_condition);
  RUN_TEST(test_result_hold_delays_transition_out_of_reporting_result);
  RUN_TEST(test_idle_stays_in_idle_while_session_not_requested);
  RUN_TEST(test_idle_transitions_to_acquiring_once_session_requested);
  RUN_TEST(test_switch_off_mid_session_does_not_abort_in_progress_batch);
  return UNITY_END();
}
