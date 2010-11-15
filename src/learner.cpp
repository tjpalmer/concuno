#include <Eigen/Dense>
#include "learner.h"
#include <sstream>

using namespace Eigen;
using namespace std;

namespace cuncuno {

void BoolLearner::learn(const vector<BoolSample>& samples) {

  // Load
  vector<bool> labels;
  for (
    vector<BoolSample>::const_iterator s(samples.begin());
    s != samples.end();
    s++
  ) {
    const BoolSample& sample(*s);
    for (size_t e(0); e < sample.entities.size(); e++) {
      labels.push_back(sample.value);
    }
  }

  // TODO I need instantiation nodes and a more formed tree notion even to get
  // TODO this far legitimately.

  // Load up all the attribute values for each attribute.
  // TODO This handles individual values. N-tuples (and pairs) should recurse.
  // TODO That is, pairs need done per sample bag, not the whole list.
  Matrix<Float,Dynamic,2> values2D(MatrixXd::Zero(labels.size(),2));
  // TODO Figure out how to get data pointers from Eigen.
  // TODO Just one buffer large enough for all dimensions?
  Float buffer2D[2];
  for (
    vector<FloatAttribute*>::iterator a(schema.floatAttributes.begin());
    a != schema.floatAttributes.end();
    a++
  ) {
    FloatAttribute& attribute(**a);
    size_t index(0);
    if (attribute.count() == 2) {
      for (
        vector<BoolSample>::const_iterator s(samples.begin());
        s != samples.end();
        s++
      ) {
        const BoolSample& sample(*s);
        for (
          vector<const Entity*>::const_iterator e(sample.entities.begin());
          e != sample.entities.end();
          e++, index++
        ) {
          attribute.get(*e, buffer2D);
          // TODO There's probably some better way than two assignments.
          values2D(index,0) = buffer2D[0];
          values2D(index,1) = buffer2D[1];
        }
      }
      // A bit of logging status. TODO Are there easier ways than all this?
      stringstream message;
      string name;
      attribute.name(name);
      //cout << values2D << endl;
      message << name << " values loaded: " << values2D.rows() << endl;
      log(message.str());
    }
  }

}

template<typename Value>
Learner<Value>::Learner(): Worker("Learner") {}

template<>
Learner<bool>::Learner(): Worker("Learner<bool>") {}

}
