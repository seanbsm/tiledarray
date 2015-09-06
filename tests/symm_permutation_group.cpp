/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2015  Virginia Tech
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Justus Calvin
 *  Department of Chemistry, Virginia Tech
 *
 *  symm_symm_group.cpp
 *  May 14, 2015
 *
 */

#include <random>
#include <chrono>

#include "TiledArray/symm/permutation_group.h"
#include "unit_test_config.h"

using TiledArray::PermutationGroup;
using TiledArray::Permutation;

struct PermutationGroupFixture {

  PermutationGroupFixture() :
    generator(std::chrono::system_clock::now().time_since_epoch().count()),
    uniform_int_distribution(0,100)
  {

    { // construct set of generators for P4__01__23__02_13 = P4{(0,1),(2,3),(0,2)(1,3)}
      // this group describes symmetries under permutations 0<->1, 2<->3, and {0,1}<->{2,3}
      P4__01__23__02_13_generators.reserve(3);
      P4__01__23__02_13_generators.emplace_back(Permutation{1,0,2,3});
      P4__01__23__02_13_generators.emplace_back(Permutation{0,1,3,2});
      P4__01__23__02_13_generators.emplace_back(Permutation{2,3,0,1});
    }

  }

  ~PermutationGroupFixture() { }

  template <size_t N>
  std::array<int, N> random_index() {
    std::array<int, N> result;
    for(auto& value : result)
      value = uniform_int_distribution(generator);
    return result;
  }

  // random number generation
  std::default_random_engine generator;
  std::uniform_int_distribution<int> uniform_int_distribution;

  // for testing symmetric group
  static const unsigned int max_degree = 5u;

  std::vector<Permutation> P4__01__23__02_13_generators;

  void validate_group(const PermutationGroup& S) {

    // Check that the group includes the identity element
    BOOST_CHECK_EQUAL(S.identity(), Permutation::identity(S.degree()));
    for(unsigned int i = 0u; i < S.order(); ++i) {
      BOOST_CHECK_EQUAL(S.identity() * S[i], S[i]);
      BOOST_CHECK_EQUAL(S[i] * S.identity(), S[i]);
    }

    // Check that the group forms a closed set
    for(unsigned int i = 0u; i < S.order(); ++i) {
      for(unsigned int j = 0u; j < S.order(); ++j) {
        Permutation e = S[i] * S[j];

        unsigned int k = 0u;
        for(; k < S.order(); ++k) {
          if(e == S[k])
            break;
        }

        // Check that e is a member of the group
        BOOST_CHECK(k < S.order());
      }
    }

    // Check that the elements of the set are associative
    for(unsigned int i = 0u; i < S.order(); ++i) {
      for(unsigned int j = 0u; j < S.order(); ++j) {
        for(unsigned int k = 0u; k < S.order(); ++k) {

          BOOST_CHECK_EQUAL((S[i] * S[j]) * S[k], S[i] * (S[j] * S[k]));
        }
      }
    }

    // Check that the group contains the inverse of each element
    for(unsigned int i = 0u; i < S.order(); ++i) {
      Permutation inv = S[i].inv();

      // Search for the inverse of S[i]
      unsigned int j = 0u;
      for(; j < S.order(); ++j)
        if(inv == S[j])
          break;

      // Check that inv is a member of the group
      BOOST_CHECK(j < S.order());

      // Check that the any element multiplied by it's own inverse is the identity
      BOOST_CHECK_EQUAL(inv * S[i], S.identity());
      BOOST_CHECK_EQUAL(S[i] * inv, S.identity());
    }
  }

}; // PermutationGroupFixture

BOOST_FIXTURE_TEST_SUITE( symm_group_suite, PermutationGroupFixture )

BOOST_AUTO_TEST_CASE( constructor )
{

  // symmetric group constructors
  {
    unsigned int order = 1u;
    for(unsigned int degree = 1u; degree <= max_degree; ++degree, order *= degree) {
      BOOST_REQUIRE_NO_THROW(PermutationGroup S(degree));
      PermutationGroup S(degree);

      // Check that the group has the correct degree
      BOOST_CHECK_EQUAL(S.degree(), degree);

      // Check that the number of elements in the group is correct
      BOOST_CHECK_EQUAL(S.order(), order);

      validate_group(S);
    }
  }

  // non-Sn permutation groups
  {
    PermutationGroup P4__01__23__02_13(4,P4__01__23__02_13_generators);

    // Check that the number of elements in the group is correct
    BOOST_CHECK_EQUAL(P4__01__23__02_13.order(), 8u);

    validate_group(P4__01__23__02_13);
  }

}

BOOST_AUTO_TEST_CASE( lexicographical_order )
{
  { // check S5
    typedef std::array<int,5> index_type;
    PermutationGroup S5(5);

//    std::cout<<"Permutation group S5:" << std::endl;
//    std::copy(S5.begin(), S5.end(), std::ostream_iterator<Permutation>(std::cout, "\n"));
//    std::cout<<std::endl;

    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,3,4,5},S5), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,1,300,300,500},S5), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{300,300,300,300,5},S5), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,1,0,0,5},S5), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,3,4,0},S5), false);
  }

  { // check P5{(0,2,4)(1,3)}
    typedef std::array<int,5> index_type;
    PermutationGroup P(5, std::vector<Permutation>{Permutation{4,3,0,1,2}});

//    std::cout<<"Permutation group P5{(0,2,4)(1,3)}:" << std::endl;
//    std::copy(P.begin(), P.end(), std::ostream_iterator<Permutation>(std::cout, "\n"));
//    std::cout<<std::endl;

    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,3,4,5},P), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,3,1,4,1},P), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,1,1,1},P), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,1,2,0,3},P), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,2,3,1},P), false);
  }

  { // check P4{(0,1),(2,3),(0,2)(1,3)}
    typedef std::array<int,4> index_type;
    PermutationGroup P(4, P4__01__23__02_13_generators);

//    std::cout<<"Permutation group P4{(0,1),(2,3),(0,2)(1,3)}:" << std::endl;
//    std::copy(P.begin(), P.end(), std::ostream_iterator<Permutation>(std::cout, "\n"));
//    std::cout<<std::endl;

    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,3,4},P), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,3,2,4},P), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,2,4},P), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{2,3,2,3},P), true);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,2,3,2},P), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{2,1,3,4},P), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{1,3,1,2},P), false);
    BOOST_CHECK_EQUAL(is_lexicographically_smallest(index_type{2,3,1,4},P), false);
  }

}

BOOST_AUTO_TEST_SUITE_END()