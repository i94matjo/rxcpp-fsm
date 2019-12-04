#include "test.h"

SCENARIO_METHOD(fsm::string_fixture5, "simple state", "[fsm][state]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    sm.with_state(initial);
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    FSM_SUBJECT(4);
    FSM_SUBJECT(5);
    std::vector<std::string> result;
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    auto s3 = fsm::make_state("s3");
    auto s4 = fsm::make_state("s4");
    auto s5 = fsm::make_state("s5");
    sm.with_state(s1, s2, s3, s4, s5);
    s1.with_on_entry([&result]() {result.push_back("s1");});
    s2.with_on_entry([&result]() {result.push_back("s2");});
    s3.with_on_entry([&result]() {result.push_back("s3");});
    s4.with_on_entry([&result]() {result.push_back("s4");});
    s5.with_on_entry([&result]() {result.push_back("s5");});
    initial.with_transition("initial_2_s1", s1);
    GIVEN("equally named transitions"){
        CHECK_NOTHROW(s1.with_transition("same_name", s2));
        CHECK_THROWS(s1.with_transition("same_name", s2));
        CHECK_THROWS(s1.with_transition("same_name", s3));
    }
    GIVEN("add state after build"){
        CHECK_NOTHROW(sm.start(cn));
        auto s6 = fsm::make_state("s6");
        CHECK_THROWS(sm.with_state(s6));
        auto s1_1 = fsm::make_state("s1_1");
        CHECK_THROWS(s1.with_sub_state(s1_1));
        auto r1 = fsm::make_region("r1");
        CHECK_THROWS(s1.with_region(r1));
    }
    GIVEN("completion transitions"){
        s1.with_on_exit([&result]() {result.push_back("s1_exit");});
        s2.with_on_exit([&result]() {result.push_back("s2_exit");});
        s3.with_on_exit([&result]() {result.push_back("s3_exit");});
        s4.with_on_exit([&result]() {result.push_back("s4_exit");});
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, [&result]() {result.push_back("s2_2_s3");}));
        CHECK_NOTHROW(s3.with_transition("s3_2_s4", s4, [&result]() {result.push_back("s3_2_s4");}, [&result]() {result.push_back("s3_2_s4_guard"); return true;}));
        CHECK_NOTHROW(s4.with_transition("s4_2_s5", s5, [&result]() {result.push_back("s4_2_s5_guard"); return true;}));
        CHECK_NOTHROW(sm.start(cn));
        REQUIRE(result.size() == 13);
        CHECK(result[0] == "s1");
        CHECK(result[1] == "s1_exit");
        CHECK(result[2] == "s2");
        CHECK(result[3] == "s2_exit");
        CHECK(result[4] == "s2_2_s3");
        CHECK(result[5] == "s3");
        CHECK(result[6] == "s3_2_s4_guard");
        CHECK(result[7] == "s3_exit");
        CHECK(result[8] == "s3_2_s4");
        CHECK(result[9] == "s4");
        CHECK(result[10] == "s4_2_s5_guard");
        CHECK(result[11] == "s4_exit");
        CHECK(result[12] == "s5");
        THEN("adding transitions after build"){
            CHECK_THROWS(s1.with_transition("s1_2_s2_2", s2));
            CHECK_THROWS(s2.with_transition("s2_2_s3_2", s3));
            CHECK_THROWS(s3.with_transition("s3_2_s4_2", s4));
            CHECK_THROWS(s4.with_transition("s4_2_s5_2", s5));
        }
    }
    GIVEN("triggered transitions"){
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, obs1));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, obs2, [&result](const std::string&) {result.push_back("s2_2_s3");}));
        CHECK_NOTHROW(s3.with_transition("s3_2_s4", s4, obs3, [&result](const std::string&) {result.push_back("s3_2_s4");}, [&result](const std::string&) {result.push_back("s3_2_s4_guard"); return true;}));
        CHECK_NOTHROW(s4.with_transition("s4_2_s5", s5, obs4, [&result](const std::string&) {result.push_back("s4_2_s5_guard"); return true;}));
        CHECK_NOTHROW(sm.start(cn));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1");
        result.clear();
        o1.on_next("a");
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s2");
        result.clear();
        o2.on_next("b");
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "s2_2_s3");
        CHECK(result[1] == "s3");
        result.clear();
        o3.on_next("c");
        REQUIRE(result.size() == 3);
        CHECK(result[0] == "s3_2_s4_guard");
        CHECK(result[1] == "s3_2_s4");
        CHECK(result[2] == "s4");
        result.clear();
        o4.on_next("d");
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "s4_2_s5_guard");
        CHECK(result[1] == "s5");
        THEN("adding transitions after build"){
            CHECK_THROWS(s1.with_transition("s1_2_s2_2", s2, obs1));
            CHECK_THROWS(s2.with_transition("s2_2_s3_2", s3, obs2));
            CHECK_THROWS(s3.with_transition("s3_2_s4_2", s4, obs3));
            CHECK_THROWS(s4.with_transition("s4_2_s5_2", s5, obs4));
        }
    }
    GIVEN("timeout transitions"){
        auto test = rxsc::make_test();
        auto w = test.create_worker();
        auto t = rxcpp::observe_on_one_worker(test);
        bool s1_2_s2_executed(false);
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, t, std::chrono::milliseconds(500), [&result, &s1_2_s2_executed]() {
            result.push_back("s1_2_s2");
            s1_2_s2_executed = true;
        }));
        CHECK_NOTHROW(s2.with_transition("s2_2_s4", s4, obs1, [&result](const std::string&) {
            result.push_back("s2_2_s4");
        }));
        bool s2_2_s3_executed(false);
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, t, std::chrono::seconds(1), [&result, &s2_2_s3_executed]() {
            result.push_back("s2_2_s3");
            s2_2_s3_executed = true;
        }));
        bool s4_2_s1_exeucted(false);
        CHECK_NOTHROW(s4.with_transition("s4_2_s1", s1, t, std::chrono::milliseconds(500), [&result, &s4_2_s1_exeucted]() {
            result.push_back("s4_2_s1_guard");
            s4_2_s1_exeucted = true;
            return false;
        }));
        CHECK_NOTHROW(sm.start(cn));
        w.advance_by(1000);
        CHECK(s1_2_s2_executed);
        CHECK(!s4_2_s1_exeucted);
        CHECK(!s2_2_s3_executed);
        o1.on_next("a");
        w.advance_by(1000);
        CHECK(s4_2_s1_exeucted);
        CHECK(!s2_2_s3_executed);
        w.advance_by(1000000);
        CHECK(!s2_2_s3_executed);
        REQUIRE(result.size() == 6);
        CHECK(result[0] == "s1");
        CHECK(result[1] == "s1_2_s2");
        CHECK(result[2] == "s2");
        CHECK(result[3] == "s2_2_s4");
        CHECK(result[4] == "s4");
        CHECK(result[5] == "s4_2_s1_guard");
        THEN("adding transitions after build"){
            CHECK_THROWS(s1.with_transition("s1_2_s2_2", s2, t, std::chrono::milliseconds(500)));
            CHECK_THROWS(s2.with_transition("s2_2_s4_2", s4, obs1));
            CHECK_THROWS(s2.with_transition("s2_2_s3_2", s3, t, std::chrono::seconds(1)));
            CHECK_THROWS(s4.with_transition("s4_2_s1_2", s1, t, std::chrono::milliseconds(500)));
        }
    }
    GIVEN("internal transitions"){
        CHECK_NOTHROW(s1.with_transition("s1_internal_1", obs1));
        CHECK_NOTHROW(s1.with_transition("s1_internal_2", obs2, [&result](const std::string&) {result.push_back("s1_internal_2");}));
        CHECK_NOTHROW(s1.with_transition("s1_internal_3", obs3, [&result](const std::string&) {result.push_back("s1_internal_3");}, [&result](const std::string&) {result.push_back("s1_internal_3_guard"); return true;}));
        CHECK_NOTHROW(s1.with_transition("s1_internal_4", obs4, [&result](const std::string&) {result.push_back("s1_internal_4_guard"); return true;}));
        CHECK_NOTHROW(sm.start(cn));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1");
        result.clear();
        o1.on_next("a");
        REQUIRE(result.size() == 0);
        o2.on_next("b");
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_2");
        result.clear();
        o3.on_next("c");
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "s1_internal_3_guard");
        CHECK(result[1] == "s1_internal_3");
        result.clear();
        o4.on_next("d");
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_4_guard");
        result.clear();
        o2.on_next("e");
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_2");
        THEN("adding transitions after build"){
            CHECK_THROWS(s1.with_transition("s1_internal_1_2", obs1));
            CHECK_THROWS(s1.with_transition("s1_internal_2_2", obs2));
            CHECK_THROWS(s1.with_transition("s1_internal_3_2", obs3));
            CHECK_THROWS(s1.with_transition("s1_internal_4_2", obs4));
        }
    }
    GIVEN("transition to faulty state"){
        auto fs1 = fsm::make_state("fs1");
        auto fs2 = fsm::make_state("fs2");
        auto initial2 = fsm::make_initial_pseudostate("initial2");
        auto sm2 = fsm::make_state_machine("sm2");
        sm2.with_state(initial2)
                .with_state(fs2);
        initial2.with_transition("initial2", fs2);
        REQUIRE_NOTHROW(sm2.start(cn));
        WHEN("state with no state machine"){
            CHECK_NOTHROW(s1.with_transition("s1_2_fs1", fs1, obs1));
            REQUIRE_THROWS(sm.start(cn));
        }
        WHEN("state on other state machine"){
            CHECK_NOTHROW(s1.with_transition("s1_2_fs2", fs2, obs2));
            REQUIRE_THROWS(sm.start(cn));
        }
    }
    GIVEN("internal timeout transitions"){
        auto test = rxsc::make_test();
        auto w = test.create_worker();
        auto t = rxcpp::observe_on_one_worker(test);
        bool guard1(true), guard2(true);
        bool s1_internal_1_executed(false);
        CHECK_NOTHROW(s1.with_transition("s1_internal_1", t, std::chrono::milliseconds(50),
            [&result, &s1_internal_1_executed, &guard1]() {
                result.push_back("s1_internal_1");
                guard1 = false;
                s1_internal_1_executed = true;
            },
            [&guard1]() {
                return guard1;
            }));
        bool s1_internal_2_executed(false);
        CHECK_NOTHROW(s1.with_transition("s1_internal_2", t, std::chrono::milliseconds(500),
            [&result, &s1_internal_2_executed, &guard2]() {
                result.push_back("s1_internal_2");
                guard2 = false;
                s1_internal_2_executed = true;
            },
            [&guard2]() {
                return guard2;
            }));
        CHECK_NOTHROW(sm.start(cn));
        w.advance_by(100);
        CHECK(s1_internal_1_executed);
        CHECK(!s1_internal_2_executed);
        w.advance_by(600);
        CHECK(s1_internal_2_executed);
        w.advance_by(1000000);
        REQUIRE(result.size() == 3);
        CHECK(result[0] == "s1");
        CHECK(result[1] == "s1_internal_1");
        CHECK(result[2] == "s1_internal_2");
        CHECK(!guard1);
        CHECK(!guard2);
    }
    GIVEN("final state"){
        auto final = fsm::make_final_state("final");
        sm.with_state(final);
        auto terminated = std::make_shared<bool>(false);
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, obs1));
        CHECK_NOTHROW(s2.with_transition("s2_2_final", final, obs2));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, obs3));
        CHECK_NOTHROW(s2.with_transition("s2_2_s4", s4, obs4));
        CHECK_NOTHROW(s2.with_transition("s2_2_s5", s5, obs5));
        sm.assemble(cn).subscribe([](const fsm::transition&) {}, [terminated]() {*terminated = true;});
        REQUIRE(!(*terminated));
        o1.on_next("a");
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "s1");
        CHECK(result[1] == "s2");
        result.clear();
        o2.on_next("b");
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o1.on_next("c");
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o2.on_next("c");
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o3.on_next("c");
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o4.on_next("c");
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o5.on_next("c");
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
    }
}


SCENARIO_METHOD(fsm::string_fixture5, "composite state 1 - complex state machine", "[fsm][state]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    FSM_SUBJECT(4);
    FSM_SUBJECT(5);
    std::vector<std::string> result;
    auto s1 = fsm::make_state("s1").with_on_entry([&result]() {result.push_back("s1");}).with_on_exit([&result]() {result.push_back("s1_exit");});
    auto s2 = fsm::make_state("s2").with_on_entry([&result]() {result.push_back("s2");}).with_on_exit([&result]() {result.push_back("s2_exit");});
    auto s3 = fsm::make_state("s3").with_on_entry([&result]() {result.push_back("s3");}).with_on_exit([&result]() {result.push_back("s3_exit");});
    auto s2_1 = fsm::make_state("s2_1").with_on_entry([&result]() {result.push_back("s2_1");}).with_on_exit([&result]() {result.push_back("s2_1_exit");});
    auto s2_2 = fsm::make_state("s2_2").with_on_entry([&result]() {result.push_back("s2_2");}).with_on_exit([&result]() {result.push_back("s2_2_exit");});
    auto s2_2_1 = fsm::make_state("s2_2_1").with_on_entry([&result]() {result.push_back("s2_2_1");}).with_on_exit([&result]() {result.push_back("s2_2_1_exit");});
    auto s2_2_2 = fsm::make_state("s2_2_2").with_on_entry([&result]() {result.push_back("s2_2_2");}).with_on_exit([&result]() {result.push_back("s2_2_2_exit");});
    auto s2_2_2_1 = fsm::make_state("s2_2_2_1").with_on_entry([&result]() {result.push_back("s2_2_2_1");}).with_on_exit([&result]() {result.push_back("s2_2_2_1_exit");});
    auto s2_2_2_2 = fsm::make_state("s2_2_2_2").with_on_entry([&result]() {result.push_back("s2_2_2_2");}).with_on_exit([&result]() {result.push_back("s2_2_2_2_exit");});
    auto s2_2_2_initial = fsm::make_initial_pseudostate("s2_2_2_initial");
    auto s2_2_2_final = fsm::make_final_state("s2_2_2_final");
    auto s2_2_2_terminate = fsm::make_terminate_pseudostate("s2_2_2_terminate");
    auto s2_2_initial = fsm::make_initial_pseudostate("s2_2_initial");
    auto s2_2_final = fsm::make_final_state("s2_2_final");
    auto s2_2_history = fsm::make_deep_history_pseudostate("s2_2_history");
    auto s2_2_exit = fsm::make_exit_point_pseudostate("s2_2_exit");
    auto s2_initial = fsm::make_initial_pseudostate("s2_initial");
    auto s2_final = fsm::make_final_state("s2_final");
    auto s2_entry = fsm::make_entry_point_pseudostate("s2_entry");
    sm.with_state(initial, s1, s2, s3);
    CHECK(s2.is_simple());
    s2.with_sub_state(s2_initial, s2_1, s2_2, s2_final, s2_entry);
    CHECK(s2.is_composite());
    CHECK(s2_2.is_simple());
    s2_2.with_sub_state(s2_2_initial, s2_2_1, s2_2_2, s2_2_final, s2_2_exit, s2_2_history);
    CHECK(s2_2.is_composite());
    CHECK(s2_2_2.is_simple());
    s2_2_2.with_sub_state(s2_2_2_initial, s2_2_2_1, s2_2_2_2, s2_2_2_final, s2_2_2_terminate);
    CHECK(s2_2_2.is_composite());
    initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
    s1.with_transition("s1_2_s2", s2, obs1, [&result](const std::string&){result.push_back("s1_2_s2");});
    s2.with_transition("s2_2_s1", s1, [&result](){result.push_back("s2_2_s1");});
    s2.with_transition("s2_2_s3", s3, obs2, [&result](const std::string&){result.push_back("s2_2_s3");});
    s3.with_transition("s3_2_s1", s1, obs3, [&result](const std::string&){result.push_back("s3_2_s1");});
    s3.with_transition("s3_2_s2_entry", s2_entry, obs1, [&result](const std::string&){result.push_back("s3_2_s2_entry");});
    s3.with_transition("s3_2_s2_2_history", s2_2_history, obs4, [&result](const std::string&){result.push_back("s3_2_s2_2_history");});
    s2_initial.with_transition("s2_initial_2_s2_1", s2_1, [&result](){result.push_back("s2_initial_2_s2_1");});
    s2_entry.with_transition("s2_entry_2_s2_2", s2_2, [&result](){result.push_back("s2_entry_2_s2_2");});
    s2_1.with_transition("s2_1_2_s2_2_2", s2_2_2, obs1, [&result](const std::string&){result.push_back("s2_1_2_s2_2_2");});
    s2_1.with_transition("s2_1_2_s2_2", s2_2, obs2, [&result](const std::string&){result.push_back("s2_1_2_s2_2");}, [](const std::string& s){return s == "a";});
    s2_2.with_transition("s2_2_2_s2_final", s2_final, [&result](){result.push_back("s2_2_2_s2_final");});
    s2_2_initial.with_transition("s2_2_initial_2_s2_2_1", s2_2_1, [&result](){result.push_back("s2_2_initial_2_s2_2_1");});
    s2_2_1.with_transition("s2_2_1_2_s3", s3, obs3, [&result](const std::string&){result.push_back("s2_2_1_2_s3");});
    s2_2_1.with_transition("s2_2_1_2_s2_2_2", s2_2_2, obs1, [&result](const std::string&){result.push_back("s2_2_1_2_s2_2_2");});
    s2_2_2.with_transition("s2_2_2_2_s2_2_final", s2_2_final, [&result](){result.push_back("s2_2_2_2_s2_2_final");});
    s2_2_exit.with_transition("s2_2_exit_2_s2_1", s2_1, [&result](){result.push_back("s2_2_exit_2_s2_1");});
    s2_2_history.with_transition("s2_2_history_2_s2_2_1", s2_2_1, [&result](){result.push_back("s2_2_history_2_s2_2_1");});
    s2_2_2_initial.with_transition("s2_2_2_initial_2_s2_2_2_1", s2_2_2_1, [&result](){result.push_back("s2_2_2_initial_2_s2_2_2_1");});
    s2_2_2_1.with_transition("s2_2_2_1_2_s3", s3, obs3, [&result](const std::string&){result.push_back("s2_2_2_1_2_s3");});
    s2_2_2_1.with_transition("s2_2_2_1_2_s2_2_2_2", s2_2_2_2, obs1, [&result](const std::string&){result.push_back("s2_2_2_1_2_s2_2_2_2");});
    s2_2_2_1.with_transition("s2_2_2_1_2_s2_2_exit", s2_2_exit, obs2, [&result](const std::string&){result.push_back("s2_2_2_1_2_s2_2_exit");}, [](const std::string& s){return s == "a";});
    s2_2_2_2.with_transition("s2_2_2_2_2_s2_2_2_final", s2_2_2_final, obs1, [&result](const std::string&){result.push_back("s2_2_2_2_2_s2_2_2_final");});
    s2_2_2_2.with_transition("s2_2_2_2_2_s2_2_2_terminate", s2_2_2_terminate, obs2, [&result](const std::string&){result.push_back("s2_2_2_2_2_s2_2_2_terminate");}, [](const std::string& s){return s == "a";});
    std::vector<fsm::transition> tresult;
    REQUIRE_NOTHROW(sm.assemble(cn).subscribe([&tresult](const fsm::transition& t){tresult.push_back(t);}));
    CHECK(!sm.is_terminated());
    auto unreachable_state_names = sm.find_unreachable_states();
    CHECK(unreachable_state_names.size() == 0);
    GIVEN("s1"){
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        REQUIRE(tresult.size() == 1);
        CHECK(tresult[0].source_state_type() == fsm::transition::pseudo);
        CHECK(tresult[0].target_state_type() == fsm::transition::regular);
        CHECK(tresult[0].source_state<fsm::pseudostate>().name() == "initial");
        CHECK(tresult[0].target_state<fsm::state>().name() == "s1");
        CHECK_THROWS(tresult[0].source_state<fsm::state>());
        CHECK_THROWS(tresult[0].target_state<fsm::final_state>());
        result.clear();
        tresult.clear();
        WHEN("transition to s2"){
            o1.on_next("a");
            REQUIRE(result.size() == 5);
            CHECK(result[0] == "s1_exit");
            CHECK(result[1] == "s1_2_s2");
            CHECK(result[2] == "s2");
            CHECK(result[3] == "s2_initial_2_s2_1");
            CHECK(result[4] == "s2_1");
            REQUIRE(tresult.size() == 2);
            CHECK(tresult[0].source_state_type() == fsm::transition::regular);
            CHECK(tresult[0].target_state_type() == fsm::transition::regular);
            CHECK(tresult[0].source_state<fsm::state>().name() == "s1");
            CHECK(tresult[0].target_state<fsm::state>().name() == "s2");
            CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[1].target_state_type() == fsm::transition::regular);
            CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s2_initial");
            CHECK(tresult[1].target_state<fsm::state>().name() == "s2_1");
            result.clear();
            tresult.clear();
            THEN("s2_1"){
               THEN("transition to s2_2"){
                   o2.on_next("a");
                   REQUIRE(result.size() == 5);
                   CHECK(result[0] == "s2_1_exit");
                   CHECK(result[1] == "s2_1_2_s2_2");
                   CHECK(result[2] == "s2_2");
                   CHECK(result[3] == "s2_2_initial_2_s2_2_1");
                   CHECK(result[4] == "s2_2_1");
                   REQUIRE(tresult.size() == 2);
                   CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                   CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                   CHECK(tresult[0].source_state<fsm::state>().name() == "s2_1");
                   CHECK(tresult[0].target_state<fsm::state>().name() == "s2_2");
                   CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                   CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                   CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s2_2_initial");
                   CHECK(tresult[1].target_state<fsm::state>().name() == "s2_2_1");
                   result.clear();
                   tresult.clear();
                   THEN("s2_2_1"){
                       THEN("transition to s2_2_2"){
                           o1.on_next("a");
                           REQUIRE(result.size() == 5);
                           CHECK(result[0] == "s2_2_1_exit");
                           CHECK(result[1] == "s2_2_1_2_s2_2_2");
                           CHECK(result[2] == "s2_2_2");
                           CHECK(result[3] == "s2_2_2_initial_2_s2_2_2_1");
                           CHECK(result[4] == "s2_2_2_1");
                           REQUIRE(tresult.size() == 2);
                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_1");
                           CHECK(tresult[0].target_state<fsm::state>().name() == "s2_2_2");
                           CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                           CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s2_2_2_initial");
                           CHECK(tresult[1].target_state<fsm::state>().name() == "s2_2_2_1");
                           result.clear();
                           tresult.clear();
                           THEN("s2_2_2_1"){
                               THEN("transition to s2_2_2_2"){
                                   o1.on_next("a");
                                   REQUIRE(result.size() == 3);
                                   CHECK(result[0] == "s2_2_2_1_exit");
                                   CHECK(result[1] == "s2_2_2_1_2_s2_2_2_2");
                                   CHECK(result[2] == "s2_2_2_2");
                                   REQUIRE(tresult.size() == 1);
                                   CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_2_1");
                                   CHECK(tresult[0].target_state<fsm::state>().name() == "s2_2_2_2");
                                   result.clear();
                                   tresult.clear();
                                   THEN("s2_2_2_2"){
                                       THEN("transition to s3"){
                                           o2.on_next("b");
                                           REQUIRE(result.size() == 6);
                                           CHECK(result[0] == "s2_2_2_2_exit");
                                           CHECK(result[1] == "s2_2_2_exit");
                                           CHECK(result[2] == "s2_2_exit");
                                           CHECK(result[3] == "s2_exit");
                                           CHECK(result[4] == "s2_2_s3");
                                           CHECK(result[5] == "s3");
                                           REQUIRE(tresult.size() == 1);
                                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                           CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                                           CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                                           CHECK(tresult[0].target_state<fsm::state>().name() == "s3");
                                           CHECK(!sm.is_terminated());
                                           result.clear();
                                           tresult.clear();
                                           THEN("s3 from s2_2_2_2"){
                                               o4.on_next("a");
                                               REQUIRE(result.size() == 6);
                                               CHECK(result[0] == "s3_exit");
                                               CHECK(result[1] == "s3_2_s2_2_history");
                                               CHECK(result[2] == "s2");
                                               CHECK(result[3] == "s2_2");
                                               CHECK(result[4] == "s2_2_2");
                                               CHECK(result[5] == "s2_2_2_2");
                                               REQUIRE(tresult.size() == 1);
                                               CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                               CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                                               CHECK(tresult[0].source_state<fsm::state>().name() == "s3");
                                               CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "s2_2_history");
                                               CHECK(!sm.is_terminated());
                                           }
                                       }
                                       THEN("transition to s2_2_2_final, i.e. transition to s1"){
                                          o1.on_next("a");
                                          REQUIRE(result.size() == 9);
                                          CHECK(result[0] == "s2_2_2_2_exit");
                                          CHECK(result[1] == "s2_2_2_2_2_s2_2_2_final");
                                          CHECK(result[2] == "s2_2_2_exit");
                                          CHECK(result[3] == "s2_2_2_2_s2_2_final");
                                          CHECK(result[4] == "s2_2_exit");
                                          CHECK(result[5] == "s2_2_2_s2_final");
                                          CHECK(result[6] == "s2_exit");
                                          CHECK(result[7] == "s2_2_s1");
                                          CHECK(result[8] == "s1");
                                          REQUIRE(tresult.size() == 4);
                                          CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                          CHECK(tresult[0].target_state_type() == fsm::transition::final);
                                          CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_2_2");
                                          CHECK(tresult[0].target_state<fsm::final_state>().name() == "s2_2_2_final");
                                          CHECK(tresult[1].source_state_type() == fsm::transition::regular);
                                          CHECK(tresult[1].target_state_type() == fsm::transition::final);
                                          CHECK(tresult[1].source_state<fsm::state>().name() == "s2_2_2");
                                          CHECK(tresult[1].target_state<fsm::final_state>().name() == "s2_2_final");
                                          CHECK(tresult[2].source_state_type() == fsm::transition::regular);
                                          CHECK(tresult[2].target_state_type() == fsm::transition::final);
                                          CHECK(tresult[2].source_state<fsm::state>().name() == "s2_2");
                                          CHECK(tresult[2].target_state<fsm::final_state>().name() == "s2_final");
                                          CHECK(tresult[3].source_state_type() == fsm::transition::regular);
                                          CHECK(tresult[3].target_state_type() == fsm::transition::regular);
                                          CHECK(tresult[3].source_state<fsm::state>().name() == "s2");
                                          CHECK(tresult[3].target_state<fsm::state>().name() == "s1");
                                       }
                                       THEN("terminate"){
                                           o2.on_next("a");
                                           REQUIRE(result.size() == 1);
                                           CHECK(result[0] == "s2_2_2_2_2_s2_2_2_terminate");
                                           REQUIRE(tresult.size() == 1);
                                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                           CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                                           CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_2_2");
                                           CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "s2_2_2_terminate");
                                           CHECK(sm.is_terminated());
                                       }
                                       THEN("no transitions"){
                                           o3.on_next("a");
                                           o4.on_next("a");
                                           o5.on_next("a");
                                           REQUIRE(result.size() == 0);
                                           REQUIRE(tresult.size() == 0);
                                           CHECK(!sm.is_terminated());
                                       }
                                   }
                               }
                               THEN("transition to s3 by o3"){
                                   o3.on_next("a");
                                   REQUIRE(result.size() == 6);
                                   CHECK(result[0] == "s2_2_2_1_exit");
                                   CHECK(result[1] == "s2_2_2_exit");
                                   CHECK(result[2] == "s2_2_exit");
                                   CHECK(result[3] == "s2_exit");
                                   CHECK(result[4] == "s2_2_2_1_2_s3");
                                   CHECK(result[5] == "s3");
                                   REQUIRE(tresult.size() == 1);
                                   CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_2_1");
                                   CHECK(tresult[0].target_state<fsm::state>().name() == "s3");
                               }
                               THEN("transition to s3 by o2"){
                                   o2.on_next("b");
                                   REQUIRE(result.size() == 6);
                                   CHECK(result[0] == "s2_2_2_1_exit");
                                   CHECK(result[1] == "s2_2_2_exit");
                                   CHECK(result[2] == "s2_2_exit");
                                   CHECK(result[3] == "s2_exit");
                                   CHECK(result[4] == "s2_2_s3");
                                   CHECK(result[5] == "s3");
                                   REQUIRE(tresult.size() == 1);
                                   CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                                   CHECK(tresult[0].target_state<fsm::state>().name() == "s3");
                                   result.clear();
                                   tresult.clear();
                                   THEN("s3 from s2_2_2_1"){
                                      o4.on_next("a");
                                      REQUIRE(result.size() == 6);
                                      CHECK(result[0] == "s3_exit");
                                      CHECK(result[1] == "s3_2_s2_2_history");
                                      CHECK(result[2] == "s2");
                                      CHECK(result[3] == "s2_2");
                                      CHECK(result[4] == "s2_2_2");
                                      CHECK(result[5] == "s2_2_2_1");
                                      REQUIRE(tresult.size() == 1);
                                      CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                      CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                                      CHECK(tresult[0].source_state<fsm::state>().name() == "s3");
                                      CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "s2_2_history");
                                   }
                               }
                               THEN("transition to s2_1"){
                                   o2.on_next("a");
                                   REQUIRE(result.size() == 6);
                                   CHECK(result[0] == "s2_2_2_1_exit");
                                   CHECK(result[1] == "s2_2_2_exit");
                                   CHECK(result[2] == "s2_2_2_1_2_s2_2_exit");
                                   CHECK(result[3] == "s2_2_exit");
                                   CHECK(result[4] == "s2_2_exit_2_s2_1");
                                   CHECK(result[5] == "s2_1");
                                   REQUIRE(tresult.size() == 2);
                                   CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                                   CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                                   CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_2_1");
                                   CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "s2_2_exit");
                                   CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                                   CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                                   CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s2_2_exit");
                                   CHECK(tresult[1].target_state<fsm::state>().name() == "s2_1");
                               }
                               THEN("no transitions"){
                                   o4.on_next("a");
                                   o5.on_next("a");
                                   REQUIRE(result.size() == 0);
                                   REQUIRE(tresult.size() == 0);
                               }
                           }
                       }
                       THEN("transition to s3 by o3"){
                           o3.on_next("a");
                           REQUIRE(result.size() == 5);
                           CHECK(result[0] == "s2_2_1_exit");
                           CHECK(result[1] == "s2_2_exit");
                           CHECK(result[2] == "s2_exit");
                           CHECK(result[3] == "s2_2_1_2_s3");
                           CHECK(result[4] == "s3");
                           REQUIRE(tresult.size() == 1);
                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].source_state<fsm::state>().name() == "s2_2_1");
                           CHECK(tresult[0].target_state<fsm::state>().name() == "s3");
                           result.clear();
                           tresult.clear();
                           THEN("s3 from s2_2_1"){
                               o4.on_next("a");
                               REQUIRE(result.size() == 5);
                               CHECK(result[0] == "s3_exit");
                               CHECK(result[1] == "s3_2_s2_2_history");
                               CHECK(result[2] == "s2");
                               CHECK(result[3] == "s2_2");
                               CHECK(result[4] == "s2_2_1");
                               REQUIRE(tresult.size() == 1);
                               CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                               CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                               CHECK(tresult[0].source_state<fsm::state>().name() == "s3");
                               CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "s2_2_history");
                           }
                       }
                       THEN("transition to s3 by o2"){
                           o2.on_next("a");
                           REQUIRE(result.size() == 5);
                           CHECK(result[0] == "s2_2_1_exit");
                           CHECK(result[1] == "s2_2_exit");
                           CHECK(result[2] == "s2_exit");
                           CHECK(result[3] == "s2_2_s3");
                           CHECK(result[4] == "s3");
                           REQUIRE(tresult.size() == 1);
                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                           CHECK(tresult[0].target_state<fsm::state>().name() == "s3");
                       }
                       THEN("no transitions"){
                           o4.on_next("a");
                           o5.on_next("a");
                           REQUIRE(result.size() == 0);
                           REQUIRE(tresult.size() == 0);
                       }
                   }
               }
               THEN("transition to s3"){
                   o2.on_next("b");
                   REQUIRE(result.size() == 4);
                   CHECK(result[0] == "s2_1_exit");
                   CHECK(result[1] == "s2_exit");
                   CHECK(result[2] == "s2_2_s3");
                   CHECK(result[3] == "s3");
                   REQUIRE(tresult.size() == 1);
                   CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                   CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                   CHECK(tresult[0].source_state<fsm::state>().name() == "s2");
                   CHECK(tresult[0].target_state<fsm::state>().name() == "s3");
                   result.clear();
                   tresult.clear();
                   THEN("s3"){
                       THEN("transition to s2, i.e. transition to s2_2_1"){
                           o1.on_next("a");
                           REQUIRE(result.size() == 7);
                           CHECK(result[0] == "s3_exit");
                           CHECK(result[1] == "s3_2_s2_entry");
                           CHECK(result[2] == "s2");
                           CHECK(result[3] == "s2_entry_2_s2_2");
                           CHECK(result[4] == "s2_2");
                           CHECK(result[5] == "s2_2_initial_2_s2_2_1");
                           CHECK(result[6] == "s2_2_1");
                           REQUIRE(tresult.size() == 3);
                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                           CHECK(tresult[0].source_state<fsm::state>().name() == "s3");
                           CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "s2_entry");
                           CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                           CHECK(tresult[1].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s2_entry");
                           CHECK(tresult[1].target_state<fsm::state>().name() == "s2_2");
                           CHECK(tresult[2].source_state_type() == fsm::transition::pseudo);
                           CHECK(tresult[2].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[2].source_state<fsm::pseudostate>().name() == "s2_2_initial");
                           CHECK(tresult[2].target_state<fsm::state>().name() == "s2_2_1");
                       }
                       THEN("transition to s1"){
                           o3.on_next("a");
                           REQUIRE(result.size() == 3);
                           CHECK(result[0] == "s3_exit");
                           CHECK(result[1] == "s3_2_s1");
                           CHECK(result[2] == "s1");
                           REQUIRE(tresult.size() == 1);
                           CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].target_state_type() == fsm::transition::regular);
                           CHECK(tresult[0].source_state<fsm::state>().name() == "s3");
                           CHECK(tresult[0].target_state<fsm::state>().name() == "s1");
                       }
                       THEN("no transitions"){
                           o5.on_next("a");
                           REQUIRE(result.size() == 0);
                           REQUIRE(tresult.size() == 0);
                       }
                   }
               }
               THEN("no transitions"){
                   o3.on_next("a");
                   o4.on_next("a");
                   o5.on_next("a");
                   REQUIRE(result.size() == 0);
                   REQUIRE(tresult.size() == 0);
               }
            }
        }
        WHEN("no transitions"){
            o2.on_next("a");
            o3.on_next("a");
            o4.on_next("a");
            o5.on_next("a");
            REQUIRE(result.size() == 0);
            REQUIRE(tresult.size() == 0);
        }
    }
}


SCENARIO_METHOD(fsm::string_fixture1, "composite state 2 - timeouts", "[fsm][state]"){
    auto cn = rxcpp::identity_immediate();
    auto test = rxsc::make_test();
    auto w = test.create_worker();
    auto t = rxcpp::observe_on_one_worker(test);
    auto initial = fsm::make_initial_pseudostate("initial");
    FSM_SUBJECT(1);
    std::vector<std::string> result;
    auto s1 = fsm::make_state("s1").with_on_entry([&result]() {result.push_back("s1");});
    auto s2 = fsm::make_state("s2").with_on_entry([&result]() {result.push_back("s2");});
    auto s1_1 = fsm::make_state("s1_1").with_on_entry([&result]() {result.push_back("s1_1");});
    auto s1_2 = fsm::make_state("s1_2").with_on_entry([&result]() {result.push_back("s1_2");});
    auto s1_1_1 = fsm::make_state("s1_1_1").with_on_entry([&result]() {result.push_back("s1_1_1");});
    auto s1_1_2 = fsm::make_state("s1_1_2").with_on_entry([&result]() {result.push_back("s1_1_2");});
    auto s1_initial = fsm::make_initial_pseudostate("s1_initial");
    auto s1_1_initial = fsm::make_initial_pseudostate("s1_1_initial");
    initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
    s1_initial.with_transition("s1_initial_2_s1_1", s1_1, [&result](){result.push_back("s1_initial_2_s1_1");});
    s1_1_initial.with_transition("s1_1_initial_2_s1_1_1", s1_1_1, [&result](){result.push_back("s1_1_initial_2_s1_1_1");});
    s1.with_transition("s1_2_s2", s2, t, std::chrono::seconds(1), [&result](){result.push_back("s1_2_s2");});
    s1_1.with_transition("s1_1_2_s1_2", s1_2, t, std::chrono::milliseconds(500), [&result](){result.push_back("s1_1_2_s1_2");});
    s1_1_1.with_transition("s1_1_1_2_s1_1_2", s1_1_2, obs1, [&result](const std::string&){result.push_back("s1_1_1_2_s1_1_2");}, [](const std::string& s){return s == "a";});
    s1_1_1.with_transition("s1_1_1_2_s1_2", s1_2, obs1, [&result](const std::string&){result.push_back("s1_1_1_2_s1_2");}, [](const std::string& s){return s == "b";});
    sm.with_state(initial, s1, s2);
    s1.with_sub_state(s1_initial, s1_1, s1_2);
    s1_1.with_sub_state(s1_1_initial, s1_1_1, s1_1_2);
    REQUIRE_NOTHROW(sm.start(cn));
    CHECK(!sm.is_terminated());
    auto unreachable_state_names = sm.find_unreachable_states();
    CHECK(unreachable_state_names.size() == 0);
    GIVEN("no interaction"){
        w.advance_by(2000);
        sm.terminate();
        CHECK(sm.is_terminated());
        REQUIRE(result.size() == 10);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        CHECK(result[2] == "s1_initial_2_s1_1");
        CHECK(result[3] == "s1_1");
        CHECK(result[4] == "s1_1_initial_2_s1_1_1");
        CHECK(result[5] == "s1_1_1");
        CHECK(result[6] == "s1_1_2_s1_2");
        CHECK(result[7] == "s1_2");
        CHECK(result[8] == "s1_2_s2");
        CHECK(result[9] == "s2");
    }
    GIVEN("transition to s1_1_2"){
        o1.on_next("a");
        w.advance_by(2000);
        sm.terminate();
        CHECK(sm.is_terminated());
        REQUIRE(result.size() == 12);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        CHECK(result[2] == "s1_initial_2_s1_1");
        CHECK(result[3] == "s1_1");
        CHECK(result[4] == "s1_1_initial_2_s1_1_1");
        CHECK(result[5] == "s1_1_1");
        CHECK(result[6] == "s1_1_1_2_s1_1_2");
        CHECK(result[7] == "s1_1_2");
        CHECK(result[8] == "s1_1_2_s1_2");
        CHECK(result[9] == "s1_2");
        CHECK(result[10] == "s1_2_s2");
        CHECK(result[11] == "s2");
    }
    GIVEN("transition to s1_2"){
        o1.on_next("b");
        w.advance_by(2000);
        sm.terminate();
        CHECK(sm.is_terminated());
        REQUIRE(result.size() == 10);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        CHECK(result[2] == "s1_initial_2_s1_1");
        CHECK(result[3] == "s1_1");
        CHECK(result[4] == "s1_1_initial_2_s1_1_1");
        CHECK(result[5] == "s1_1_1");
        CHECK(result[6] == "s1_1_1_2_s1_2");
        CHECK(result[7] == "s1_2");
        CHECK(result[8] == "s1_2_s2");
        CHECK(result[9] == "s2");
    }
}

SCENARIO_METHOD(fsm::string_fixture2, "sub machine state", "[fsm][state]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    auto sm = fsm::make_state_machine("sm");
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    std::vector<std::string> result;
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    sm.with_state(initial, s1, s2);
    s1.with_on_entry([&result]() {result.push_back("s1");});
    s2.with_on_entry([&result]() {result.push_back("s2");});
    initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
    s1.with_transition("s1_2_s2", s2, obs1, [&result](const std::string&){result.push_back("s1_2_s2");});
    auto sub_initial = fsm::make_initial_pseudostate("sub_initial");
    auto sub_machine = fsm::make_state_machine("sub_machine");
    auto ss1 = fsm::make_state("ss1");
    auto ss2 = fsm::make_state("ss2");
    sub_machine.with_state(sub_initial, ss1, ss2);
    ss1.with_on_entry([&result]() {result.push_back("ss1");});
    ss2.with_on_entry([&result]() {result.push_back("ss2");});
    sub_initial.with_transition("sub_initial_2_ss1", ss1, [&result](){result.push_back("sub_initial_2_ss1");});
    ss1.with_transition("ss1_2_ss2", ss2, obs1, [&result](const std::string&){result.push_back("ss1_2_ss2");});
    GIVEN("already assembled sub machine"){
        CHECK_NOTHROW(sub_machine.start(cn));
        CHECK_THROWS(s1.with_state_machine(sub_machine));
    }
    GIVEN("assemble sub machine"){
        CHECK(s1.is_simple());
        CHECK_NOTHROW(s1.with_state_machine(sub_machine));
        CHECK(s1.is_sub_machine());
        CHECK_THROWS(sub_machine.start(cn));
    }
    GIVEN("simple sub machine"){
        CHECK_NOTHROW(s1.with_state_machine(sub_machine));
        REQUIRE_NOTHROW(sm.start(cn));
        REQUIRE(result.size() == 4);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        CHECK(result[2] == "sub_initial_2_ss1");
        CHECK(result[3] == "ss1");
        result.clear();
        WHEN("transition to ss2"){
            o1.on_next("a");
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "ss1_2_ss2");
            CHECK(result[1] == "ss2");
            result.clear();
            THEN("transition to s2"){
                o1.on_next("a");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s1_2_s2");
                CHECK(result[1] == "s2");
            }
        }
    }
    GIVEN("complex sub machine"){
        auto entry = fsm::make_entry_point_pseudostate("entry");
        auto exit = fsm::make_exit_point_pseudostate("exit");
        auto s3 = fsm::make_state("s3");
        s3.with_on_entry([&result]() {result.push_back("s3");});
        sub_machine.with_state(entry, exit);
        sm.with_state(s3);
        CHECK_NOTHROW(entry.with_transition("entry_2_ss2", ss2, [&result](){result.push_back("entry_2_ss2");}));
        CHECK_NOTHROW(s2.with_transition("s2_2_ss1", ss1, obs1, [&result](const std::string&){result.push_back("s2_2_ss1");}));
        CHECK_NOTHROW(s2.with_transition("s2_2_entry", entry, obs2, [&result](const std::string&){result.push_back("s2_2_entry");}));
        CHECK_NOTHROW(exit.with_transition("exit_2_s3", s3, [&result](){result.push_back("exit_2_s3");}));
        CHECK_NOTHROW(ss2.with_transition("ss2_2_exit", exit, obs2, [&result](const std::string&){result.push_back("ss2_2_exit");}));
        CHECK_NOTHROW(s1.with_state_machine(sub_machine));
        REQUIRE_NOTHROW(sm.start(cn));
        REQUIRE(result.size() == 4);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        CHECK(result[2] == "sub_initial_2_ss1");
        CHECK(result[3] == "ss1");
        result.clear();
        WHEN("transition to ss2"){
            o1.on_next("a");
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "ss1_2_ss2");
            CHECK(result[1] == "ss2");
            result.clear();
            THEN("transition to s2"){
                o1.on_next("a");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s1_2_s2");
                CHECK(result[1] == "s2");
                result.clear();
                THEN("transition to ss1"){
                    o1.on_next("a");
                    REQUIRE(result.size() == 3);
                    CHECK(result[0] == "s2_2_ss1");
                    CHECK(result[1] == "s1");
                    CHECK(result[2] == "ss1");
                }
                THEN("transition to entry, i.e. transition to ss2"){
                    o2.on_next("a");
                    REQUIRE(result.size() == 4);
                    CHECK(result[0] == "s2_2_entry");
                    CHECK(result[1] == "s1");
                    CHECK(result[2] == "entry_2_ss2");
                    CHECK(result[3] == "ss2");
                    result.clear();
                    THEN("transition to exit, i.e. transition to s3"){
                        o2.on_next("a");
                        REQUIRE(result.size() == 3);
                        CHECK(result[0] == "ss2_2_exit");
                        CHECK(result[1] == "exit_2_s3");
                        CHECK(result[2] == "s3");
                    }
                }
            }
        }
    }
}
