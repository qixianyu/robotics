/*
 * MarkovChain.h
 *
 *  Copyright (c) 2015, Norman Alan Oursland
 *  All rights reserved.
 */

#ifndef MATH_PROBABILITY_DISCRETE_MARKOVCHAIN_H_
#define MATH_PROBABILITY_DISCRETE_MARKOVCHAIN_H_

#include "cognitoware/math/probability/discrete/DiscreteConditional.h"
#include "cognitoware/math/probability/discrete/DistributionValueMap.h"
#include "cognitoware/math/probability/RandomConditional.h"
#include "cognitoware/math/probability/RandomDistribution.h"

#include <memory>

namespace cognitoware {
namespace math {
namespace probability {
namespace discrete {

// A Markov Chain is a conditional probability distribution that provides the
// probability of transitioning from a start x to an end x. This class has the
// "Markov Property", which means the prediction about the next x only depends
// on the current x and no other states before it. States that have this
// property are called "Markov States". The Markov chain can be represented as
// the conditional probability P(X<sub>t+1</sub> | X<sub>t</sub>).
template<typename X>
class MarkovChain : public DiscreteConditional<X, X> {
public:
  MarkovChain() {
  }

  ~MarkovChain() override {
  }

  void Set(X end, X start, double p) {
    auto end_map = transitions_.find(start);
    if (end_map == transitions_.end()) {
      transitions_[start] = std::map<X, double>();
      end_map = transitions_.find(start);
    }
    end_map->second[end] = p;
    range_[end] = end;
  }

  X DoTransition(X start, std::default_random_engine* generator) {
    std::uniform_real_distribution<double> dist(0, 1);
    double select = dist(*generator);
    X result = start;
    auto end_map = transitions_.find(start);
    if (end_map != transitions_.end()) {
      for (auto& x : end_map->second) {
        select -= x.second;
        if (select <= 0) {
          result = x.first;
          break;
        }
      }
    }
    return result;
  }

  std::shared_ptr<MarkovChain> Reverse() {
    auto result = std::make_shared<MarkovChain<X>>();
    for (X today : range()) {
      auto x = this->BayesianInference(today, DistributionValueMap<X>(domain()));
      for (X yesterday : domain()) {
        double p = x->ProbabilityOf(yesterday);
        result->Set(yesterday, today, p);
      }
    }
    return result;
  }

  std::vector<X> domain() {
    std::vector<X> result;
    result.reserve(range_.size());
    for (auto& pair : range_) {
      result.push_back(pair.first);
    }
    return result;
  }

  std::vector<X> range() {
    std::vector<X> result;
    result.reserve(transitions_.size());
    for (auto& pair : transitions_) {
      result.push_back(pair.first);
    }
    return result;
  }

  double ConditionalProbabilityOf(const X& end, const X& start) const override {
    auto end_map = transitions_.find(start);
    if (end_map == transitions_.end()) {
      return 0.0;
    }
    auto result = end_map->second.find(end);
    if (result == end_map->second.end()) {
      return 0.0;
    }
    return result->second;
  }

  std::shared_ptr<DistributionValueMap<X>> LikelihoodOf(X data) {
    auto result = std::make_shared<DistributionValueMap<X>>();
    for (auto& end_map : transitions_) {
      auto end = end_map.second.find(data);
      if (end != end_map.second.end()) {
        result->Set(end_map.first, end->second);
      }
    }
    return result;
  }

  std::shared_ptr<DistributionValueMap<X>> Marginalize(
      const RandomDistribution<X>& start) {
    auto result = std::make_shared<DistributionValueMap<X>>();
    for (auto& xfr_map : transitions_) {
      double pstart = start.ProbabilityOf(xfr_map.first);
      for (auto& end : xfr_map.second) {
        double pend = end.second;
        double p0 = result->ProbabilityOf(end.first);
        double pxfr = pend * pstart;
        double p1 = p0 + pxfr;
        result->Set(end.first, p1);
      }
    }
    result->Normalize();
    return result;
  }

  std::shared_ptr<DistributionValueMap<X>> BayesianInference(
      X data, const DistributionValueMap<X>& prior) {
    auto likelihood = LikelihoodOf(data);
    return likelihood->Product(prior);
  }

private:
  std::map<X, std::map<X, double>> transitions_;
  std::map<X, X> range_;
};

}  // namespace discrete
}  // namespace probability
}  // namespace math
}  // namespace cognitoware

#endif /* MATH_PROBABILITY_DISCRETE_MARKOVCHAIN_H_ */
