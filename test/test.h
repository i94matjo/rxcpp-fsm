#include <exception>
#if (__GLIBCXX__ / 10000) == 2014 || (__GLIBCXX__ / 10000) == 2015
namespace std {
inline bool uncaught_exception() noexcept(true) {
    return current_exception() != nullptr;
}
}
#endif

#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxs=rxcpp::sources;
namespace rxo=rxcpp::operators;
namespace rxsub=rxcpp::subjects;
namespace rxsc=rxcpp::schedulers;
namespace rxn=rx::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt = rxcpp::test;

#include "rxcpp/rx-fsm.hpp"
namespace fsm = rxcpp::fsm;

#include "catch.hpp"

namespace rxcpp {
namespace fsm {

template<class T, int no_of_subjects = 1>
struct test_fixture
{
    test_fixture()
        : sm(fsm::make_state_machine("sm"))
    {
        for(int i = 0; i < no_of_subjects; i++)
        {
            _subjects.push_back(rxsub::subject<T>());
        }
    }

    ~test_fixture()
    {
        sm.terminate();
    }

    template<class Coordination>
    void start(Coordination cn)
    {
        sm.start(std::move(cn));
    }

    template<class Coordination>
    rx::observable<fsm::transition> assemble(Coordination cn)
    {
        return sm.assemble(std::move(cn));
    }

    std::vector<rxsub::subject<T>> _subjects;
    fsm::state_machine sm;
};

typedef test_fixture<std::string, 1> string_fixture1;
typedef test_fixture<std::string, 2> string_fixture2;
typedef test_fixture<std::string, 3> string_fixture3;
typedef test_fixture<std::string, 4> string_fixture4;
typedef test_fixture<std::string, 5> string_fixture5;
typedef test_fixture<std::string, 6> string_fixture6;

}
}

#define FSM_SUBJECT(x) \
    auto sub##x = _subjects[ x - 1 ]; \
    auto obs##x = sub##x.get_observable(); \
    auto o##x = sub##x.get_subscriber();
