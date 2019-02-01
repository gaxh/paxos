#include "env.h"

#include "paxos.h"
#include "utils.h"

#include <vector>

// (1/DROP_PROB) of the packages will be droped, from example, 5 means 20%
#define DROP_PROB 3
#define DUP_PROB 5

using namespace std;

void Env::distributeMessages(int curCycle) {
  while (!outPackages.empty()
      && outPackages.top().getReadyCycle() <= curCycle) {
    const Package& p = outPackages.top();
    if (p.message.type == MessageType::PROMISE) {
      proposers[p.to].enqueMessage(curCycle, p.message);
    } else {
      acceptors[p.to].enqueMessage(curCycle, p.message);
    }
    outPackages.pop();
  }
  return;
}

void Env::processPackages(int curCycle) {
  vector<Package> tmp;
  for (const Package& p : inPackages) {
    if (rand() % DUP_PROB == 0) {
      tmp.push_back(p);
    }
  }
  for (const Package& p : tmp) {
    inPackages.push_back(p);
  }

  for (auto it = inPackages.begin(); it != inPackages.end();) {
    if (rand() % DROP_PROB == 0) {
      it = inPackages.erase(it);
    } else {
      ++it;
    }
  }

  // Sets ready cycle randomly.
  for (Package& p : inPackages) {
    p.readyCycle = curCycle
      + (rand() % (ROUNDTRIP_CYCLES) + ROUNDTRIP_CYCLES / 2)
      + ((rand() % 5) == 0 ? ROUNDTRIP_CYCLES * 10 : 0);
  }

  // Pushes packages to outPackages.
  for (const Package& p : inPackages) {
    outPackages.push(p);
  }

  inPackages.clear();
}

// Process the target address(may duplicate the message).
// If "to" equals to -1 then this message is to be broadcasted.
void Env::sendMessage(Message m, int to, int from) {
  m.from = from;

  if (to == -1) {
    // This must be proposer to acceptor.
    for (int i = 0; i < ACCEPTOR_COUNT; i++) {
      inPackages.push_back(Package(i, m));
    }
  } else {
    // This must be acceptor to proposer.
    inPackages.push_back(Package(to, m));
  }
  return;
}

void Env::init() {
  for (int i = 0; i < PROPOSER_COUNT; i++) {
    proposers[i].processId = i;
    proposers[i].setMajority(getMajority());
  }
  for (int i = 0; i < ACCEPTOR_COUNT; i++) {
    acceptors[i].processId = i;
  }
}

void Env::run() {
  int cycle = 0;
  while (true) {
    ++cycle;

    // Process packages from previous cycle.
    processPackages(cycle);

    // Distribute the messages that are ready in the current cycle.
    distributeMessages(cycle);

    // Call run on all proposers and acceptors.
    for (int i = 0; i < PROPOSER_COUNT; i++) {
      proposers[i].run(cycle, this);
    }
    for (int i = 0; i < ACCEPTOR_COUNT; i++) {
      acceptors[i].run(cycle, this);
    }

    // Stop condition.
    bool allProposerEnded = true;
    for (int i = 0; i < PROPOSER_COUNT; i++) {
      if (!proposers[i].finished()) {
        allProposerEnded = false;
        break;
      }
    }

    if (allProposerEnded) {
      printf("Leader selection finished in cycle: %d, the final value: %d\n",
        cycle, proposers[0].finalValue.value);
      break;
    }
  }
}