#include <unity.h>
#include "core/FeatureExtractor.h"

using namespace paddy;

void setUp() {}
void tearDown() {}

namespace {

MoistureReading valid(float v) { return MoistureReading{v, true}; }
MoistureReading invalidReading() { return MoistureReading{0.0f, false}; }

} // namespace

void test_extract_returns_invalid_when_no_valid_moisture_readings() {
  BatchSample sample{};
  for (auto& r : sample.moisture) r = invalidReading();
  sample.temperature[0] = TemperatureReading{25.0f, true};

  BatchFeatures features = FeatureExtractor::extract(sample);

  TEST_ASSERT_FALSE(features.valid);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, features.meanMoisture);
}

void test_extract_computes_mean_of_valid_readings_only() {
  BatchSample sample{};
  sample.moisture[0] = valid(10.0f);
  sample.moisture[1] = valid(20.0f);
  sample.moisture[2] = invalidReading();
  sample.moisture[3] = valid(30.0f);
  sample.moisture[4] = valid(40.0f);
  sample.temperature[0] = TemperatureReading{28.0f, true};

  BatchFeatures features = FeatureExtractor::extract(sample);

  TEST_ASSERT_TRUE(features.valid);
  TEST_ASSERT_EQUAL_FLOAT(25.0f, features.meanMoisture); // (10+20+30+40)/4
  TEST_ASSERT_EQUAL_FLOAT(28.0f, features.temperatureCelsius);
}

void test_extract_computes_zero_variability_for_uniform_readings() {
  BatchSample sample{};
  for (auto& r : sample.moisture) r = valid(15.0f);
  sample.temperature[0] = TemperatureReading{26.0f, true};

  BatchFeatures features = FeatureExtractor::extract(sample);

  TEST_ASSERT_EQUAL_FLOAT(0.0f, features.moistureVariability);
}

void test_extract_invalid_when_temperature_missing() {
  BatchSample sample{};
  for (auto& r : sample.moisture) r = valid(15.0f);
  sample.temperature[0] = TemperatureReading{0.0f, false};

  BatchFeatures features = FeatureExtractor::extract(sample);

  TEST_ASSERT_FALSE(features.valid);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_extract_returns_invalid_when_no_valid_moisture_readings);
  RUN_TEST(test_extract_computes_mean_of_valid_readings_only);
  RUN_TEST(test_extract_computes_zero_variability_for_uniform_readings);
  RUN_TEST(test_extract_invalid_when_temperature_missing);
  return UNITY_END();
}
