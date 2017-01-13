#define BOOST_TEST_MODULE de1220_test
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <iostream>
#include <string>

#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/de1220.hpp>
#include <pagmo/algorithms/null_algorithm.hpp>
#include <pagmo/io.hpp>
#include <pagmo/population.hpp>
#include <pagmo/problems/hock_schittkowsky_71.hpp>
#include <pagmo/problems/inventory.hpp>
#include <pagmo/problems/rosenbrock.hpp>
#include <pagmo/problems/zdt.hpp>
#include <pagmo/serialization.hpp>
#include <pagmo/types.hpp>

using namespace pagmo;

BOOST_AUTO_TEST_CASE(construction_test)
{
    std::vector<unsigned int> mutation_variants(18);
    std::iota(mutation_variants.begin(), mutation_variants.end(), 1u);
    de1220 user_algo(53u, mutation_variants, 1u, 1e-6, 1e-6, false, 23u);
    BOOST_CHECK(user_algo.get_verbosity() == 0u);
    BOOST_CHECK(user_algo.get_seed() == 23u);
    BOOST_CHECK((user_algo.get_log() == de1220::log_type{}));

    BOOST_CHECK_THROW((de1220{53u, {3u, 5u, 0u, 14u}, 1u, 1e-6, 1e-6, false, 23u}), std::invalid_argument);
    BOOST_CHECK_THROW((de1220{53u, {4u, 5u, 15u, 22u, 7u}, 1u, 1e-6, 1e-6, false, 23u}), std::invalid_argument);
    BOOST_CHECK_THROW((de1220{53u, mutation_variants, 0u, 1e-6, 1e-6, false, 23u}), std::invalid_argument);
    BOOST_CHECK_THROW((de1220{53u, mutation_variants, 3u, 1e-6, 1e-6, false, 23u}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(evolve_test)
{
    // We consider all variants
    std::vector<unsigned int> mutation_variants(18);
    std::iota(mutation_variants.begin(), mutation_variants.end(), 1u);
    // Here we only test that evolution is deterministic if the
    // seed is controlled for all variants
    problem prob1{rosenbrock{25u}};
    population pop1{prob1, 15u, 23u};
    problem prob2{rosenbrock{25u}};
    population pop2{prob2, 15u, 23u};

    for (unsigned int i = 1u; i <= 2u; ++i) {
        de1220 user_algo1(10u, mutation_variants, i, 1e-6, 1e-6, false, 41u);
        user_algo1.set_verbosity(1u);
        pop1 = user_algo1.evolve(pop1);

        de1220 user_algo2{10u, mutation_variants, i, 1e-6, 1e-6, false, 41u};
        user_algo2.set_verbosity(1u);
        pop2 = user_algo2.evolve(pop2);

        BOOST_CHECK(user_algo1.get_log().size() > 0u);
        BOOST_CHECK(user_algo1.get_log() == user_algo2.get_log());
    }

    // Here we check that the exit condition of ftol and xtol actually provoke an exit within 5000 gen (rosenbrock{2} is
    // used)
    { // xtol
        de1220 user_algo(300u, mutation_variants, 2, 1e-3, 1e-45, false, 41u);
        user_algo.set_verbosity(1u);
        problem prob{rosenbrock{2u}};
        population pop{prob, 20u, 23u};
        pop = user_algo.evolve(pop);
        BOOST_CHECK(user_algo.get_log().size() < 300u);
    }
    { // ftol
        de1220 user_algo(300u, mutation_variants, 1, 1e-45, 1e-3, false, 41u);
        user_algo.set_verbosity(1u);
        problem prob{rosenbrock{2u}};
        population pop{prob, 20u, 23u};
        pop = user_algo.evolve(pop);
        BOOST_CHECK(user_algo.get_log().size() < 300u);
    }

    // We then check that the evolve throws if called on unsuitable problems
    BOOST_CHECK_THROW(de1220{10u}.evolve(population{problem{rosenbrock{}}, 6u}), std::invalid_argument);
    BOOST_CHECK_THROW(de1220{10u}.evolve(population{problem{zdt{}}, 15u}), std::invalid_argument);
    BOOST_CHECK_THROW(de1220{10u}.evolve(population{problem{hock_schittkowsky_71{}}, 15u}), std::invalid_argument);
    BOOST_CHECK_THROW(de1220{10u}.evolve(population{problem{inventory{}}, 15u}), std::invalid_argument);
    // And a clean exit for 0 generations
    population pop{rosenbrock{25u}, 10u};
    BOOST_CHECK(de1220{0u}.evolve(pop).get_x()[0] == pop.get_x()[0]);
}

BOOST_AUTO_TEST_CASE(setters_getters_test)
{
    // We consider all variants
    std::vector<unsigned int> mutation_variants(18);
    std::iota(mutation_variants.begin(), mutation_variants.end(), 1u);
    de1220 user_algo(10000u, mutation_variants, 1, 1e-6, 1e-6, false, 41u);
    user_algo.set_verbosity(23u);
    BOOST_CHECK(user_algo.get_verbosity() == 23u);
    user_algo.set_seed(23u);
    BOOST_CHECK(user_algo.get_seed() == 23u);
    BOOST_CHECK(user_algo.get_name().find("1220") != std::string::npos);
    BOOST_CHECK(user_algo.get_extra_info().find("Allowed variants") != std::string::npos);
    BOOST_CHECK_NO_THROW(user_algo.get_log());
}

BOOST_AUTO_TEST_CASE(serialization_test)
{
    // Make one evolution
    problem prob{rosenbrock{2u}};
    population pop{prob, 15u, 23u};
    std::vector<unsigned int> mutation_variants(18);
    std::iota(mutation_variants.begin(), mutation_variants.end(), 1u);
    algorithm algo(de1220{10000u, mutation_variants, 1, 1e-6, 1e-6, false, 41u});
    algo.set_verbosity(1u);
    pop = algo.evolve(pop);

    // Store the string representation of p.
    std::stringstream ss;
    auto before_text = boost::lexical_cast<std::string>(algo);
    auto before_log = algo.extract<de1220>()->get_log();
    // Now serialize, deserialize and compare the result.
    {
        cereal::JSONOutputArchive oarchive(ss);
        oarchive(algo);
    }
    // Change the content of p before deserializing.
    algo = algorithm{null_algorithm{}};
    {
        cereal::JSONInputArchive iarchive(ss);
        iarchive(algo);
    }
    auto after_text = boost::lexical_cast<std::string>(algo);
    auto after_log = algo.extract<de1220>()->get_log();
    BOOST_CHECK_EQUAL(before_text, after_text);
    // BOOST_CHECK(before_log == after_log); // This fails because of floating point problems when using JSON and cereal
    // so we implement a close check
    BOOST_CHECK(before_log.size() > 0u);
    for (auto i = 0u; i < before_log.size(); ++i) {
        BOOST_CHECK_EQUAL(std::get<0>(before_log[i]), std::get<0>(after_log[i]));
        BOOST_CHECK_EQUAL(std::get<1>(before_log[i]), std::get<1>(after_log[i]));
        BOOST_CHECK_CLOSE(std::get<2>(before_log[i]), std::get<2>(after_log[i]), 1e-8);
        BOOST_CHECK_CLOSE(std::get<3>(before_log[i]), std::get<3>(after_log[i]), 1e-8);
        BOOST_CHECK_CLOSE(std::get<4>(before_log[i]), std::get<4>(after_log[i]), 1e-8);
        BOOST_CHECK_EQUAL(std::get<5>(before_log[i]), std::get<5>(after_log[i]));
        BOOST_CHECK_CLOSE(std::get<6>(before_log[i]), std::get<6>(after_log[i]), 1e-8);
        BOOST_CHECK_CLOSE(std::get<7>(before_log[i]), std::get<7>(after_log[i]), 1e-8);
    }
}
