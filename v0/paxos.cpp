#include "paxos.h"

#include "env.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#define DEBUG_ 0

void printMessage(const Message& m) {
  printf("type: %s from: %d ", messageTypeStrings[m.type], m.from);
  if (m.type == MessageType::PREPARE) {
    printf("proposalId: %d\n", m.prepare.proposalId);
  } else if (m.type == MessageType::PROMISE) {
    printf("proposalId: %d value: %d value proposed in %d\n",
      m.promise.proposalId, m.promise.value.value, m.promise.value.proposalId);
  } else {
    printf("value: %d value proposed in %d\n", m.accept.value.value, m.accept.value.proposalId);
  }
}

void Proposer::enqueMessage(int curCycle, const Message& m) {
  if (DEBUG_) {
    printf("cycle %d proposer %d received ", curCycle, processId);
    printMessage(m);
  }
  messages.push(m);
}

int Proposer::getProposalId() {
  return round * PROPOSER_COUNT + processId;
}

void Proposer::run(int curCycle, Env* env) {
  if (proposerState == ProposerState::START) {
    // Add a random delay.
    if (rand() % 5 != 0) {
      return;
    }

    Message out;
    out.type = MessageType::PREPARE;
    out.prepare.proposalId = getProposalId();
    env->sendMessage(out, -1, processId);

    timeoutCycle = curCycle + ROUNDTRIP_CYCLES * TIMEOUT_ROUNDTRIPS;
    proposerState = ProposerState::WAIT;

  } else if (proposerState == ProposerState::WAIT) {

    if (curCycle >= timeoutCycle) {
      proposerState = ProposerState::START;
      round++;
      gotPromiseFrom.clear();
      promises.clear();
      return;
    }

    // Process messages.
    while (!messages.empty()) {
      Message m = messages.front(); messages.pop();
      assert(m.type == MessageType::PROMISE);

      // Message from previous rounds, ignore.
      if (m.promise.proposalId < getProposalId()) {
        continue;
      }

      // Message from future rounds, should not happen.
      if (m.promise.proposalId > getProposalId()) {
        exit(1);
      }

      // Duplicated promise, should ignore.
      if (gotPromiseFrom.find(m.from) != gotPromiseFrom.end()) {
        continue;
      }

      gotPromiseFrom.insert(m.from);
      promises.push_back(m);
    }

    // TODO: wait a few more cycles(but do not timeout) after reaching majority.
    if (promises.size() >= majority) {
      if (isConsensusReached(promises, majority)) {
        finalValue = getHighestAcceptedValue(promises);
        proposerState = ProposerState::FINISHED;
        return;
      }

      // Ready to send the accept command, need to check which value
      // to propose.
      // If all promised value is null, then pick a random value.
      // If only one promised value is not null, use it.
      // If more than one promised value is not null, use the one with the highest
      // proposal id.
      Value pickedValue;
      if (isAllPromiseNull(promises)) {
        pickedValue.value = processId;
        pickedValue.proposalId = getProposalId();
      } else {
        pickedValue.value = getHighestAcceptedValue(promises).value;
        pickedValue.proposalId = getProposalId();
      }

      // A value is picked, now send the accept message.
      // After sending this message, we don't expect any incoming message,
      // so this proposer will just wait until timeout, and go back to
      // the start state, have another round of proposal to see if
      // consensus is reached.
      Message out;
      out.type = MessageType::ACCEPT;
      out.accept.value = pickedValue;
      env->sendMessage(out, -1, processId);

      timeoutCycle = curCycle + ROUNDTRIP_CYCLES * TIMEOUT_ROUNDTRIPS;
      gotPromiseFrom.clear();
      promises.clear();

      if (DEBUG_) {
        printf("proposer %d sending out proposal with value %d and proposalId %d\n",
          processId, out.accept.value.value, out.accept.value.proposalId);
      }
    }
  } else if (proposerState == ProposerState::FINISHED) {
    // Do nothing in this state, consensus reached.
  } else {
    exit(1);
  }
}

Proposer::Proposer() {
  round = 0;
  timeoutCycle = -1;
  proposerState = ProposerState::START;
}

void Acceptor::enqueMessage(int curCycle, const Message& m) {
  if (DEBUG_) {
    printf("cycle %d acceptor %d received ", curCycle, processId);
    printMessage(m);
  }
  messages.push(m);
}

void Acceptor::run(int curCycle, Env* env) {
  while (!messages.empty()) {
    Message m = messages.front(); messages.pop();
    if (m.type == MessageType::PREPARE) {
      if (m.prepare.proposalId > highestProposal) {
        highestProposal = m.prepare.proposalId;

        // Send the promise.
        Message out;
        out.type = MessageType::PROMISE;
        out.promise.proposalId = m.prepare.proposalId;
        out.promise.value = acceptedValue;
        env->sendMessage(out, m.from, processId);
      }
    } else if (m.type == MessageType::ACCEPT) {
      if (m.accept.value.proposalId >= highestProposal) {
        highestProposal = m.accept.value.proposalId;
        acceptedValue = m.accept.value;
        if (DEBUG_) {
          printf("acceptor %d accepting value: %d proposal %d "
            "at cycle %d from proposer: %d\n",
            processId, acceptedValue.value, acceptedValue.proposalId,
            curCycle, m.from);
        }
      } else {
        // Ignore.
      }
    } else {
      exit(1);
    }
  }
}

Acceptor::Acceptor() {
  acceptedValue.proposalId = -1;
  acceptedValue.value = -1;
  highestProposal = -1;
}