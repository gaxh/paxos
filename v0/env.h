#ifndef ENV_H
#define ENV_H

#include "paxos.h"

#include <queue>
#include <vector>

#define ROUNDTRIP_CYCLES 100
#define TIMEOUT_ROUNDTRIPS 6

class Env {
 private:
  struct Package {
    int readyCycle;
    Message message;
    int to;

    bool operator < (const Package& o) const {
      return readyCycle > o.readyCycle;
    }

    int getReadyCycle() const {
      return readyCycle;
    }

    Package(int to, const Message& message) : to(to), message(message) {}
  };

  // Distribute the ready packages.
  void distributeMessages(int);

  // From in packages to out packages.
  // Add duplication, drop, delay.
  void processPackages(int);

  int getMajority() { return ACCEPTOR_COUNT / 2 + 1; };

  Proposer proposers[PROPOSER_COUNT];
  Acceptor acceptors[ACCEPTOR_COUNT];

  // Packages into the network.
  ::std::vector<Package> inPackages;
  // Packages out of the network.
  ::std::priority_queue<Package> outPackages;

 public:
  void init();
  void sendMessage(Message, int, int);
  void run();

  Env() {};
};

#endif