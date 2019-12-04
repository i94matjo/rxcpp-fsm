
# rxcpp-fsm 

rxcpp-fsm is a library implementing a feature complete<sup>1</sup> UML2 state machine in C++, using reactive extensions observables as events.
Based on an idea by [furuholm](https://github.com/furuholm/RxFsm).

## Prerequisites

You need a C++ compiler that supports C++11. The library is developed and tested with gcc 7.4.0 and clang 8.

## Features

Complete UML2 feature set, including:

* All pseodostate types: Initial, Terminate, Entry point, Exit point, Choice, Join, Fork, Junction, Shallow History, Deep History
* Composite states
* Orthogonal states
* Sub-machine states

## Simple example

```cpp
auto sm = rxcpp::fsm::make_state_machine("philosopher");

auto initial = rxcpp::fsm::make_initial_pseudostate("initial");
auto thinking = rxcpp::fsm::make_state("thinking");
auto hungry = rxcpp::fsm::make_state("hungry");
auto eating = rxcpp::fsm::make_state("eating");

auto eat_subject = rxcpp::subjects::subject<int>();
auto eat = eat_subject.get_observable();

initial.with_transition("initial", thinking);

thinking.with_on_entry([]() {
    std::cout << "Thinking" << std::endl;
}).with_transition("thinking_to_hungry", hungry, std::chrono::milliseconds(500));

hungry.with_on_entry([]() {
    std::cout << "Hungry" << std::endl;
}).with_transition("hungry_to_eating", eating, eat);

eating.with_on_entry([]() {
    std::cout << "Eating" << std::endl;
}).with_transition("eating_to_thinking", thinking, std::chrono::milliseconds(500));

sm.with_state(initial, thinking, hungry, eating);

auto scheduler = rxcpp::schedulers::make_new_thread();

auto o = sm.assemble(rxcpp::serialize_one_worker(scheduler.create_worker()));

o.subscribe([](const rxcpp::fsm::transition& t) {
    std::cout << "Taking transition " << t.name() << std::endl;
});
```

## TODO

<sup>1</sup> Pseudostate *Junction* is currently implemented exactly as *Choice* which does not conform to the UML2 standard.

## SEE
[OMG UML Specification](https://www.omg.org/spec/UML/2.5.1/PDF)
