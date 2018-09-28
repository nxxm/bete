#pragma once

struct benchmark {
  std::string name{};
  size_t min{};
  size_t max{};
  size_t avg{};
};

struct datamodel_t {
  std::vector<benchmark> benchmarks;
};

BELLE_VUE_OBSERVABLE(benchmark, name, min, max, avg);
BELLE_VUE_OBSERVABLE(datamodel_t, benchmarks);
