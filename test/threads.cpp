#include "test.h"

//#define PRINT_TID_INFO
//#define VALGRIND

namespace {

void report_tid(std::set<std::thread::id>& coordination_tids, std::set<std::thread::id>& tids) {
   auto tid = std::this_thread::get_id();
   tids.insert(tid);
   auto it = coordination_tids.find(tid);
   if (it == coordination_tids.end()) {
       WARN(tid << " is not a coordination thread");
   }
}

template<class Coordination>
void tids_init(const Coordination& cn, std::set<std::thread::id>& coordination_tids) {
    for(int i = 0; i < 32; i++)
    {
        auto o = rxcpp::observable<>::range(i, i, cn).as_blocking();
        std::atomic<bool> complete(false);
        o.subscribe([&coordination_tids](int){coordination_tids.insert(std::this_thread::get_id());}, [&complete](){complete = true;});
        while(!complete)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
#ifdef PRINT_TID_INFO
    for(const auto& tid : coordination_tids)
    {
        std::cout << "coordination thread: " << tid << std::endl;
    }
    std::cout << "this thread: " << std::this_thread::get_id() << std::endl;
#endif
}

}

#define REPORT_TID \
{ \
    report_tid(coordination_tids, tids); \
}

#define TIDS_INIT \
std::set<std::thread::id> coordination_tids, tids; \
tids_init(cn, coordination_tids);

#define TRANSITION_ACTION(...) [&tids, &coordination_tids]( __VA_ARGS__ ) { REPORT_TID }

#define COODINATION rxcpp::serialize_event_loop()

#define TM_COORDINATION rxcpp::identity_current_thread()


SCENARIO_METHOD(fsm::string_fixture5, "simple state - threads", "[fsm][state][threads]"){
    auto cn = COODINATION;
    auto t = TM_COORDINATION;
    TIDS_INIT
    auto initial = fsm::make_initial_pseudostate("initial");
    sm.with_state(initial);
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    FSM_SUBJECT(4);
    FSM_SUBJECT(5);
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    auto s3 = fsm::make_state("s3");
    auto s4 = fsm::make_state("s4");
    auto s5 = fsm::make_state("s5");
    sm.with_state(s1, s2, s3, s4, s5);
    initial.with_transition("initial_2_s1", s1, TRANSITION_ACTION());
    std::vector<std::string> result;
    GIVEN("completion transitions"){
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, TRANSITION_ACTION()));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, TRANSITION_ACTION()));
        CHECK_NOTHROW(s3.with_transition("s3_2_s4", s4, TRANSITION_ACTION()));
        CHECK_NOTHROW(s4.with_transition("s4_2_s5", s5, TRANSITION_ACTION()));
        auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
        o.subscribe([&result](const std::string& s){result.push_back(s);});
#ifdef VALGRIND
        while(result.size() < 5)
        {
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        REQUIRE(result.size() == 5);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1_2_s2");
        CHECK(result[2] == "s2_2_s3");
        CHECK(result[3] == "s3_2_s4");
        CHECK(result[4] == "s4_2_s5");
    }
    GIVEN("triggered transitions"){
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, obs1, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, obs2, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s3.with_transition("s3_2_s4", s4, obs3, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s4.with_transition("s4_2_s5", s5, obs4, TRANSITION_ACTION(const std::string&)));
        auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
        o.subscribe([&result](const std::string& s){result.push_back(s);});
#ifdef VALGRIND
        while(result.size() < 1)
        {
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "initial_2_s1");
        result.clear();
        o1.on_next("a");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_2_s2");
        result.clear();
        o2.on_next("b");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s2_2_s3");
        result.clear();
        o3.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s3_2_s4");
        result.clear();
        o4.on_next("d");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s4_2_s5");
    }
    GIVEN("timeout transitions"){
        bool guard1(true);
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, t, std::chrono::milliseconds(500),
            [&guard1, &coordination_tids, &tids]() {
                REPORT_TID
                guard1 = false;
            },
            [&guard1, &coordination_tids, &tids]() {
                REPORT_TID
                return guard1;
            }));
        CHECK_NOTHROW(s2.with_transition("s2_2_s4", s4, obs1, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, t, std::chrono::seconds(1), TRANSITION_ACTION()));
        CHECK_NOTHROW(s4.with_transition("s4_2_s1", s1, t, std::chrono::milliseconds(500), TRANSITION_ACTION()));
        auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
        o.subscribe([&result](const std::string& s){result.push_back(s);});
        std::this_thread::sleep_for(std::chrono::seconds(1));
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1_2_s2");
        result.clear();
        o1.on_next("a");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "s2_2_s4");
        CHECK(result[1] == "s4_2_s1");
        result.clear();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        REQUIRE(result.size() == 0);
        CHECK(!guard1);
    }
    GIVEN("internal transitions"){
        CHECK_NOTHROW(s1.with_transition("s1_internal_1", obs1, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s1.with_transition("s1_internal_2", obs2, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s1.with_transition("s1_internal_3", obs3, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s1.with_transition("s1_internal_4", obs4, TRANSITION_ACTION(const std::string&)));
        auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
        o.subscribe([&result](const std::string& s){result.push_back(s);});
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "initial_2_s1");
        result.clear();
        o1.on_next("a");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_1");
        result.clear();
        o2.on_next("b");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_2");
        result.clear();
        o3.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_3");
        result.clear();
        o4.on_next("d");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_4");
    }
    GIVEN("internal timeout transitions"){
        bool guard1(true), guard2(true);
        CHECK_NOTHROW(s1.with_transition("s1_internal_1", t, std::chrono::milliseconds(50),
            [&guard1, &coordination_tids, &tids]() {
                REPORT_TID
                guard1 = false;
            },
            [&guard1, &coordination_tids, &tids]() {
                REPORT_TID
                return guard1;
            }));
        CHECK_NOTHROW(s1.with_transition("s1_internal_2", t, std::chrono::milliseconds(500),
            [&guard2, &coordination_tids, &tids]() {
                REPORT_TID
                guard2 = false;
            },
            [&guard2, &coordination_tids, &tids]() {
                REPORT_TID
                return guard2;
            }));
        auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
        o.subscribe([&result](const std::string& s){result.push_back(s);});
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 2);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1_internal_1");
        result.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_internal_2");
        result.clear();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        REQUIRE(result.size() == 0);
        CHECK(!guard1);
        CHECK(!guard2);
    }
    GIVEN("final state"){
        auto final = fsm::make_final_state("final");
        sm.with_state(final);
        auto terminated = std::make_shared<bool>(false);
        CHECK_NOTHROW(s1.with_transition("s1_2_s2", s2, obs1, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s2.with_transition("s2_2_final", final, obs2, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s2.with_transition("s2_2_s3", s3, obs3, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s2.with_transition("s2_2_s4", s4, obs4, TRANSITION_ACTION(const std::string&)));
        CHECK_NOTHROW(s2.with_transition("s2_2_s5", s5, obs5, TRANSITION_ACTION(const std::string&)));
        auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
        o.subscribe([&result](const std::string& s){result.push_back(s);}, [terminated](){*terminated = true;});
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(!(*terminated));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "initial_2_s1");
        result.clear();
        o1.on_next("a");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s1_2_s2");
        result.clear();
        o2.on_next("b");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "s2_2_final");
        result.clear();
        REQUIRE(!(*terminated));
        o1.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o2.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o3.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o4.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
        o5.on_next("c");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result.size() == 0);
        REQUIRE(!(*terminated));
    }
}


SCENARIO_METHOD(fsm::string_fixture1, "composite state 2 - timeouts, threads", "[fsm][state][threads]"){
    auto cn = COODINATION;
    auto t = TM_COORDINATION;
    TIDS_INIT;
    auto initial = fsm::make_initial_pseudostate("initial");
    FSM_SUBJECT(1);
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    auto s1_1 = fsm::make_state("s1_1");
    auto s1_2 = fsm::make_state("s1_2");
    auto s1_1_1 = fsm::make_state("s1_1_1");
    auto s1_1_2 = fsm::make_state("s1_1_2");
    auto s1_initial = fsm::make_initial_pseudostate("s1_initial");
    auto s1_1_initial = fsm::make_initial_pseudostate("s1_1_initial");
    initial.with_transition("initial_2_s1", s1, TRANSITION_ACTION());
    s1_initial.with_transition("s1_initial_2_s1_1", s1_1, TRANSITION_ACTION());
    s1_1_initial.with_transition("s1_1_initial_2_s1_1_1", s1_1_1, TRANSITION_ACTION());
    s1.with_transition("s1_2_s2", s2, t, std::chrono::seconds(1), TRANSITION_ACTION());
    s1_1.with_transition("s1_1_2_s1_2", s1_2, t, std::chrono::milliseconds(500), TRANSITION_ACTION());
    s1_1_1.with_transition("s1_1_1_2_s1_1_2", s1_1_2, obs1, TRANSITION_ACTION(const std::string&), [](const std::string& s){return s == "a";});
    s1_1_1.with_transition("s1_1_1_2_s1_2", s1_2, obs1, TRANSITION_ACTION(const std::string&), [](const std::string& s){return s == "b";});
    sm.with_state(initial, s1, s2);
    s1.with_sub_state(s1_initial, s1_1, s1_2);
    s1_1.with_sub_state(s1_1_initial, s1_1_1, s1_1_2);
    auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
    std::vector<std::string> result;
    o.subscribe([&result](const std::string& s){result.push_back(s);});
    GIVEN("no interaction"){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        sm.terminate();
        CHECK(sm.is_terminated());
        REQUIRE(result.size() == 5);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1_initial_2_s1_1");
        CHECK(result[2] == "s1_1_initial_2_s1_1_1");
        CHECK(result[3] == "s1_1_2_s1_2");
        CHECK(result[4] == "s1_2_s2");
    }
    GIVEN("transition to s1_1_2"){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        o1.on_next("a");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        sm.terminate();
        CHECK(sm.is_terminated());
        REQUIRE(result.size() == 6);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1_initial_2_s1_1");
        CHECK(result[2] == "s1_1_initial_2_s1_1_1");
        CHECK(result[3] == "s1_1_1_2_s1_1_2");
        CHECK(result[4] == "s1_1_2_s1_2");
        CHECK(result[5] == "s1_2_s2");
    }
    GIVEN("transition to s1_2"){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        o1.on_next("b");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        sm.terminate();
        CHECK(sm.is_terminated());
        REQUIRE(result.size() == 5);
        CHECK(result[0] == "initial_2_s1");
        CHECK(result[1] == "s1_initial_2_s1_1");
        CHECK(result[2] == "s1_1_initial_2_s1_1_1");
        CHECK(result[3] == "s1_1_1_2_s1_2");
        CHECK(result[4] == "s1_2_s2");
    }
}


SCENARIO_METHOD(fsm::string_fixture6, "region 1 - threads", "[fsm][state][region][threads]"){
    auto cn = COODINATION;
    TIDS_INIT;
    auto initial = fsm::make_initial_pseudostate("initial");
    FSM_SUBJECT(1);
    FSM_SUBJECT(2);
    FSM_SUBJECT(3);
    FSM_SUBJECT(4);
    FSM_SUBJECT(5);
    FSM_SUBJECT(6);
    auto s1 = fsm::make_state("s1");
    auto s2 = fsm::make_state("s2");
    auto s3 = fsm::make_state("s3");
    auto final = fsm::make_final_state("final");
    auto r1 = fsm::make_region("r1");
    auto r2 = fsm::make_region("r2");
    auto r3 = fsm::make_region("r3");
    s2.with_region(r1, r2, r3);
    auto s21_initial = fsm::make_initial_pseudostate("s21_initial");
    auto s21_1 = fsm::make_state("s21_1");
    auto s21_2 = fsm::make_state("s21_2");
    auto s21_final = fsm::make_final_state("s21_final");
    auto s22_initial = fsm::make_initial_pseudostate("s22_initial");
    auto s22_1 = fsm::make_state("s22_1");
    auto s22_2 = fsm::make_state("s22_2");
    auto s22_final = fsm::make_final_state("s22_final");
    auto s23_initial = fsm::make_initial_pseudostate("s23_initial");
    auto s23_1 = fsm::make_state("s23_1");
    auto s23_2 = fsm::make_state("s23_2");
    auto s23_final = fsm::make_final_state("s23_final");
    r1.with_sub_state(s21_initial, s21_1, s21_2, s21_final);
    r2.with_sub_state(s22_initial, s22_1, s22_2, s22_final);
    r3.with_sub_state(s23_initial, s23_1, s23_2, s23_final);
    auto join = fsm::make_join_pseudostate("join");
    auto fork = fsm::make_fork_pseudostate("fork");
    sm.with_state(initial, s1, s2, s3, join, fork, final);
    initial.with_transition("initial_2_s1", s1, TRANSITION_ACTION());
    s1.with_transition("s1_2_s2", s2, obs2, TRANSITION_ACTION(const std::string&));
    s1.with_transition("s1_2_fork", fork, obs1, TRANSITION_ACTION(const std::string&));
    s2.with_transition("s2_2_s3", s3, TRANSITION_ACTION());
    fork.with_transition("fork_2_s21_2", s21_2, TRANSITION_ACTION());
    fork.with_transition("fork_2_s22_2", s22_2, TRANSITION_ACTION());
    fork.with_transition("fork_2_s23_2", s23_2, TRANSITION_ACTION());
    s21_initial.with_transition("s21_initial_2_s21_1", s21_1, TRANSITION_ACTION());
    s21_1.with_transition("s21_1_2_s21_2", s21_2, obs1, TRANSITION_ACTION(const std::string&));
    s21_2.with_transition("s21_2_2_s21_final", s21_final, obs1, TRANSITION_ACTION(const std::string&));
    s21_2.with_transition("s21_2_2_join", join, obs4, TRANSITION_ACTION(const std::string&));
    s22_initial.with_transition("s22_initial_2_s22_1", s22_1, TRANSITION_ACTION());
    s22_1.with_transition("s22_1_2_s22_2", s22_2, obs1, TRANSITION_ACTION(const std::string&));
    s22_2.with_transition("s22_2_2_s22_final", s22_final, obs2, TRANSITION_ACTION(const std::string&));
    s22_2.with_transition("s22_2_2_join", join, obs5, TRANSITION_ACTION(const std::string&));
    s23_initial.with_transition("s23_initial_2_s23_1", s23_1, TRANSITION_ACTION());
    s23_1.with_transition("s23_1_2_s23_2", s23_2, obs1, TRANSITION_ACTION(const std::string&));
    s23_2.with_transition("s23_2_2_s23_final", s23_final, obs3, TRANSITION_ACTION(const std::string&));
    s23_2.with_transition("s23_2_2_join", join, obs6, TRANSITION_ACTION(const std::string&));
    join.with_transition("join_2_final", final, TRANSITION_ACTION());
    auto o = sm.assemble(cn).map([](const fsm::transition& t) -> std::string {return t.name();}).observe_on(cn);
    std::vector<std::string> result;
    o.subscribe([&result](const std::string& s){result.push_back(s);});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(!sm.is_terminated());
    GIVEN("s1"){
        REQUIRE(result.size() == 1);
        CHECK(result[0] == "initial_2_s1");
        result.clear();
        WHEN("transition to s2"){
            o2.on_next("a");
#ifdef VALGRIND
            while(result.size() < 4)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
            REQUIRE(result.size() == 4);
            auto it = result.begin();
            CHECK(*it == "s1_2_s2");
            ++it;
            CHECK(std::find(it, result.end(), "s21_initial_2_s21_1") != result.end());
            CHECK(std::find(it, result.end(), "s22_initial_2_s22_1") != result.end());
            CHECK(std::find(it, result.end(), "s23_initial_2_s23_1") != result.end());
            result.clear();
            THEN("transition to s21_2/s22_2/s23_2"){
                o1.on_next("a");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                REQUIRE(result.size() == 3);
                it = result.begin();
                CHECK(std::find(it, result.end(), "s21_1_2_s21_2") != result.end());
                CHECK(std::find(it, result.end(), "s22_1_2_s22_2") != result.end());
                CHECK(std::find(it, result.end(), "s23_1_2_s23_2") != result.end());
                result.clear();
                THEN("transition to s21_final"){
                    o1.on_next("a");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    REQUIRE(result.size() == 1);
                    CHECK(result[0] == "s21_2_2_s21_final");
                    result.clear();
                    THEN("transition to s22_final"){
                        o2.on_next("a");
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        REQUIRE(result.size() == 1);
                        CHECK(result[0] == "s22_2_2_s22_final");
                        result.clear();
                        THEN("transition 2 s23_final, i.e. transition to s3"){
                            o3.on_next("a");
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            REQUIRE(result.size() == 2);
                            CHECK(result[0] == "s23_2_2_s23_final");
                            CHECK(result[1] == "s2_2_s3");
                        }
                    }
                }
            }
        }
        WHEN("transition to fork. i.e. transition to s21_2/s22_2/s23_2"){
            o1.on_next("a");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            REQUIRE(result.size() == 4);
            auto it = result.begin();
            CHECK(*it == "s1_2_fork");
            ++it;
            CHECK(std::find(it, result.end(), "fork_2_s21_2") != result.end());
            CHECK(std::find(it, result.end(), "fork_2_s22_2") != result.end());
            CHECK(std::find(it, result.end(), "fork_2_s23_2") != result.end());
            result.clear();
            THEN("transition to join from s21_2, i.e join not entered"){
                o4.on_next("a");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "s21_2_2_join");
                result.clear();
                THEN("transition to join from s22_2, i.e. join not entered"){
                    o5.on_next("a");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    REQUIRE(result.size() == 1);
                    CHECK(result[0] == "s22_2_2_join");
                    result.clear();
                    THEN("transition to join from s23_2, i.e. join entered, i.e. transition to final"){
                        o6.on_next("a");
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        REQUIRE(result.size() == 2);
                        CHECK(result[0] == "s23_2_2_join");
                        CHECK(result[1] == "join_2_final");
                    }
                }
            }
        }
    }
}
