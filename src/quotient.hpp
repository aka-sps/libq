/// @brief provides info class for division operation result (quotient) type

#ifndef INC_CORE_QUOTIENT_HPP_
#define INC_CORE_QUOTIENT_HPP_

#include <boost/integer.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>

#include <limits>
#include <boost/cstdint.hpp>

namespace core {
    namespace {
        using boost::mpl::if_;
        using boost::mpl::eval_if;
    }

    template<typename T, size_t n, size_t f, class op, class up>
    class fixed_point;

    /// @brief tool for type inference of the division result with fixed-point and
    /// floating-point numbers
    template<typename T, typename U>
    class quotient
    {
        struct is_signed
        {
            enum { value = std::numeric_limits<T>::is_signed };
        };

        struct is_closed
        {
            enum { value = true };
        };

    public:
        typedef T type;
        typedef T word_type;
    };

    /// @brief in case of fixed-point numbers
    template<typename T1, size_t n1, size_t f1, class op1, class up1,
        typename T2, size_t n2, size_t f2, class op2, class up2>
    class quotient<fixed_point<T1, n1, f1, op1, up1>, fixed_point<T2, n2, f2, op2, up2> >
    {
        // is the type of division a signed type?
        struct is_signed
        {
            enum { value = std::numeric_limits<T1>::is_signed };
        };

        // is type of the division closed under arithmetic operations?
        struct is_closed
        {
            typedef typename if_<is_signed, boost::intmax_t, boost::uintmax_t>::type
                max_type;

            // total bits and std::numeric_limits do not count sign bit
            enum { value = !(n1 + n2 <= std::numeric_limits<max_type>::digits) };
        };

        // logics: 1. if we don't have integral type with enough bits amount then we 
        //            interpret division as closed operation
        //         2. boost::int_t template parameter takes into account a sign bit
        // integral value type for fixed-point result type of division
        struct word_type_info
        {
            // in case if quotient type is not closed under arithmetic operations
            // (promoted type exists)
            struct ex
            {
                typedef typename if_<is_signed, typename boost::int_t<n1 + n2 + 1u>::least,
                    typename boost::uint_t<n1 + n2>::least>::type type;
            };

            // in case if quotient type is closed under arithmetic operations
            struct cl
            {
                typedef T1 type;
            };
        };

        template<typename T, size_t n, size_t f, class op, class up>
        friend class fixed_point;

    public:
        // integral value type that is below of fixed-point result type of the
        // division
        typedef typename eval_if<is_closed, typename word_type_info::cl,
            typename word_type_info::ex>::type word_type;

        // fixed-point type for division result
        typedef typename if_<is_closed,
            fixed_point<T1, n1, f1, op1, up1>,
            fixed_point<word_type, n1 + n2, f1 + (n2 - f2), op1, up1>
        >::type type;
    };
}

#endif