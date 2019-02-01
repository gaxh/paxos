# paxos

This program implements Paxos based on the Paxos made simple paper, and an
environment which simulates an async network to test Paxos implementation.

This implementation of Paxos made one design choice that is not explicitly stated
in the Paxos made simple paper, each value accepted by acceptor has a proposal
id, refers to the proposal this value was proposed.
If a proposer receives multiple promise messages with different
value, it will pick the value with highest proposal id.

There is no learner in this implementation, consensus is learned by the proposer.

A few possible improvements/future works are:
1. Add a NACK message, when an acceptor receives a prepare message with a proposal id
less than the highest proposal it has received, so the proposal can start another round
eariler.
2. Implement other variations, for example, fast paxos and Byzantine Paxos.
3. This program implements a library for leader selection, maybe extend it to a 
replicated state machine first(as mentioned in the Paxos made simple paper) and
base on it implement a lock service as described in the Chubby paper.
4. Write states to disk, so the acceptors can fail.
5. Improve the Env class, use real threads/processes to simulate components in the
Paxos implementation.
