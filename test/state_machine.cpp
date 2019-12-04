#include "test.h"

SCENARIO_METHOD(fsm::string_fixture5, "state_machine", "[fsm][state_machine]"){
    auto cn = rxcpp::identity_immediate();
    GIVEN("copy"){
        WHEN("equal"){
            auto sm2 = fsm::make_state_machine("sm2");
            CHECK(sm != sm2);
            auto sm_copy = sm;
            CHECK(sm == sm_copy);
        }
    }
    GIVEN("start"){
        WHEN("no initial state"){
            REQUIRE_THROWS(sm.start(cn));
        }
    }
    GIVEN("no substates"){
        auto result = std::vector<std::string>();
        FSM_SUBJECT(1);
        FSM_SUBJECT(2);
        auto initial = fsm::make_initial_pseudostate("initial");
        auto s1 = fsm::make_state("s1")
                .with_on_entry([&result]() { result.push_back("enter s1"); })
                .with_on_exit([&result]() { result.push_back("exit s1"); });
        auto s2 = fsm::make_state("s2")
                .with_on_entry([&result]() { result.push_back("enter s2"); })
                .with_on_exit([&result]() { result.push_back("exit s2"); });
        initial.with_transition("initial_2_s1", s1, [&result]() {
            result.push_back("initial");
        });
        s1.with_transition("s1_2_s2", s2, obs1, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs1 triggered: " << s;
            result.push_back(msg.str());
        });
        s2.with_transition("s2_2_s1", s1, obs2, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs2 triggered: " << s;
            result.push_back(msg.str());
        });
        sm.with_state(initial)
                .with_state(s1)
                .with_state(s2);
        WHEN("start"){
            CHECK_NOTHROW(sm.start(cn));
            auto unreachable_state_names = sm.find_unreachable_states();
            CHECK(unreachable_state_names.size() == 0);
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "initial");
            CHECK(result[1] == "enter s1");
            result.clear();
            o1.on_next("a");
            REQUIRE(result.size() == 3);
            CHECK(result[0] == "exit s1");
            CHECK(result[1] == "obs1 triggered: a");
            CHECK(result[2] == "enter s2");
            result.clear();
            o2.on_next("b");
            REQUIRE(result.size() == 3);
            CHECK(result[0] == "exit s2");
            CHECK(result[1] == "obs2 triggered: b");
            CHECK(result[2] == "enter s1");
        }
    }
    GIVEN("with substates"){
        auto result = std::vector<std::string>();
        FSM_SUBJECT(1);
        FSM_SUBJECT(2);
        FSM_SUBJECT(3);
        FSM_SUBJECT(4);
        FSM_SUBJECT(5);
        auto initial = fsm::make_initial_pseudostate("initial");
        auto s1_initial = fsm::make_initial_pseudostate("s1_initial");
        auto s1_1 = fsm::make_state("s1_1")
                .with_on_entry([&result]() { result.push_back("enter s1_1"); })
                .with_on_exit([&result]() { result.push_back("exit s1_1"); });
        auto s1_2 = fsm::make_state("s1_2")
                .with_on_entry([&result]() { result.push_back("enter s1_2"); })
                .with_on_exit([&result]() { result.push_back("exit s1_2"); });
        auto s1 = fsm::make_state("s1")
                .with_on_entry([&result]() { result.push_back("enter s1"); })
                .with_on_exit([&result]() { result.push_back("exit s1"); })
                .with_sub_state(s1_initial)
                .with_sub_state(s1_1)
                .with_sub_state(s1_2);
        auto s2_1 = fsm::make_state("s2_1")
                .with_on_entry([&result]() { result.push_back("enter s2_1"); })
                .with_on_exit([&result]() { result.push_back("exit s2_1"); });
        auto s2_2 = fsm::make_state("s2_2")
                .with_on_entry([&result]() { result.push_back("enter s2_2"); })
                .with_on_exit([&result]() { result.push_back("exit s2_2"); });
        auto s2 = fsm::make_state("s2")
                .with_on_entry([&result]() { result.push_back("enter s2"); })
                .with_on_exit([&result]() { result.push_back("exit s2"); })
                .with_sub_state(s2_1)
                .with_sub_state(s2_2);
        initial.with_transition("initial_2_s1", s1, [&result]() {
            result.push_back("initial");
        });
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1, [&result]() {
            result.push_back("s1 initial");
        });
        s1_1.with_transition("s1_1_2_s2_2", s2_2, obs1, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs1 triggered: " << s;
            result.push_back(msg.str());
        });
        s1_2.with_transition("s1_2_internal", obs5, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs5 triggered internal s1_2: " << s;
            result.push_back(msg.str());
        },
        [](const std::string& s) {
            return s == "h";
        });
        s1.with_transition("s1_internal_1", obs4, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs4 triggered internal s1: " << s;
            result.push_back(msg.str());
        });
        s1.with_transition("s1_internal_2", obs5, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs5 triggered internal s1: " << s;
            result.push_back(msg.str());
        });
        s2_2.with_transition("s2_2_2_s2_1", s2_1, obs2, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs2 triggered: " << s;
            result.push_back(msg.str());
        },
        [](const std::string& s) {
            return s == "c";
        });
        s2_2.with_transition("s2_2_2_s1_2", s1_2, obs3, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs3 triggered: " << s;
            result.push_back(msg.str());
        });
        s2.with_transition("s2_2_s1", s1, obs3, [&result](const std::string& s) {
            std::ostringstream msg;
            msg << "obs3 triggered: " << s;
            result.push_back(msg.str());
        });
        sm.with_state(initial).with_state(s1).with_state(s2);
        WHEN("start"){
            // should transition to s1_1
            CHECK_NOTHROW(sm.start(cn));
            auto unreachable_state_names = sm.find_unreachable_states();
            CHECK(unreachable_state_names.size() == 0);
            REQUIRE(result.size() == 4);
            CHECK(result[0] == "initial");
            CHECK(result[1] == "enter s1");
            CHECK(result[2] == "s1 initial");
            CHECK(result[3] == "enter s1_1");
            result.clear();
            // should transition to s2_2
            o1.on_next("a");
            REQUIRE(result.size() == 5);
            CHECK(result[0] == "exit s1_1");
            CHECK(result[1] == "exit s1");
            CHECK(result[2] == "obs1 triggered: a");
            CHECK(result[3] == "enter s2");
            CHECK(result[4] == "enter s2_2");
            result.clear();
            // should not transition since guard function will not be evaluated to true
            o2.on_next("b");
            REQUIRE(result.size() == 0);
            // should transition to s2_1
            o2.on_next("c");
            REQUIRE(result.size() == 3);
            CHECK(result[0] == "exit s2_2");
            CHECK(result[1] == "obs2 triggered: c");
            CHECK(result[2] == "enter s2_1");
            result.clear();
            // should transition to s1_1 since s2 has registered a transition to s1
            // (which should result in a transition to its initial sub state s1_1)
            o3.on_next("d");
            REQUIRE(result.size() == 6);
            CHECK(result[0] == "exit s2_1");
            CHECK(result[1] == "exit s2");
            CHECK(result[2] == "obs3 triggered: d");
            CHECK(result[3] == "enter s1");
            CHECK(result[4] == "s1 initial");
            CHECK(result[5] == "enter s1_1");
            result.clear();
            // should transition to s2_2
            o1.on_next("e");
            REQUIRE(result.size() == 5);
            CHECK(result[0] == "exit s1_1");
            CHECK(result[1] == "exit s1");
            CHECK(result[2] == "obs1 triggered: e");
            CHECK(result[3] == "enter s2");
            CHECK(result[4] == "enter s2_2");
            result.clear();
            // should transition to s1_2 since s2_2 has "overridden" transition (sub3)
            o3.on_next("f");
            REQUIRE(result.size() == 5);
            CHECK(result[0] == "exit s2_2");
            CHECK(result[1] == "exit s2");
            CHECK(result[2] == "obs3 triggered: f");
            CHECK(result[3] == "enter s1");
            CHECK(result[4] == "enter s1_2");
            result.clear();
            // should trigger internal transition
            o4.on_next("g");
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "obs4 triggered internal s1: g");
            result.clear();
            // should trigger internal transition in s1_2, which should "override"
            // internal transition in s1
            o5.on_next("h");
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "obs5 triggered internal s1_2: h");
            result.clear();
            // should NOT trigger internal transition in s1_2, since guard will fail
            // but instead trigger internal transition in s1
            o5.on_next("i");
            CHECK(result[0] == "obs5 triggered internal s1: i");
            result.clear();
        }
    }
}


SCENARIO_METHOD(fsm::string_fixture1, "exceptions", "[fsm][state_machine]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    FSM_SUBJECT(1);
    std::vector<std::string> result;
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    sm.with_state(initial, s1, s2);
    initial.with_transition("initial_2_s1", s1);
    GIVEN("entry/exit actions"){
        s1.with_transition("s1_2_s2", s2, obs1);
        WHEN("entry"){
            s2.with_on_entry([]() {throw std::runtime_error("entry");});
            auto o = sm.assemble(cn);
            o.subscribe([](const fsm::transition&){}, [&result](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                } catch(const std::runtime_error& e) {
                    result.push_back(e.what());
                }
            });
            REQUIRE(result.size() == 0);
            o1.on_next("a");
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "entry");
        }
        WHEN("exit"){
            s1.with_on_exit([]() {throw std::runtime_error("exit");});
            auto o = sm.assemble(cn);
            o.subscribe([](const fsm::transition&){}, [&result](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                } catch(const std::runtime_error& e) {
                    result.push_back(e.what());
                }
            });
            REQUIRE(result.size() == 0);
            o1.on_next("a");
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "exit");
        }
    }
    GIVEN("transition action"){
        s1.with_transition("s1_2_s2", s2, obs1, [](const std::string&) {throw std::runtime_error("action");}, [&result](const std::string&) {result.push_back("guard_executed"); return true;});
        auto o = sm.assemble(cn);
        o.subscribe([](const fsm::transition&){}, [&result](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch(const std::runtime_error& e) {
                result.push_back(e.what());
            }
        });
        REQUIRE(result.size() == 0);
        o1.on_next("a");
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "guard_executed");
        CHECK(result[1] == "action");
    }
    GIVEN("transition guard"){
        s1.with_transition("s1_2_s2", s2, obs1, [&result](const std::string&) {result.push_back("action_executed");}, [](const std::string& s) {if (s == "a") throw std::runtime_error("guard"); return true;});
        auto o = sm.assemble(cn);
        o.subscribe([](const fsm::transition&){}, [&result](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch(const std::runtime_error& e) {
                result.push_back(e.what());
            }
        });
        REQUIRE(result.size() == 0);
        WHEN("guard throws"){
            o1.on_next("a");
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "guard");
        }
        WHEN("guard does not throw"){
            o1.on_next("b");
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "action_executed");
        }
    }
    GIVEN("timeout action"){
        auto test = rxsc::make_test();
        auto w = test.create_worker();
        auto t = rxcpp::observe_on_one_worker(test);
        s1.with_transition("s1_2_s2", s2, t, std::chrono::seconds(1), [](){throw std::runtime_error("action");}, [&result](){result.push_back("guard_executed"); return true;});
        auto o = sm.assemble(cn);
        o.subscribe([](const fsm::transition&){}, [&result](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch(const std::runtime_error& e) {
                result.push_back(e.what());
            }
        });
        REQUIRE(result.size() == 0);
        w.advance_by(1001);
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "guard_executed");
        CHECK(result[1] == "action");
    }
    GIVEN("timeout guard"){
        auto test = rxsc::make_test();
        auto w = test.create_worker();
        auto t = rxcpp::observe_on_one_worker(test);
        bool do_throw(false);
        s1.with_transition("s1_2_s2", s2, t, std::chrono::seconds(1), [&result]() {result.push_back("action_executed");}, [&do_throw]() {if (do_throw) throw std::runtime_error("guard"); return true;});
        auto o = sm.assemble(cn);
        o.subscribe([](const fsm::transition&){}, [&result](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch(const std::runtime_error& e) {
                result.push_back(e.what());
            }
        });
        REQUIRE(result.size() == 0);
        WHEN("guard throws"){
            do_throw = true;
            w.advance_by(1001);
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "guard");
        }
        WHEN("guard does not throw"){
            do_throw = false;
            w.advance_by(1001);
            REQUIRE(result.size() == 1);
            CHECK(result[0] == "action_executed");
        }
    }
}


SCENARIO_METHOD(fsm::string_fixture2, "unreachable states", "[fsm][state_machine]"){
    auto cn = rxcpp::identity_immediate();
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    auto initial = fsm::make_initial_pseudostate("initial");
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    initial.with_transition("initial_2_s1", s1);
    s1.with_transition("s1_2_s2", s2, obs1);
    s2.with_transition("s2_2_s1", s1, obs2);
    sm.with_state(initial, s1, s2);
    GIVEN("unreachable top state"){
        auto s3 = fsm::make_state("s3");
        s3.with_transition("s3_2_s2", s2, obs1);
        sm.with_state(s3);
        CHECK_NOTHROW(sm.start(cn));
        auto unreachable_state_names = sm.find_unreachable_states();
        REQUIRE(unreachable_state_names.size() == 1);
        CHECK(unreachable_state_names[0] == "s3");
    }
    GIVEN("unreachable sub state"){
        auto s1_initial = fsm::make_initial_pseudostate("s1_initial");
        auto s1_1 = fsm::make_state("s1_1");
        auto s1_2 = fsm::make_state("s1_2");
        auto s1_3 = fsm::make_state("s1_3");
        auto s1_4 = fsm::make_state("s1_4");
        s1.with_sub_state(s1_initial, s1_1, s1_2, s1_3, s1_4);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        s1_1.with_transition("s1_1_2_s1_2", s1_2, obs2);
        s2.with_transition("s2_2_s1_3", s1_3, obs1);
        CHECK_NOTHROW(sm.start(cn));
        auto unreachable_state_names = sm.find_unreachable_states();
        REQUIRE(unreachable_state_names.size() == 1);
        CHECK(unreachable_state_names[0] == "s1_4");
    }
    GIVEN("unreachable sub state (orthogonal)"){
        auto r1 = fsm::make_region("r1");
        auto r2 = fsm::make_region("r2");
        s1.with_region(r1, r2);
        auto s11_initial = fsm::make_initial_pseudostate("s11_initial");
        auto s11_1 = fsm::make_state("s11_1");
        auto s11_2 = fsm::make_state("s11_2");
        s11_initial.with_transition("s11_initial_2_s11_1", s11_1);
        s11_1.with_transition("s11_1_2_s11_2", s11_2, obs2);
        r1.with_sub_state(s11_initial, s11_1, s11_2);
        auto s21_initial = fsm::make_initial_pseudostate("s21_initial");
        auto s21_1 = fsm::make_state("s21_1");
        auto s21_2 = fsm::make_state("s21_2");
        auto s21_3 = fsm::make_state("s21_3");
        s21_initial.with_transition("s21_initial_2_s21_1", s21_1);
        s21_1.with_transition("s21_1_2_s21_2", s21_2, obs2);
        r2.with_sub_state(s21_initial, s21_1, s21_2, s21_3);
        CHECK_NOTHROW(sm.start(cn));
        auto unreachable_state_names = sm.find_unreachable_states();
        REQUIRE(unreachable_state_names.size() == 1);
        CHECK(unreachable_state_names[0] == "s21_3");
    }
}

