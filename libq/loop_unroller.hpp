// loop_unroller.hpp
//
// Copyright (c) 2016 Piotr K. Semenov (piotr.k.semenov at gmail dot com)
// Distributed under the New BSD License. (See accompanying file LICENSE)

/*!
 @file loop_unroller.hpp

 Provides the loop unroller to improve the generated code performance.
 It works nice with closures and functor objects.
*/

#ifndef INC_LIBQ_DETAILS_LOOP_UNROLLER_HPP_
#define INC_LIBQ_DETAILS_LOOP_UNROLLER_HPP_

#include <cstdlib>


namespace libq {
namespace details {

/*!
 \defgroup details details

 Aux classes/functions.

 \{
*/
template<std::size_t N>
struct loop_size {
};

/*!
 \brief Unrolls the loop represented by the iteration body and the starting
 iterator.
 \tparam N Parameter number of iterations to be unrolled.
 \tparam Functor_type Type of a functor/lambda that is the body of iterations.
 \tparam Iterator_type Type of an iterator that traverses the loop's range set.
 \param[in] _f Functor/lambda object representing the loop body.
 \param[in] _begin The starting iterator for loop's range set.

 <B>Usage</B>

 <I>Example 1</I>: "how-to" to improve the performance of the array
 initialization
 \code{.cpp}
    #include "loop_unroller.hpp"

    #include <cstdlib>
    #include <cassert>
    #include <cstdint>

    #include <vector>
    #include <numeric>
    #include <chrono>
    #include <iostream>

    #define N 100000000ul
    
    int main(int argc, char** argv) {
        using namespace std::chrono;
        using std::uintmax_t;
    
        std::vector<uintmax_t> array(N);
        auto const filler = [&](uintmax_t const _i) {
            array[_i] = _i * (_i+1u) * (_i+2u) * (_i+3u) * (_i+4u) * (_i+5u);
        };

        auto const start1 = system_clock::now();
        for (uintmax_t i = 0; i != N; i += CHUNK_LENGTH) {
            libq::details::unroll(filler,
                                  i,
                                  libq::details::loop_size<CHUNK_LENGTH-1>());
        }
        auto const end1 = system_clock::now();
        duration<double> const elapsed1 = end1 - start1;
    
        auto const start2 = system_clock::now();
        for (uintmax_t i = 0; i != N; i++) {
            array[i] = i * (i+1u) * (i+2u) * (i+3u) * (i+4u) * (i+5u);
        }
        auto const end2 = system_clock::now();
        duration<double> const elapsed2 = end2 - start2;
    
        std::cout
            << elapsed1.count()
            << " "
            << elapsed2.count()
            << std::endl;
    
        return EXIT_SUCCESS;
    }
 \endcode
 \note The target machine is x64 Intel(R) i7-4702MQ CPU @ 2.20GHz 2.20GHz.
 Please see the performance points for the native code generated by the engines
 below:
 - g++ ver. 4.8.3 (CYGWIN_NT-6.3 x86_64) :
  * \verbatim
g++ -std=gnu++11 -O3 -ftemplate-depth=101 -DCHUNK_LENGTH=1 -mtune=native -o example1 ./example1.cpp
while true; do ./example1; done
\endverbatim
  * received performance points (confidence intervals for level 99%):
    \verbatim
| CHUNK_LENGTH |   elapsed1 (s)   |  elapsed2 (s)    |
------------------------------------------------------
[      1       | [0.6574, 0.6656] | [0.6591, 0.6670] |
[      2       | [0.5445, 0.5477] | [0.6598, 0.6637] |
[      5       | [0.1520, 0.1635] | [0.6631, 0.6759] |
|      10      | [0.1545, 0.1587] | [0.6596, 0.6656] |
------------------------------------------------------
\endverbatim

 - MS VC 18.00.21005.1 (x64):
  * \verbatim
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
vcvarsall.bat amd64
@echo OFF
cl /nologo /O2 /favor:AMD64 /DCHUNK_LENGTH=1 /Feexample1 ./example1.cpp
FOR /L %i in (1,1,50) do (example1.exe) >> log.txt
\endverbatim
  * received performance points (confidence intervals for level 99%):
    \verbatim
| CHUNK_LENGTH |   elapsed1 (s)   |  elapsed2 (s)    |
------------------------------------------------------
[      1       | [0.2211, 0.2256] | [0.2223, 0.2247] |
[      2       | [0.1737, 0.1765] | [0.2230, 0.2268] |
[      5       | [0.1732, 0.1756] | [0.2232, 0.2270] |
|      10      | [0.1735, 0.1768] | [0.2234, 0.2275] |
------------------------------------------------------
\endverbatim
*/
template<std::size_t N, typename Functor_type, typename Iterator_type>
inline void unroll(Functor_type const& _f, Iterator_type _begin, loop_size<N>) {
    unroll(_f, _begin, loop_size<N-1>());

    _f(_begin + N);
}


template<typename Functor_type, typename Iterator_type>
inline void
    unroll(Functor_type const& _f, Iterator_type _begin, loop_size<0u>) {
    _f(_begin);
}

/*! \} */  // doxygen: details
}  // namespace details
}  // namespace libq
#endif  // INC_LIBQ_DETAILS_LOOP_UNROLLER_HPP_
