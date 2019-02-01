// This program implements Paxos based on the Paxos made simple paper, and an
// environment which simulates an async network to test Paxos implementation.

// This implementation of Paxos made one design choice that is not explicitly stated
// in the Paxos made simple paper, each value accepted by acceptor has a proposal
// id, refers to the proposal this value was proposed.
// If a proposer receives multiple promise messages with different
// value, it will pick the value with highest proposal id.
//
// There is no learner in this implementation, consensus is learned by the proposer.
//
// The uniqueness of proposal id is achieved by making it
// (round * PROPOSER_COUNT + processId).
//  
// A few possible improvements/future works are:
// 1. Add a NACK message, when an acceptor receives a prepare message with a proposal id
// less than the highest proposal it has received, so the proposal can start another round
// eariler.
// 2. Implement other variations, for example, fast paxos and Byzantine Paxos.
// 3. This program implements a library for leader selection, maybe extend it to a 
// replicated state machine first(as mentioned in the Paxos made simple paper) and
// base on it implement a lock service as described in the Chubby paper.
// 4. Write states to disk and implement a recovery schema, so acceptors can fail(this
// only makes sense after point 3 is done).
// 5. Improve the Env class, use real threads/processes to simulate components in the
// Paxos implementation.
#ifndef PAXOS_H
#define PAXOS_H

#include <queue>
#include <set>
#include <vector>

const int PROPOSER_COUNT = 5;
const int ACCEPTOR_COUNT = 5;

// The value to be agreed on.
struct Value {
  int value;
  // At which proposal this value is proposed.
  int proposalId;
};

struct PrepareMessage {
  int proposalId;
};

struct PromiseMessage {
  // The proposal this message is responding to.
  int proposalId;
  Value value;
};

struct AcceptMessage {
  Value value;
};

// From message type we can know whether the message is from proposer
// or from acceptor.
enum MessageType {PREPARE, PROMISE, ACCEPT};
static const char * messageTypeStrings[] = { "PREPARE", "PROMISE", "ACCEPT" };

struct Message {
  MessageType type;
  union {
    PrepareMessage prepare;
    PromiseMessage promise;
    AcceptMessage accept;
  };

  // Id of the proposer/acceptor that sent this message.
  int from;
};

class Env;

class Proposer {
 private:
  int round;

  enum ProposerState {START, WAIT, FINISHED};
  ProposerState proposerState;

  // Per round data.
  int timeoutCycle;
  ::std::set<int> gotPromiseFrom;
  ::std::queue<Message> messages;
  ::std::vector<Message> promises;

  int majority;

 public:
  int processId;
  Value finalValue;
  void enqueMessage(int, const Message& m);
  int getProposalId();
  void run(int curCycle, Env* env);
  bool finished() { return proposerState == ProposerState::FINISHED; };
  void setMajority(int m) { majority = m; };
  Proposer();
};

class Acceptor {
 private:
  Value acceptedValue;
  int highestProposal;
  ::std::queue<Message> messages;

 public:
  int processId;
  void enqueMessage(int, const Message& m);
  void run(int curCycle, Env* env);
  Acceptor();
};

#endif