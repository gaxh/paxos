#include "utils.h"

#include <cstdlib>
#include <map>
#include <set>
#include <vector>

using namespace std;

bool isPromiseValueNull(const Message& m) {
  return m.promise.value.proposalId == -1;
}

bool isAllPromiseNull(const vector<Message>& promises) {
  for (Message m : promises) {
    if (m.type != MessageType::PROMISE) {
      exit(-1);
    }
    if (!isPromiseValueNull(m)) {
      return false;
    }
  }
  return true;
}

// If there are MAJORITY promises with the same value, consensus is reached.
bool isConsensusReached(const vector<Message>& promises, int majority) {
  map<int, int> valueToCount;
  for (const Message& m : promises) {
    if (m.promise.value.proposalId < 0) {
      continue;
    }
    valueToCount[m.promise.value.value]++;
  }

  for (const auto& entry : valueToCount) {
    if (entry.second >= majority) {
      return true;
    }
  }
  return false;
}

Value getHighestAcceptedValue(const vector<Message>& promises) {
  Value tmpValue;
  tmpValue.proposalId = -1;

  for (const Message& m : promises) {
    if (isPromiseValueNull(m)) {
      continue;
    }
    if (tmpValue.proposalId < m.promise.value.proposalId) {
      tmpValue = m.promise.value;
    }
  }
  return tmpValue;
}
