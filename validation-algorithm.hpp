// validation-algorithm.hpp: Masaki Hara
#ifndef VALIDATION_ALGORITHM_HPP
#define VALIDATION_ALGORITHM_HPP
#include "validation.hpp"
#include <algorithm>
#include <vector>
#include <queue>

namespace validation {
  using namespace std;
  template<typename Iterator>
  bool isUnique(Iterator begin, Iterator end) {
    vector<__typeof(*begin)> v(end-begin);
    copy(begin, end, v.begin());
    sort(v.begin(), v.end());
    return unique(v.begin(), v.end())==v.end();
  }
}

#endif

