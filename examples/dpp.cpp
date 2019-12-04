/*! \file  dpp.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/rx-fsm.hpp"
#include <cassert>

#define LOG std::cout

class table;

class philosopher final
{
public:
    philosopher(const std::string& name,
                int timeout_ms);

    const std::string& name() const
    {
        return sm.name();
    }

    void eat()
    {
        eat_subscriber.on_next(1);
    }

    template<class Coordination>
    void start(Coordination cn)
    {
        sm.start(std::move(cn));
    }

    void terminate()
    {
        sm.terminate();
    }

    bool is_hungry()
    {
        return hungry;
    }

    std::atomic<bool>& fork()
    {
        return _fork;
    }

    int ate() const
    {
        return _ate;
    }

    bool is_terminated() const
    {
        return sm.is_terminated();
    }

    void set_table(::table* t)
    {
       table = t;
    }

private:
    rxcpp::fsm::state_machine sm;
    rxcpp::subjects::subject<int> eat_subject;
    rxcpp::subscriber<int> eat_subscriber;
    ::table* table;
    std::atomic<bool> hungry, _fork;
    int _ate;
};

class table final
{
public:
    table(const std::string& name);

    void hungry(std::string& name)
    {
        hungry_subscriber.on_next(name);
    }

    void done(std::string& name)
    {
        done_subscriber.on_next(name);
    }

    template<class Coordination>
    void start(Coordination cn)
    {
        sm.start(std::move(cn));
    }

    void terminate()
    {
        sm.terminate();
    }

    bool is_terminated() const
    {
        return sm.is_terminated();
    }

    void add_philosopher(::philosopher* p)
    {
       philosophers.push_back(p);
    }

private:
    philosopher* find(const std::string& name)
    {
        auto it = std::find_if(philosophers.begin(), philosophers.end(), [name](::philosopher* p) {
            return name == p->name();
        });
        if (it != philosophers.end())
        {
            return *it;
        }
        return nullptr;
    }

    philosopher* right(philosopher* p)
    {
        auto it = std::find(philosophers.begin(), philosophers.end(), p);
        ++it;
        if (it == philosophers.end())
        {
            return philosophers.front();
        }
        return *it;
    }

    philosopher* left(philosopher* p)
    {
        auto it = std::find(philosophers.rbegin(), philosophers.rend(), p);
        ++it;
        if (it == philosophers.rend())
        {
            return philosophers.back();
        }
        return *it;
    }

    void do_hungry(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto* p = find(name);
        assert(p && p->is_hungry());
        auto* left = this->left(p);
        auto& fork = p->fork();
        auto& left_fork = left->fork();
        if (fork && left_fork)
        {
            fork = false;
            left_fork = false;
            p->eat();
        }
    }

    void do_done(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto* p = find(name);
        assert(p && !p->is_hungry());
        auto* left = this->left(p);
        auto& fork = p->fork();
        auto& left_fork = left->fork();
        assert(!fork && !left_fork);
        fork = left_fork = true;
        auto* right = this->right(p);
        auto& right_fork = right->fork();
        if (right->is_hungry() && right_fork)
        {
            fork = false;
            right_fork = false;
            right->eat();
        }
        auto* left_left = this->left(left);
        auto& left_left_fork = left_left->fork();
        if (left->is_hungry() && left_left_fork)
        {
            left_left_fork = false;
            left_fork = false;
            left->eat();
        }
    }

    rxcpp::fsm::state_machine sm;
    rxcpp::subjects::subject<std::string> hungry_subject, done_subject;
    rxcpp::subscriber<std::string> hungry_subscriber, done_subscriber;
    std::vector<philosopher*> philosophers;
    std::mutex mutex;
};

philosopher::philosopher(const std::string& name,
                         int timeout_ms)
    : sm(rxcpp::fsm::make_state_machine(name))
    , eat_subscriber(eat_subject.get_subscriber())
    , table(nullptr)
    , hungry(false)
    , _fork(true)
    , _ate(0)
{
    auto initial = rxcpp::fsm::make_initial_pseudostate("initial");
    auto thinking = rxcpp::fsm::make_state("thinking");
    auto hungry = rxcpp::fsm::make_state("hungry");
    auto eating = rxcpp::fsm::make_state("eating");

    auto eat = eat_subject.get_observable();

    initial.with_transition("initial", thinking);
    thinking.with_on_entry([this]() {
        auto name = sm.name();
        LOG << name << " is thinking" << std::endl;
    }).with_transition("thinking_to_hungry", hungry, std::chrono::milliseconds(timeout_ms));
    hungry.with_on_entry([this]() {
        this->hungry = true;
        auto name = sm.name();
        table->hungry(name);
        LOG << name << " is hungry" << std::endl;
    });
    hungry.with_on_exit([this]() {
        this->hungry = false;
    });
    hungry.with_transition("hungry_to_eating", eating, eat);
    eating.with_on_entry([this]() {
        auto name = sm.name();
        LOG << name << " is eating" << std::endl;
        _ate++;
    }).with_on_exit([this]() {
        auto name = sm.name();
        table->done(name);
    }).with_transition("eating_to_thinking", thinking, std::chrono::milliseconds(timeout_ms));

    sm.with_state(initial, thinking, hungry, eating);

}

table::table(const std::string& name)
    : sm(rxcpp::fsm::make_state_machine(name))
    , hungry_subscriber(hungry_subject.get_subscriber())
    , done_subscriber(done_subject.get_subscriber())

{
    auto initial = rxcpp::fsm::make_initial_pseudostate("initial");
    auto serving = rxcpp::fsm::make_state("serving");

    initial.with_transition("initial", serving);
    serving.with_transition("internal_hungry", hungry_subject.get_observable(), [this] (const std::string& name) {
        do_hungry(name);
    }).with_transition("internal_done", done_subject.get_observable(), [this] (const std::string& name) {
        do_done(name);
    });

    sm.with_state(initial, serving);
}

int main(int, char**)
{
    auto scheduler = rxcpp::schedulers::make_new_thread();

    ::table table("Table");

    philosopher socrates("Socrates", 500);
    philosopher plato("Plato", 700);
    philosopher aristotle("Aristotle", 1100);
    philosopher epicurus("Epicurus", 2100);

    table.add_philosopher(&socrates);
    table.add_philosopher(&plato);
    table.add_philosopher(&aristotle);
    table.add_philosopher(&epicurus);
    socrates.set_table(&table);
    plato.set_table(&table);
    aristotle.set_table(&table);
    epicurus.set_table(&table);

    table.start(rxcpp::serialize_same_worker(scheduler.create_worker()));
    socrates.start(rxcpp::serialize_same_worker(scheduler.create_worker()));
    plato.start(rxcpp::serialize_same_worker(scheduler.create_worker()));
    aristotle.start(rxcpp::serialize_same_worker(scheduler.create_worker()));
    epicurus.start(rxcpp::serialize_same_worker(scheduler.create_worker()));

    std::this_thread::sleep_for(std::chrono::seconds(60));

    table.terminate();
    socrates.terminate();
    plato.terminate();
    aristotle.terminate();
    epicurus.terminate();

    LOG << std::endl;
    LOG << socrates.name() << " ate " << socrates.ate() << " times." << std::endl;
    LOG << plato.name() << " ate " << plato.ate() << " times." << std::endl;
    LOG << aristotle.name() << " ate " << aristotle.ate() << " times." << std::endl;
    LOG << epicurus.name() << " ate " << epicurus.ate() << " times." << std::endl;

    return 0;
}
