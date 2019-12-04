#include "test.h"

SCENARIO_METHOD(fsm::string_fixture3, "pseudostate", "[fsm][pseudostate]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    auto s1_initial = fsm::make_initial_pseudostate("s1_initial");
    auto s1_1 = fsm::make_state("s1_1");
    auto s1_2 = fsm::make_state("s1_2");
    auto s1 = fsm::make_state("s1").with_sub_state(s1_1, s1_2);
    auto s2 = fsm::make_state("s2");
    sm.with_state(s1, s2);
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    GIVEN("no initial state"){
        CHECK_THROWS(sm.start(cn));
    }
    GIVEN("initial"){
        sm.with_state(initial);
        s1.with_sub_state(s1_initial);
        WHEN("incoming transitions"){
            CHECK_THROWS(s1.with_transition("s1_2_initial", initial, obs1));
        }
        WHEN("no outgoing transitions"){
           CHECK_THROWS(sm.start(cn));
        }
        WHEN("multiple outgoing transitions"){
            CHECK_NOTHROW(initial.with_transition("initial_2_s1", s1));
            CHECK_THROWS(initial.with_transition("initial_2_s2", s2));
            CHECK_THROWS(initial.with_transition("initial_2_s1", s1));
        }
        WHEN("guarded outgoing transitions"){
            CHECK_THROWS(initial.with_transition("initial_2_s1", s1, []() {return true;}));
        }
        WHEN("transition to wrong target"){
            CHECK_NOTHROW(initial.with_transition("initial_2_s1", s1));
            CHECK_NOTHROW(s1_initial.with_transition("s1_initial_2_s2", s2));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("transition to pseudostate target"){
            CHECK_THROWS(initial.with_transition("initial_2_s1_initial", s1_initial));
        }
        WHEN("single outgoing transition"){
            CHECK_NOTHROW(initial.with_transition("initial_2_s1", s1));
            CHECK_NOTHROW(s1_initial.with_transition("s1_initial_2_s1_1", s1_1));
            CHECK_NOTHROW(sm.start(cn));
        }
    }
    GIVEN("terminate"){
        initial.with_transition("initial_2_s1", s1);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        sm.with_state(initial);
        s1.with_sub_state(s1_initial);
        auto terminal = fsm::make_terminate_pseudostate("terminate");
        s1.with_transition("s1_2_terminate", terminal, obs1);
        sm.with_state(terminal);
        WHEN("outgoing transitions"){
            CHECK_THROWS(terminal.with_transition("terminate_2_s2", s2));
        }
        WHEN("termination"){
            auto terminated = std::make_shared<bool>(false);
            std::vector<std::string> result;
            CHECK_NOTHROW(sm.assemble(cn).subscribe([&](const fsm::transition&) {}, [terminated]() {*terminated = true;}));
            REQUIRE(!(*terminated));
            o1.on_next("a");
            REQUIRE(*terminated);
        }
    }
    GIVEN("entry_point"){
        initial.with_transition("initial_2_s1", s1);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        sm.with_state(initial);
        s1.with_sub_state(s1_initial);
        auto ep = fsm::make_entry_point_pseudostate("s1_entry");
        s1.with_sub_state(ep);
        auto s3 = fsm::make_state("s3");
        auto r1 = fsm::make_region("r1");
        auto r2 = fsm::make_region("r2");
        s3.with_region(r1, r2);
        auto s31_initial = fsm::make_initial_pseudostate("s31_initial");
        auto s31_1 = fsm::make_state("s31_1");
        auto s32_initial = fsm::make_initial_pseudostate("s32_initial");
        auto s32_1 = fsm::make_state("s32_1");
        r1.with_sub_state(s31_initial, s31_1);
        r2.with_sub_state(s32_initial, s32_1);
        s31_initial.with_transition("s31_initial_2_s31_1", s31_1);
        s32_initial.with_transition("s32_initial_2_s32_1", s32_1);
        sm.with_state(s3);
        WHEN("no outgoing transitions"){
           CHECK_THROWS(sm.start(cn));
        }
        WHEN("multiple outgoing transitions"){
            CHECK_NOTHROW(ep.with_transition("s1_entry_2_s1_1", s1_1));
            CHECK_THROWS(initial.with_transition("s1_entry_2_s1_2", s1_2));
            CHECK_THROWS(ep.with_transition("s1_entry_2_s1_1", s1_1));
        }
        WHEN("guarded outgoing transitions"){
            CHECK_THROWS(ep.with_transition("s1_entry_2_s1_1", s1_1, []() {return true;}));
        }
        WHEN("transition to wrong target"){
            CHECK_NOTHROW(ep.with_transition("s1_entry_2_s2", s2));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("entry point of orthogonal state"){
            auto ep31 = fsm::make_entry_point_pseudostate("s31_entry");
            auto fork = fsm::make_fork_pseudostate("fork");
            s1.with_transition("s1_2_fork", fork, obs3);
            fork.with_transition("fork_2_s31_entry", ep31);
            fork.with_transition("fork_2_s32_1", s32_1);
            ep31.with_transition("s31_entry_2_s31_1", s31_1);
            r1.with_sub_state(ep31);
            sm.with_state(fork);
            CHECK_NOTHROW(ep.with_transition("s1_entry_2_s1_1", s1_1));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("single outgoing transition"){
            CHECK_NOTHROW(ep.with_transition("s1_entry_2_s1_1", s1_1));
            CHECK_NOTHROW(sm.start(cn));
        }
    }
    GIVEN("exit_point"){
        initial.with_transition("initial_2_s1", s1);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        sm.with_state(initial);
        s1.with_sub_state(s1_initial);
        auto ep = fsm::make_exit_point_pseudostate("s1_exit");
        s1.with_sub_state(ep);
        auto s3 = fsm::make_state("s3");
        auto r1 = fsm::make_region("r1");
        auto r2 = fsm::make_region("r2");
        s3.with_region(r1, r2);
        auto s31_initial = fsm::make_initial_pseudostate("s31_initial");
        auto s31_1 = fsm::make_state("s31_1");
        auto s32_initial = fsm::make_initial_pseudostate("s32_initial");
        auto s32_1 = fsm::make_state("s32_1");
        r1.with_sub_state(s31_initial, s31_1);
        r2.with_sub_state(s32_initial, s32_1);
        s31_initial.with_transition("s31_initial_2_s31_1", s31_1);
        s32_initial.with_transition("s32_initial_2_s32_1", s32_1);
        sm.with_state(s3);
        WHEN("no outgoing transitions"){
           CHECK_THROWS(sm.start(cn));
        }
        WHEN("multiple outgoing transitions"){
            CHECK_NOTHROW(ep.with_transition("s1_exit_2_s2", s2));
            CHECK_THROWS(initial.with_transition("s1_exit_2_s3", s3));
            CHECK_THROWS(ep.with_transition("s1_exit_2_s2", s2));
        }
        WHEN("guarded outgoing transitions"){
            CHECK_THROWS(ep.with_transition("s1_exit_2_s2", s2, []() {return true;}));
        }
        WHEN("transition to wrong target"){
            CHECK_NOTHROW(ep.with_transition("s1_exit_2_s1_1", s1_1));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("transition to parent target"){
            CHECK_NOTHROW(ep.with_transition("s1_exit_2_s1", s1));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("exit point of orthogonal state"){
            auto ep31 = fsm::make_exit_point_pseudostate("s31_exit");
            s31_1.with_transition("s31_1_2_s31_exit", ep31, obs3);
            ep31.with_transition("s31_exit_2_s1", s1);
            r1.with_sub_state(ep31);
            CHECK_NOTHROW(ep.with_transition("s1_exit_2_s2", s2));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("single outgoing transition"){
            CHECK_NOTHROW(ep.with_transition("s1_exit_2_s2", s2));
            CHECK_NOTHROW(sm.start(cn));
        }
    }
    GIVEN("junction"){
        initial.with_transition("initial_2_s1", s1);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        sm.with_state(initial);
        s1.with_sub_state(s1_initial);
        auto junction = fsm::make_junction_pseudostate("junction");
        sm.with_state(junction);
        WHEN("no outgoing transitions"){
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("transition to self"){
            CHECK_NOTHROW(junction.with_transition("junction_2_junction", junction));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("no default outgoing transition"){
            CHECK_NOTHROW(junction.with_transition("junction_2_s1", s1, []() {return true;}));
            CHECK_NOTHROW(junction.with_transition("junction_2_s2", s2, []() {return true;}));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("with default outgoing transition"){
            CHECK_NOTHROW(junction.with_transition("junction_2_s1", s1, []() {return true;}));
            CHECK_NOTHROW(junction.with_transition("junction_2_s2", s2));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("with single default outgoing transition"){
            CHECK_NOTHROW(junction.with_transition("junction_2_s2", s2));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("guard evaluation and actions"){
            bool guard1(false), guard2(false);
            std::vector<std::string> result;
            s1_1.with_transition("s1_1_2_junction", junction, obs1);
            auto s1_3 = fsm::make_state("s1_3");
            s1.with_sub_state(s1_3);
            junction.with_transition("junction_2_s1_2", s1_2, [&result]() {
                result.push_back("action1");
            }, [&guard1]() {return guard1;});
            junction.with_transition("junction_2_s1_3", s1_3, [&result]() {
                result.push_back("action2");
            }, [&guard2]() {return guard2;});
            junction.with_transition("junction_2_s2", s2, [&result]() {result.push_back("action3");});
            CHECK_NOTHROW(sm.start(cn));
            THEN("guard1"){
                guard1 = true;
                o1.on_next("a");
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "action1");
            }
            THEN("guard2"){
                guard2 = true;
                o1.on_next("a");
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "action2");
            }
            THEN("default"){
                o1.on_next("a");
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "action3");
            }
        }
    }
    GIVEN("choice"){
        initial.with_transition("initial_2_s1", s1);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        sm.with_state(initial);
        s1.with_sub_state(s1_initial);
        auto choice = fsm::make_choice_pseudostate("choice");
        sm.with_state(choice);
        WHEN("no outgoing transitions"){
           CHECK_THROWS(sm.start(cn));
        }
        WHEN("no default outgoing transition"){
            CHECK_NOTHROW(choice.with_transition("choice_2_s1", s1, []() {return true;}));
            CHECK_NOTHROW(choice.with_transition("choice_2_s2", s2, []() {return true;}));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("with default outgoing transition"){
            CHECK_NOTHROW(choice.with_transition("choice_2_s1", s1, []() {return true;}));
            CHECK_NOTHROW(choice.with_transition("choice_2_s2", s2));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("with single default outgoing transition"){
            CHECK_NOTHROW(choice.with_transition("choice_2_s2", s2));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("guard evaluation and actions"){
            bool guard1(false), guard2(false);
            std::vector<std::string> result;
            s1_1.with_transition("s1_1_2_choice", choice, obs1);
            auto s1_3 = fsm::make_state("s1_3");
            s1.with_sub_state(s1_3);
            choice.with_transition("choice_2_s1_2", s1_2, [&result]() {result.push_back("action1");}, [&guard1]() {return guard1;});
            choice.with_transition("choice_2_s1_3", s1_3, [&result]() {result.push_back("action2");}, [&guard2]() {return guard2;});
            choice.with_transition("choice_2_s2", s2, [&result]() {result.push_back("action3");});
            CHECK_NOTHROW(sm.start(cn));
            THEN("guard1"){
                guard1 = true;
                o1.on_next("a");
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "action1");
            }
            THEN("guard2"){
                guard2 = true;
                o1.on_next("a");
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "action2");
            }
            THEN("default"){
                o1.on_next("a");
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "action3");
            }
        }
    }
    GIVEN("shallow_history"){
        std::vector<std::string> result;
        initial.with_transition("initial_2_s2", s2);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        auto s1_history = fsm::make_shallow_history_pseudostate("history");
        sm.with_state(initial);
        s1.with_sub_state(s1_initial).with_sub_state(s1_history);
        auto s1_1_1 = fsm::make_state("s1_1_1");
        auto s1_1_2 = fsm::make_state("s1_1_2");
        auto s1_1_initial = fsm::make_initial_pseudostate("s1_1_initial");
        s1_1.with_sub_state(s1_1_1).with_sub_state(s1_1_2).with_sub_state(s1_1_initial);
        s1_1_initial.with_transition("s1_1_initial_2_s1_1_1", s1_1_1);
        s1.with_transition("s1_2_s2", s2, obs1);
        s2.with_transition("s2_2_s1", s1, obs1);
        s2.with_transition("s2_2_s1_history", s1_history, obs2);
        s1_1.with_transition("s1_1_2_s1_2", s1_2, obs2);
        s1_2.with_transition("s1_2_2_s1_1", s1_1, obs2);
        s1_1_1.with_transition("s1_1_1_2_s_1_1_2", s1_1_2, obs3);
        s1_1_2.with_transition("s1_1_2_2_s_1_1_1", s1_1_1, obs3);
        s1.with_on_entry([&result]() {result.push_back("s1");});
        s2.with_on_entry([&result]() {result.push_back("s2");});
        s1_1.with_on_entry([&result]() {result.push_back("s1_1");});
        s1_2.with_on_entry([&result]() {result.push_back("s1_2");});
        s1_1_1.with_on_entry([&result]() {result.push_back("s1_1_1");});
        s1_1_2.with_on_entry([&result]() {result.push_back("s1_1_2");});
        WHEN("multiple outgoing transitions"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1", s1_1));
            CHECK_THROWS(s1_history.with_transition("s1_history_2_s1_2", s1_2));
            CHECK_THROWS(s1_history.with_transition("s1_history_2_s1_1", s1_1));
        }
        WHEN("guarded outgoing transitions"){
            CHECK_THROWS(s1_history.with_transition("s1_history_2_s1_1", s1_1, []() {return true;}));
        }
        WHEN("transition to wrong target"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s2", s2));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("single outgoing transition"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1", s1_1));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("single outgoing transition to sub state"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1_1", s1_1_1));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("exiting state"){
            std::vector<fsm::transition> tresult;
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1_1", s1_1_1));
            CHECK_NOTHROW(sm.assemble(cn).subscribe([&tresult](const fsm::transition& t){tresult.push_back(t);}));
            THEN("default transition taken"){
                result.clear();
                tresult.clear();
                o2.on_next("a");
                REQUIRE(result.size() == 3);
                CHECK(result[0] == "s1");
                CHECK(result[1] == "s1_1");
                CHECK(result[2] == "s1_1_1");
                REQUIRE(tresult.size() == 2);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "history");
                CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "history");
                CHECK(tresult[1].target_state<fsm::state>().name() == "s1_1_1");
            }
            THEN("enter history"){
                o1.on_next("a");
                o2.on_next("b");
                REQUIRE(result.back() == "s1_2");
                o1.on_next("c");
                REQUIRE(result.back() == "s2");
                result.clear();
                tresult.clear();
                o2.on_next("d");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s1");
                CHECK(result[1] == "s1_2");
                REQUIRE(tresult.size() == 1);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "history");
            }
            THEN("enter history no sub state"){
                o1.on_next("a");
                o3.on_next("b");
                REQUIRE(result.back() == "s1_1_2");
                o1.on_next("c");
                REQUIRE(result.back() == "s2");
                result.clear();
                tresult.clear();
                o2.on_next("d");
                REQUIRE(result.size() == 3);
                CHECK(result[0] == "s1");
                CHECK(result[1] == "s1_1");
                CHECK(result[2] == "s1_1_1");
                REQUIRE(tresult.size() == 2);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "history");
                CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s1_1_initial");
                CHECK(tresult[1].target_state<fsm::state>().name() == "s1_1_1");
            }
        }
    }
    GIVEN("deep_history"){
        std::vector<std::string> result;
        initial.with_transition("initial_2_s2", s2);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        auto s1_history = fsm::make_deep_history_pseudostate("history");
        sm.with_state(initial);
        s1.with_sub_state(s1_initial).with_sub_state(s1_history);
        auto s1_1_1 = fsm::make_state("s1_1_1");
        auto s1_1_2 = fsm::make_state("s1_1_2");
        auto s1_1_initial = fsm::make_initial_pseudostate("s1_1_initial");
        s1_1.with_sub_state(s1_1_1).with_sub_state(s1_1_2).with_sub_state(s1_1_initial);
        s1_1_initial.with_transition("s1_1_initial_2_s1_1_1", s1_1_1);
        s1.with_transition("s1_2_s2", s2, obs1);
        s2.with_transition("s2_2_s1", s1, obs1);
        s2.with_transition("s2_2_s1_history", s1_history, obs2);
        s1_1.with_transition("s1_1_2_s1_2", s1_2, obs2);
        s1_2.with_transition("s1_2_2_s1_1", s1_1, obs2);
        s1_1_1.with_transition("s1_1_1_2_s_1_1_2", s1_1_2, obs3);
        s1_1_2.with_transition("s1_1_2_2_s_1_1_1", s1_1_1, obs3);
        s1.with_on_entry([&result]() {result.push_back("s1");});
        s2.with_on_entry([&result]() {result.push_back("s2");});
        s1_1.with_on_entry([&result]() {result.push_back("s1_1");});
        s1_2.with_on_entry([&result]() {result.push_back("s1_2");});
        s1_1_1.with_on_entry([&result]() {result.push_back("s1_1_1");});
        s1_1_2.with_on_entry([&result]() {result.push_back("s1_1_2");});
        WHEN("multiple outgoing transitions"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1", s1_1));
            CHECK_THROWS(s1_history.with_transition("s1_history_2_s1_2", s1_2));
            CHECK_THROWS(s1_history.with_transition("s1_history_2_s1_1", s1_1));
        }
        WHEN("guarded outgoing transitions"){
            CHECK_THROWS(s1_history.with_transition("s1_history_2_s1_1", s1_1, []() {return true;}));
        }
        WHEN("transition to wrong target"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s2", s2));
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("single outgoing transition"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1", s1_1));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("single outgoing transition to sub state"){
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1_1", s1_1_1));
            CHECK_NOTHROW(sm.start(cn));
        }
        WHEN("exiting state"){
            std::vector<fsm::transition> tresult;
            CHECK_NOTHROW(s1_history.with_transition("s1_history_2_s1_1_1", s1_1_1));
            CHECK_NOTHROW(sm.assemble(cn).subscribe([&tresult](const fsm::transition& t){tresult.push_back(t);}));
            THEN("default transition taken"){
                result.clear();
                tresult.clear();
                o2.on_next("a");
                REQUIRE(result.size() == 3);
                CHECK(result[0] == "s1");
                CHECK(result[1] == "s1_1");
                CHECK(result[2] == "s1_1_1");
                REQUIRE(tresult.size() == 2);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "history");
                CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "history");
                CHECK(tresult[1].target_state<fsm::state>().name() == "s1_1_1");
            }
            THEN("enter history"){
                o1.on_next("a");
                o2.on_next("b");
                REQUIRE(result.back() == "s1_2");
                o1.on_next("c");
                REQUIRE(result.back() == "s2");
                result.clear();
                tresult.clear();
                o2.on_next("d");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s1");
                CHECK(result[1] == "s1_2");
                REQUIRE(tresult.size() == 1);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "history");
            }
            THEN("enter history sub state"){
                o1.on_next("a");
                o3.on_next("b");
                REQUIRE(result.back() == "s1_1_2");
                o1.on_next("c");
                REQUIRE(result.back() == "s2");
                result.clear();
                tresult.clear();
                o2.on_next("d");
                REQUIRE(result.size() == 3);
                CHECK(result[0] == "s1");
                CHECK(result[1] == "s1_1");
                CHECK(result[2] == "s1_1_2");
                REQUIRE(tresult.size() == 1);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "history");
            }
        }
    }
    GIVEN("fork/join"){
        initial.with_transition("initial_2_s2", s2);
        auto s3 = fsm::make_state("s3");
        s3.with_transition("s3_2_s2", s2);
        sm.with_state(initial, s3);
        s1.with_sub_state(s1_initial);
        s1_initial.with_transition("s1_initial_2_s1_1", s1_1);
        auto r1 = fsm::make_region("r1");
        auto r2 = fsm::make_region("r2");
        auto r3 = fsm::make_region("r3");
        s3.with_region(r1, r2, r3);
        auto s31_initial = fsm::make_initial_pseudostate("s31_initial");
        auto s31_1 = fsm::make_state("s31_1");
        auto s31_final = fsm::make_final_state("s31_final");
        r1.with_sub_state(s31_initial, s31_1, s31_final);
        auto s32_initial = fsm::make_initial_pseudostate("s32_initial");
        auto s32_1 = fsm::make_state("s32_1");
        auto s32_final = fsm::make_final_state("s32_final");
        r2.with_sub_state(s32_initial, s32_1, s32_final);
        auto s33_initial = fsm::make_initial_pseudostate("s33_initial");
        auto s33_1 = fsm::make_state("s33_1");
        auto s33_final = fsm::make_final_state("s33_final");
        r3.with_sub_state(s33_initial, s33_1, s33_final);
        s31_initial.with_transition("s31_initial_2_s31_1", s31_1);
        s31_1.with_transition("s31_1_2_s31_final", s31_final, obs1);
        s32_initial.with_transition("s32_initial_2_s32_1", s32_1);
        s32_1.with_transition("s32_1_2_s32_final", s32_final, obs1);
        s33_initial.with_transition("s33_initial_2_s33_1", s33_1);
        s33_1.with_transition("s33_1_2_s33_final", s33_final, obs1);
        auto fork = fsm::make_fork_pseudostate("fork");
        auto join = fsm::make_join_pseudostate("join");
        sm.with_state(fork, join);
        s2.with_transition("s2_2_fork", fork, obs1);
        join.with_transition("join_2_s2", s2);
        WHEN("join ok"){
            s31_1.with_transition("s31_1_2_join", join, obs1);
            s32_1.with_transition("s32_1_2_join", join, obs2);
            s33_1.with_transition("s33_1_2_join", join, obs3);
            THEN("no outgoing fork transitions"){
                CHECK_THROWS(sm.start(cn));
            }
            THEN("to few outgoing fork transitions"){
                fork.with_transition("fork_2_s31_1", s31_1);
                CHECK_THROWS(sm.start(cn));
            }
            THEN("transition to wrong target"){
                fork.with_transition("fork_2_s31_1", s31_1);
                fork.with_transition("fork_2_s32_1", s32_1);
                fork.with_transition("fork_2_s2", s2);
                CHECK_THROWS(sm.start(cn));
            }
            THEN("transition to same region twice"){
                fork.with_transition("fork_2_s31_1", s31_1);
                fork.with_transition("fork_2_s32_1", s32_1);
                fork.with_transition("fork_2_s32_final", s32_final);
                CHECK_THROWS(sm.start(cn));
            }
            THEN("transitions to all regions"){
                fork.with_transition("fork_2_s31_1", s31_1);
                fork.with_transition("fork_2_s32_1", s32_1);
                fork.with_transition("fork_2_s33_1", s33_1);
                CHECK_NOTHROW(sm.start(cn));
            }
        }
        WHEN("fork ok"){
            fork.with_transition("fork_2_s31_1", s31_1);
            fork.with_transition("fork_2_s32_1", s32_1);
            fork.with_transition("fork_2_s33_1", s33_1);
            THEN("no incoming join transitions"){
                CHECK_THROWS(sm.start(cn));
            }
            THEN("to few incoming join transitions"){
                s31_1.with_transition("s31_1_2_join", join, obs1);
                CHECK_THROWS(sm.start(cn));
            }
            THEN("transition from wrong target"){
                s31_1.with_transition("s31_1_2_join", join, obs1);
                s32_1.with_transition("s32_1_2_join", join, obs2);
                s2.with_transition("s2_2_join", join);
                CHECK_THROWS(sm.start(cn));
            }
            THEN("transition from same region twice"){
                s31_1.with_transition("s31_1_2_join", join, obs1);
                s32_1.with_transition("s32_1_2_join", join, obs2);
                s32_1.with_transition("s32_1_2_join_2", join, obs3);
                CHECK_THROWS(sm.start(cn));
            }
            THEN("transitions from all regions"){
                s31_1.with_transition("s31_1_2_join", join, obs1);
                s32_1.with_transition("s32_1_2_join", join, obs2);
                s33_1.with_transition("s33_1_2_join", join, obs3);
                CHECK_NOTHROW(sm.start(cn));
            }
        }
    }
}
