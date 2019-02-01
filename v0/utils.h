#ifndef UTILS_H
#define UTILS_H

#include "paxos.h"

#include <vector>

int getMajority();
bool isAllPromiseNull(const ::std::vector<Message>& promises);
bool isConsensusReached(const ::std::vector<Message>& promises, int majority);
Value getHighestAcceptedValue(const ::std::vector<Message>& promises);

#endif