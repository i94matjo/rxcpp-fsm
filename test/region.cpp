#include "test.h"

SCENARIO_METHOD(fsm::string_fixture4, "simple region", "[fsm][state][region]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    std::vector<std::string> result;
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    FSM_SUBJECT(4);
    auto s1 = fsm::make_state("s1").with_on_entry([&result]() {result.push_back("s1");});
    auto s2 = fsm::make_state("s2").with_on_entry([&result]() {result.push_back("s2");});
    sm.with_state(initial, s1, s2);
    s1.with_transition("s1_2_s2", s2, obs3, [&result](const std::string&){result.push_back("s1_2_s2");});
    GIVEN("nested region"){
        std::vector<std::string> result11, result12, result111, result112;
        auto r1 = fsm::make_region("r1");
        auto r2 = fsm::make_region("r2");
        auto s11_initial = fsm::make_initial_pseudostate("s11_initial");
        auto s11_1 = fsm::make_state("s11_1").with_on_entry([&result11]() {result11.push_back("s11_1");});
        auto s11_2 = fsm::make_state("s11_2").with_on_entry([&result11]() {result11.push_back("s11_2");});
        auto s21_initial = fsm::make_initial_pseudostate("s21_initial");
        auto s21_1 = fsm::make_state("s21_1").with_on_entry([&result12]() {result12.push_back("s21_1");});
        auto r11 = fsm::make_region("r11");
        auto r12 = fsm::make_region("r12");
        auto s111_initial = fsm::make_initial_pseudostate("s111_initial");
        auto s111_1 = fsm::make_state("s111_1").with_on_entry([&result111]() {result111.push_back("s111_1");});
        auto s112_initial = fsm::make_initial_pseudostate("s112_initial");
        auto s112_1 = fsm::make_state("s112_1").with_on_entry([&result112]() {result112.push_back("s112_1");});
        r11.with_sub_state(s111_initial, s111_1);
        r12.with_sub_state(s112_initial, s112_1);
        s11_1.with_region(r11, r12);
        r1.with_sub_state(s11_initial, s11_1, s11_2);
        r2.with_sub_state(s21_initial, s21_1);
        s1.with_region(r1, r2);
        s11_initial.with_transition("s11_initial_2_s11_1", s11_1, [&result11](){result11.push_back("s11_initial_2_s11_1");});
        s21_initial.with_transition("s21_initial_2_s21_1", s21_1, [&result12](){result12.push_back("s21_initial_2_s21_1");});
        s111_initial.with_transition("s111_initial_2_s111_1", s111_1, [&result111](){result111.push_back("s111_initial_2_s111_1");});
        s112_initial.with_transition("s112_initial_2_s112_1", s112_1, [&result112](){result112.push_back("s112_initial_2_s112_1");});
        s111_1.with_transition("s111_1_2_s11_2", s11_2, obs1, [&result111](const std::string&){result111.push_back("s111_1_2_s11_2");});
        s112_1.with_transition("s112_1_2_s11_2", s11_2, obs2, [&result112](const std::string&){result112.push_back("s112_1_2_s11_2");});
        WHEN("transition between regions"){
            initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
            s11_1.with_transition("s11_1_2_s21_1", s21_1, obs1);
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("transition between regions (2)"){
            initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
            s21_1.with_transition("s21_1_2_s111_1", s111_1, obs1);
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("transition between regions (3)"){
            initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
            s111_1.with_transition("s111_1_2_s21_1", s21_1, obs1);
            CHECK_THROWS(sm.start(cn));
        }
        WHEN("transition out of orthogonal state"){
            initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
            s111_1.with_on_exit([&result111]() {result111.push_back("xs111_1");});
            s112_1.with_on_exit([&result112]() {result112.push_back("xs112_1");});
            REQUIRE_NOTHROW(sm.start(cn));
            auto unreachable_state_names = sm.find_unreachable_states();
            CHECK(unreachable_state_names.size() == 0);
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "initial_2_s1");
            CHECK(result[1] == "s1");
            REQUIRE(result11.size() == 2);
            CHECK(result11[0] == "s11_initial_2_s11_1");
            CHECK(result11[1] == "s11_1");
            REQUIRE(result12.size() == 2);
            CHECK(result12[0] == "s21_initial_2_s21_1");
            CHECK(result12[1] == "s21_1");
            REQUIRE(result111.size() == 2);
            CHECK(result111[0] == "s111_initial_2_s111_1");
            CHECK(result111[1] == "s111_1");
            REQUIRE(result112.size() == 2);
            CHECK(result112[0] == "s112_initial_2_s112_1");
            CHECK(result112[1] == "s112_1");
            result.clear();
            result11.clear();
            result12.clear();
            result111.clear();
            result112.clear();
            THEN("transitioning out from s111"){
                o1.on_next("a");
                REQUIRE(result111.size() == 2);
                CHECK(result111[0] == "xs111_1");
                CHECK(result111[1] == "s111_1_2_s11_2");
                REQUIRE(result112.size() == 1);
                CHECK(result112[0] == "xs112_1");
                REQUIRE(result11.size() == 1);
                CHECK(result11[0] == "s11_2");
                CHECK(result.size() == 0);
            }
            THEN("transitioning out from s112"){
                o2.on_next("a");
                REQUIRE(result112.size() == 2);
                CHECK(result112[0] == "xs112_1");
                CHECK(result112[1] == "s112_1_2_s11_2");
                REQUIRE(result111.size() == 1);
                CHECK(result111[0] == "xs111_1");
                REQUIRE(result11.size() == 1);
                CHECK(result11[0] == "s11_2");
                CHECK(result.size() == 0);
            }
        }
        WHEN("transition to history states"){
            initial.with_transition("initial_2_s2", s2, [&result](){result.push_back("initial_2_s2");});
            auto s11_deep = fsm::make_deep_history_pseudostate("s11_deep");
            auto s11_shallow = fsm::make_shallow_history_pseudostate("s11_shallow");
            auto s21_shallow = fsm::make_shallow_history_pseudostate("s21_shallow");
            auto deep_fork = fsm::make_fork_pseudostate("deep_fork");
            auto shallow_fork = fsm::make_fork_pseudostate("shallow_fork");
            auto s111_2 = fsm::make_state("s111_2").with_on_entry([&result111]() {result111.push_back("s111_2");});
            sm.with_state(deep_fork, shallow_fork);
            r1.with_sub_state(s11_deep, s11_shallow);
            r2.with_sub_state(s21_shallow);
            r11.with_sub_state(s111_2);
            s2.with_transition("s2_2_deep_fork", deep_fork, obs1, [&result](const std::string&){result.push_back("s2_2_deep_fork");});
            s2.with_transition("s2_2_shallow_fork", shallow_fork, obs2, [&result](const std::string&){result.push_back("s2_2_shallow_fork");});
            s2.with_transition("s2_2_s1", s1, obs3, [&result](const std::string&){result.push_back("s2_2_s1");});
            deep_fork.with_transition("deep_fork_2_s11_deep", s11_deep, [&result11](){result11.push_back("deep_fork_2_s11_deep");});
            deep_fork.with_transition("deep_fork_2_s21_shallow", s21_shallow, [&result12](){result12.push_back("deep_fork_2_s21_shallow");});
            shallow_fork.with_transition("shallow_fork_2_s11_shallow", s11_shallow, [&result11](){result11.push_back("shallow_fork_2_s11_shallow");});
            shallow_fork.with_transition("shallow_fork_2_s21_shallow", s21_shallow, [&result12](){result12.push_back("shallow_fork_2_s21_shallow");});
            s111_1.with_transition("s111_1_2_s111_2", s111_2, obs4, [&result111](const std::string&){result111.push_back("s111_1_2_s111_2");});
            REQUIRE_NOTHROW(sm.start(cn));
            auto unreachable_state_names = sm.find_unreachable_states();
            CHECK(unreachable_state_names.size() == 0);
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "initial_2_s2");
            CHECK(result[1] == "s2");
            REQUIRE(result11.size() == 0);
            REQUIRE(result12.size() == 0);
            REQUIRE(result111.size() == 0);
            REQUIRE(result112.size() == 0);
            result.clear();
            THEN("transition to deep fork"){
                o1.on_next("a");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s2_2_deep_fork");
                CHECK(result[1] == "s1");
                REQUIRE(result11.size() == 3);
                CHECK(result11[0] == "deep_fork_2_s11_deep");
                CHECK(result11[1] == "s11_initial_2_s11_1");
                CHECK(result11[2] == "s11_1");
                REQUIRE(result12.size() == 3);
                CHECK(result12[0] == "deep_fork_2_s21_shallow");
                CHECK(result12[1] == "s21_initial_2_s21_1");
                CHECK(result12[2] == "s21_1");
                REQUIRE(result111.size() == 2);
                CHECK(result111[0] == "s111_initial_2_s111_1");
                CHECK(result111[1] == "s111_1");
                REQUIRE(result112.size() == 2);
                CHECK(result112[0] == "s112_initial_2_s112_1");
                CHECK(result112[1] == "s112_1");
            }
            THEN("transition to shallow fork"){
                o2.on_next("a");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s2_2_shallow_fork");
                CHECK(result[1] == "s1");
                REQUIRE(result11.size() == 3);
                CHECK(result11[0] == "shallow_fork_2_s11_shallow");
                CHECK(result11[1] == "s11_initial_2_s11_1");
                CHECK(result11[2] == "s11_1");
                REQUIRE(result12.size() == 3);
                CHECK(result12[0] == "shallow_fork_2_s21_shallow");
                CHECK(result12[1] == "s21_initial_2_s21_1");
                CHECK(result12[2] == "s21_1");
                REQUIRE(result111.size() == 2);
                CHECK(result111[0] == "s111_initial_2_s111_1");
                CHECK(result111[1] == "s111_1");
                REQUIRE(result112.size() == 2);
                CHECK(result112[0] == "s112_initial_2_s112_1");
                CHECK(result112[1] == "s112_1");
            }
            THEN("transition to s1"){
                o3.on_next("a");
                REQUIRE(result.size() == 2);
                CHECK(result[0] == "s2_2_s1");
                CHECK(result[1] == "s1");
                REQUIRE(result11.size() == 2);
                CHECK(result11[0] == "s11_initial_2_s11_1");
                CHECK(result11[1] == "s11_1");
                REQUIRE(result12.size() == 2);
                CHECK(result12[0] == "s21_initial_2_s21_1");
                CHECK(result12[1] == "s21_1");
                REQUIRE(result111.size() == 2);
                CHECK(result111[0] == "s111_initial_2_s111_1");
                CHECK(result111[1] == "s111_1");
                REQUIRE(result112.size() == 2);
                CHECK(result112[0] == "s112_initial_2_s112_1");
                CHECK(result112[1] == "s112_1");
                result.clear();
                result11.clear();
                result12.clear();
                result111.clear();
                result112.clear();
                THEN("transition to s111_2"){
                    o4.on_next("a");
                    REQUIRE(result111.size() == 2);
                    CHECK(result111[0] == "s111_1_2_s111_2");
                    CHECK(result111[1] == "s111_2");
                    REQUIRE(result.size() == 0);
                    REQUIRE(result11.size() == 0);
                    REQUIRE(result12.size() == 0);
                    REQUIRE(result112.size() == 0);
                    result111.clear();
                    THEN("transition to s2"){
                        o3.on_next("a");
                        REQUIRE(result.size() == 2);
                        CHECK(result[0] == "s1_2_s2");
                        CHECK(result[1] == "s2");
                        REQUIRE(result11.size() == 0);
                        REQUIRE(result12.size() == 0);
                        REQUIRE(result111.size() == 0);
                        REQUIRE(result112.size() == 0);
                        result.clear();
                        THEN("transition to deep fork"){
                            o1.on_next("a");
                            REQUIRE(result.size() == 2);
                            CHECK(result[0] == "s2_2_deep_fork");
                            CHECK(result[1] == "s1");
                            REQUIRE(result11.size() == 2);
                            CHECK(result11[0] == "deep_fork_2_s11_deep");
                            CHECK(result11[1] == "s11_1");
                            REQUIRE(result12.size() == 2);
                            CHECK(result12[0] == "deep_fork_2_s21_shallow");
                            CHECK(result12[1] == "s21_1");
                            REQUIRE(result111.size() == 1);
                            CHECK(result111[0] == "s111_2");
                            REQUIRE(result112.size() == 1);
                            CHECK(result112[0] == "s112_1");
                        }
                        THEN("transition to shallow fork"){
                            o2.on_next("a");
                            REQUIRE(result.size() == 2);
                            CHECK(result[0] == "s2_2_shallow_fork");
                            CHECK(result[1] == "s1");
                            REQUIRE(result11.size() == 2);
                            CHECK(result11[0] == "shallow_fork_2_s11_shallow");
                            CHECK(result11[1] == "s11_1");
                            REQUIRE(result12.size() == 2);
                            CHECK(result12[0] == "shallow_fork_2_s21_shallow");
                            CHECK(result12[1] == "s21_1");
                            REQUIRE(result111.size() == 2);
                            CHECK(result111[0] == "s111_initial_2_s111_1");
                            CHECK(result111[1] == "s111_1");
                            REQUIRE(result112.size() == 2);
                            CHECK(result112[0] == "s112_initial_2_s112_1");
                            CHECK(result112[1] == "s112_1");
                        }
                    }
                }
            }
        }
    }
}


SCENARIO_METHOD(fsm::string_fixture6, "region 1", "[fsm][state][region]"){
    auto cn = rxcpp::identity_immediate();
    auto initial = fsm::make_initial_pseudostate("initial");
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    FSM_SUBJECT(4);
    FSM_SUBJECT(5);
    FSM_SUBJECT(6);
    std::vector<std::string> result, result21, result22, result23;
    auto s1 = fsm::make_state("s1").with_on_entry([&result]() {result.push_back("s1");});
    auto s2 = fsm::make_state("s2").with_on_entry([&result]() {result.push_back("s2");});
    auto s3 = fsm::make_state("s3").with_on_entry([&result]() {result.push_back("s3");});
    auto final = fsm::make_final_state("final");
    auto r1 = fsm::make_region("r1");
    auto r2 = fsm::make_region("r2");
    auto r3 = fsm::make_region("r3");
    s2.with_region(r1, r2, r3);
    auto s21_initial = fsm::make_initial_pseudostate("s21_initial");
    auto s21_1 = fsm::make_state("s21_1").with_on_entry([&result21]() {result21.push_back("s21_1");});
    auto s21_2 = fsm::make_state("s21_2").with_on_entry([&result21]() {result21.push_back("s21_2");});
    auto s21_final = fsm::make_final_state("s21_final");
    auto s22_initial = fsm::make_initial_pseudostate("s22_initial");
    auto s22_1 = fsm::make_state("s22_1").with_on_entry([&result22]() {result22.push_back("s22_1");});
    auto s22_2 = fsm::make_state("s22_2").with_on_entry([&result22]() {result22.push_back("s22_2");});
    auto s22_final = fsm::make_final_state("s22_final");
    auto s23_initial = fsm::make_initial_pseudostate("s23_initial");
    auto s23_1 = fsm::make_state("s23_1").with_on_entry([&result23]() {result23.push_back("s23_1");});
    auto s23_2 = fsm::make_state("s23_2").with_on_entry([&result23]() {result23.push_back("s23_2");});
    auto s23_final = fsm::make_final_state("s23_final");
    r1.with_sub_state(s21_initial, s21_1, s21_2, s21_final);
    r2.with_sub_state(s22_initial, s22_1, s22_2, s22_final);
    r3.with_sub_state(s23_initial, s23_1, s23_2, s23_final);
    auto join = fsm::make_join_pseudostate("join");
    auto fork = fsm::make_fork_pseudostate("fork");
    sm.with_state(initial, s1, s2, s3, join, fork, final);
    initial.with_transition("initial_2_s1", s1, [&result](){result.push_back("initial_2_s1");});
    s1.with_transition("s1_2_s2", s2, obs2, [&result](const std::string&){result.push_back("s1_2_s2");});
    s1.with_transition("s1_2_fork", fork, obs1, [&result](const std::string&){result.push_back("s1_2_fork");});
    s2.with_transition("s2_2_s3", s3, [&result](){result.push_back("s2_2_s3");});
    fork.with_transition("fork_2_s21_2", s21_2, [&result21](){result21.push_back("fork_2_s21_2");});
    fork.with_transition("fork_2_s22_2", s22_2, [&result22](){result22.push_back("fork_2_s22_2");});
    fork.with_transition("fork_2_s23_2", s23_2, [&result23](){result23.push_back("fork_2_s23_2");});
    s21_initial.with_transition("s21_initial_2_s21_1", s21_1, [&result21](){result21.push_back("s21_initial_2_s21_1");});
    s21_1.with_transition("s21_1_2_s21_2", s21_2, obs1, [&result21](const std::string&){result21.push_back("s21_1_2_s21_2");});
    s21_2.with_transition("s21_2_2_s21_final", s21_final, obs1, [&result21](const std::string&){result21.push_back("s21_2_2_s21_final");});
    s21_2.with_transition("s21_2_2_join", join, obs4, [&result21](const std::string&){result21.push_back("s21_2_2_join");});
    s22_initial.with_transition("s22_initial_2_s22_1", s22_1, [&result22](){result22.push_back("s22_initial_2_s22_1");});
    s22_1.with_transition("s22_1_2_s22_2", s22_2, obs1, [&result22](const std::string&){result22.push_back("s22_1_2_s22_2");});
    s22_2.with_transition("s22_2_2_s22_final", s22_final, obs2, [&result22](const std::string&){result22.push_back("s22_2_2_s22_final");});
    s22_2.with_transition("s22_2_2_join", join, obs5, [&result22](const std::string&){result22.push_back("s22_2_2_join");});
    s23_initial.with_transition("s23_initial_2_s23_1", s23_1, [&result23](){result23.push_back("s23_initial_2_s23_1");});
    s23_1.with_transition("s23_1_2_s23_2", s23_2, obs1, [&result23](const std::string&){result23.push_back("s23_1_2_s23_2");});
    s23_2.with_transition("s23_2_2_s23_final", s23_final, obs3, [&result23](const std::string&){result23.push_back("s23_2_2_s23_final");});
    s23_2.with_transition("s23_2_2_join", join, obs6, [&result23](const std::string&){result23.push_back("s23_2_2_join");});
    join.with_transition("join_2_final", final, [&result](){result.push_back("join_2_final");});
    std::vector<fsm::transition> tresult;
    REQUIRE_NOTHROW(sm.assemble(cn).subscribe([&tresult](const fsm::transition& t){tresult.push_back(t);}));
    CHECK(!sm.is_terminated());
    auto unreachable_state_names = sm.find_unreachable_states();
    CHECK(unreachable_state_names.size() == 0);
    GIVEN("s1"){
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1");
        result.clear();
        tresult.clear();
        WHEN("transition to s2"){
            o2.on_next("a");
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "s1_2_s2");
            CHECK(result[1] == "s2");
            REQUIRE(result21.size() == 2);
            CHECK(result21[0] == "s21_initial_2_s21_1");
            CHECK(result21[1] == "s21_1");
            REQUIRE(result22.size() == 2);
            CHECK(result22[0] == "s22_initial_2_s22_1");
            CHECK(result22[1] == "s22_1");
            REQUIRE(result23.size() == 2);
            CHECK(result23[0] == "s23_initial_2_s23_1");
            CHECK(result23[1] == "s23_1");
            REQUIRE(tresult.size() == 4);
            CHECK(tresult[0].source_state_type() == fsm::transition::regular);
            CHECK(tresult[0].target_state_type() == fsm::transition::regular);
            CHECK(tresult[0].source_state<fsm::state>().name() == "s1");
            CHECK(tresult[0].target_state<fsm::state>().name() == "s2");
            CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[1].target_state_type() == fsm::transition::regular);
            CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "s21_initial");
            CHECK(tresult[1].target_state<fsm::state>().name() == "s21_1");
            CHECK(tresult[2].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[2].target_state_type() == fsm::transition::regular);
            CHECK(tresult[2].source_state<fsm::pseudostate>().name() == "s22_initial");
            CHECK(tresult[2].target_state<fsm::state>().name() == "s22_1");
            CHECK(tresult[3].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[3].target_state_type() == fsm::transition::regular);
            CHECK(tresult[3].source_state<fsm::pseudostate>().name() == "s23_initial");
            CHECK(tresult[3].target_state<fsm::state>().name() == "s23_1");
            result.clear();
            result21.clear();
            result22.clear();
            result23.clear();
            tresult.clear();
            THEN("transition to s21_2/s22_2/s23_2"){
                o1.on_next("a");
                REQUIRE(result.size() == 0);
                REQUIRE(result21.size() == 2);
                CHECK(result21[0] == "s21_1_2_s21_2");
                CHECK(result21[1] == "s21_2");
                REQUIRE(result22.size() == 2);
                CHECK(result22[0] == "s22_1_2_s22_2");
                CHECK(result22[1] == "s22_2");
                REQUIRE(result23.size() == 2);
                CHECK(result23[0] == "s23_1_2_s23_2");
                CHECK(result23[1] == "s23_2");
                result21.clear();
                result22.clear();
                result23.clear();
                THEN("transition to s21_final"){
                    o1.on_next("a");
                    REQUIRE(result.size() == 0);
                    REQUIRE(result22.size() == 0);
                    REQUIRE(result23.size() == 0);
                    REQUIRE(result21.size() == 1);
                    CHECK(result21[0] == "s21_2_2_s21_final");
                    result21.clear();
                    THEN("transition to s22_final"){
                        o2.on_next("a");
                        REQUIRE(result.size() == 0);
                        REQUIRE(result21.size() == 0);
                        REQUIRE(result23.size() == 0);
                        REQUIRE(result22.size() == 1);
                        CHECK(result22[0] == "s22_2_2_s22_final");
                        result22.clear();
                        THEN("transition 2 s23_final, i.e. transition to s3"){
                            o3.on_next("a");
                            REQUIRE(result21.size() == 0);
                            REQUIRE(result22.size() == 0);
                            REQUIRE(result23.size() == 1);
                            CHECK(result23[0] == "s23_2_2_s23_final");
                            REQUIRE(result.size() == 2);
                            CHECK(result[0] == "s2_2_s3");
                            CHECK(result[1] == "s3");
                        }
                    }
                }
            }
        }
        WHEN("transition to fork. i.e. transition to s21_2/s22_2/s23_2"){
            o1.on_next("a");
            REQUIRE(result.size() == 2);
            CHECK(result[0] == "s1_2_fork");
            CHECK(result[1] == "s2");
            REQUIRE(result21.size() == 2);
            CHECK(result21[0] == "fork_2_s21_2");
            CHECK(result21[1] == "s21_2");
            REQUIRE(result22.size() == 2);
            CHECK(result22[0] == "fork_2_s22_2");
            CHECK(result22[1] == "s22_2");
            REQUIRE(result23.size() == 2);
            CHECK(result23[0] == "fork_2_s23_2");
            CHECK(result23[1] == "s23_2");
            REQUIRE(tresult.size() == 4);
            CHECK(tresult[0].source_state_type() == fsm::transition::regular);
            CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
            CHECK(tresult[0].source_state<fsm::state>().name() == "s1");
            CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "fork");
            CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[1].target_state_type() == fsm::transition::regular);
            CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "fork");
            CHECK(tresult[1].target_state<fsm::state>().name() == "s21_2");
            CHECK(tresult[2].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[2].target_state_type() == fsm::transition::regular);
            CHECK(tresult[2].source_state<fsm::pseudostate>().name() == "fork");
            CHECK(tresult[2].target_state<fsm::state>().name() == "s22_2");
            CHECK(tresult[3].source_state_type() == fsm::transition::pseudo);
            CHECK(tresult[3].target_state_type() == fsm::transition::regular);
            CHECK(tresult[3].source_state<fsm::pseudostate>().name() == "fork");
            CHECK(tresult[3].target_state<fsm::state>().name() == "s23_2");
            result.clear();
            result21.clear();
            result22.clear();
            result23.clear();
            tresult.clear();
            THEN("transition to join from s21_2, i.e join not entered"){
                o4.on_next("a");
                REQUIRE(result.size() == 0);
                REQUIRE(result22.size() == 0);
                REQUIRE(result23.size() == 0);
                REQUIRE(result21.size() == 1);
                CHECK(result21[0] == "s21_2_2_join");
                REQUIRE(tresult.size() == 1);
                CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                CHECK(tresult[0].source_state<fsm::state>().name() == "s21_2");
                CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "join");
                result21.clear();
                tresult.clear();
                THEN("transition to join from s22_2, i.e. join not entered"){
                    o5.on_next("a");
                    REQUIRE(result.size() == 0);
                    REQUIRE(result21.size() == 0);
                    REQUIRE(result23.size() == 0);
                    REQUIRE(result22.size() == 1);
                    CHECK(result22[0] == "s22_2_2_join");
                    REQUIRE(tresult.size() == 1);
                    CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                    CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                    CHECK(tresult[0].source_state<fsm::state>().name() == "s22_2");
                    CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "join");
                    result22.clear();
                    tresult.clear();
                    THEN("transition to join from s23_2, i.e. join entered, i.e. transition to final"){
                        o6.on_next("a");
                        REQUIRE(result21.size() == 0);
                        REQUIRE(result22.size() == 0);
                        REQUIRE(result23.size() == 1);
                        CHECK(result23[0] == "s23_2_2_join");
                        REQUIRE(result.size() == 1);
                        CHECK(result[0] == "join_2_final");
                        REQUIRE(tresult.size() == 2);
                        CHECK(tresult[0].source_state_type() == fsm::transition::regular);
                        CHECK(tresult[0].target_state_type() == fsm::transition::pseudo);
                        CHECK(tresult[0].source_state<fsm::state>().name() == "s23_2");
                        CHECK(tresult[0].target_state<fsm::pseudostate>().name() == "join");
                        CHECK(tresult[1].source_state_type() == fsm::transition::pseudo);
                        CHECK(tresult[1].target_state_type() == fsm::transition::final);
                        CHECK(tresult[1].source_state<fsm::pseudostate>().name() == "join");
                        CHECK(tresult[1].target_state<fsm::final_state>().name() == "final");
                    }
                }
            }
        }
    }
}
