// fixed_point.hpp
//
// Copyright (c) 2016 Piotr K. Semenov (piotr.k.semenov at gmail dot com)
// Distributed under the New BSD License. (See accompanying file LICENSE)

/*!
 \file fixed_point.hpp

 \brief Provides the fixed-point numbers of any format and CORDIC-based
 overloading of STD math as well.
*/

#ifndef INC_LIBQ_FIXED_POINT_HPP_
#define INC_LIBQ_FIXED_POINT_HPP_

#include <boost/integer.hpp>
#include <boost/integer/integer_mask.hpp>

#include <cstdint>
#include <cmath>
#include <exception>
#include <type_traits>

#include "arithmetics_safety.hpp"
#include "type_promotion.hpp"

namespace libq {

/*!
 \brief Gets the reference to the stored integer behind the fixed-point number
 \param[in] _x the fixed-point number
*/
template<typename T, std::size_t n, std::size_t f, int e, class ... Ps>
T& lift(fixed_point<T, n, f, e, Ps...>& _x) {  // NOLINT
    return _x.m_value;
}

/*!
 \brief Gets the value of the stored integer behind the fixed-point number
 \param[in] _x the fixed-point number
*/
template<typename T, std::size_t n, std::size_t f, int e, class ... Ps>
T const lift(fixed_point<T, n, f, e, Ps...> const& _x) {
    return _x.m_value;
}

/*!
 \brief Implements the fixed-point number arithmetics. Note, this extends the
 Q-formats like UQn.f, Qn.f with fixed pre-scaling factor \f$2^e\f$.
 \tparam value_type Built-in integral type to represent a fixed-point number.
 \tparam n Number of integral bits.
 \tparam f Number of fractional bits.
 \remark Note, \f$n\f$ and \f$f\f$ exclude the sign bit. So if storage_type is
 signed then total number of bits is \f$(n + f + 1)\f$.
 \remake Note, the supremum of \f$(n + f)\f$ is
 std::numeric_limits<std::uintmax_t>::digits in case of the unsigned numbers
 and std::numeric_limits<std::intmax_t>::digits in case of the signed numbers.
 \tparam e Exponent of the pre-scaling factor \f2^e\f$.
 \tparam op Policy class specifying the actions to do if overflow occurred.
 \tparam up Policy class specifying the actions to do if underflow occurred.

 <B>Usage</B>

 <I>Example 1</I>: Flexible switch between the floating-point calculations and
 fixed precision calculations while algorithm staying the same.
 \code{.cpp}
    #include "fixed_point.hpp"
    #include <iostream>
    #include <cstdlib>

    namespace floating_point {
        using value_type = double;
    };  // namespace floating_point
    namespace fixed_precision {
        using value_type = libq::Q<10, 20>;
    };  // namespace fixed_precision
    using value_type = fixed_precision::value_type;

    int main(int, char**) {
        value_type input{ 0.0 };
        std::cin >> input;

        value_type const result = your_algorithm_here(input);
        std::cout << result << std::endl;

        return EXIT_SUCCESS;
    }
 \endcode

 \ref see http://en.wikipedia.org/wiki/Q_(number_format) for details
*/
template<typename value_type,
         std::size_t n,
         std::size_t f,
         int e,
         class op,
         class up>
class fixed_point {
        static_assert(std::is_integral<value_type>::value,
                      "value_type must be of the built-in integral type");

    using this_class = fixed_point<value_type, n, f, e, op, up>;
    using largest_type = typename std::conditional<
                                    std::numeric_limits<value_type>::is_signed,
                                    std::intmax_t, std::uintmax_t>::type;

  public:
    using type = this_class;
    using overflow_policy = op;
    using underflow_policy = up;

    /*!
     \brief Used type for the stored integer.
    */
    using storage_type = value_type;

    enum: int {
        scaling_factor_exponent = e
    };
    enum: std::size_t {
        /*!
         \brief Total number of significant bits.
        */
        number_of_significant_bits = n + f,

        /*!
         \brief Queried number of bits to represent the fractional part of
         fixed-point number.
        */
        bits_for_fractional = f,

        /*!
         \brief Number of bits to represent the integral part.
        */
        bits_for_integral = n,

        /*!
         \brief This checks if this fixed-point number is signed.
        */
        is_signed = std::numeric_limits<storage_type>::is_signed
    };

    /*!
     \brief Gets the scaling factor for this fixed-point number.
    */
    inline static double scaling_factor() {
        static double const factor = std::exp2(
                    -static_cast<double>(this_class::scaling_factor_exponent));

        return factor;
    }


    static_assert(this_class::number_of_significant_bits <=
                    std::numeric_limits<largest_type>::digits,
                  "too big word size is required");
#define EXP2N(N) (std::uintmax_t(1u) << (N))
    enum : std::uintmax_t {
        /*!
         \brief Scale factor for this fixed-point number.
        */
        scale = EXP2N(this_class::bits_for_fractional),

        /*!
         \brief Binary mask to extract the integral bits only from the stored
         integer.
        */
        // This is tricky to process if n + f = max. possible word size.
        integer_bits_mask = (this_class::bits_for_integral > 0u) ?
            2u * ( EXP2N(this_class::bits_for_fractional + this_class::bits_for_integral - 1u) -  // NOLINT
                   ( (this_class::bits_for_fractional > 0u) ? EXP2N(this_class::bits_for_fractional - 1u) : 0u)) : 0u,  // NOLINT

        /*!
         \brief Binary mask to extract the fractional bits from the stored
         integer.
        */
        fractional_bits_mask = (this_class::bits_for_fractional > 0u) ?
            2u * (EXP2N(this_class::bits_for_fractional - 1u) - 1u) + 1u : 0u
    };
#undef EXP2N

    /*!
     \brief The maximum value of stored integer for this fixed-point format.
    */
    static typename this_class::largest_type const largest_stored_integer =
        boost::low_bits_mask_t<this_class::number_of_significant_bits>::sig_bits;  // NOLINT
    /*!
     \brief Gets the maximum available fixed-point number.
    */
    static this_class largest() {
        return
            this_class::wrap<typename this_class::largest_type>(
                                           this_class::largest_stored_integer);
    }


    /*
     \brief The minimum value of stored integer for this fixed-point format.
    */
    static std::intmax_t const least_stored_integer =
        this_class::is_signed * (-static_cast<std::intmax_t>(this_class::largest_stored_integer) - 1);  // NOLINT
    /*!
     \brief Gets the minimum available fixed-point number.
    */
    static this_class least() {
        return
            this_class::wrap(this_class::least_stored_integer);
    }


    /*!
     \brief Gets the dynamic range (dB) for this fixed-point format
    */
    static double dynamic_range_db() {
        double const max_stored_integer =
            static_cast<double>(this_class::largest_stored_integer);

        return
            20.0 * std::log10(max_stored_integer);
    }


    /*!
     \brief Gets the precision of this fixed-point number.
    */
    static double precision() {
        return
            1.0 / this_class::scale;
    }


    /*!
     \brief Signed version of this fixed-point number type.
    */
    using to_signed_type = fixed_point<
                                 typename std::make_signed<storage_type>::type,
                                 this_class::bits_for_integral,
                                 this_class::bits_for_fractional,
                                 this_class::scaling_factor_exponent,
                                 overflow_policy,
                                 underflow_policy>;


    /*!
     \brief Unsigned version of this fixed-point number type.
    */
    using to_unsigned_type = fixed_point<
                               typename std::make_unsigned<storage_type>::type,
                               this_class::bits_for_integral,
                               this_class::bits_for_fractional,
                               this_class::scaling_factor_exponent,
                               overflow_policy,
                               underflow_policy>;


    /*!
     \brief Wraps the input integer _val as a fixed-point number.

     <B>Usage</B>

     <I>Example 1</I>:
     \code{.cpp}
         #include "fixed_point.hpp"
         #include <cstdint>
     
         int main(int, char**) {
             using Q = libq::Q<10, 20>;
     
             std::uint8_t input{ 23u };
             Q const fp = Q::wrap(input);
     
             return EXIT_SUCCESS;
         }
     \endcode
    */
    template<typename T>
    static this_class wrap(T const& _val) {
        static_assert(std::is_integral<T>::value,
                      "input param must be of the built-in integral type");

        if ((_val < 0 && _val < this_class::least_stored_integer) ||
            (_val > 0 && _val > this_class::largest_stored_integer)) {
            overflow_policy::raise_event();
        }

        this_class x;
        x.m_value = storage_type(_val);

        return x;
    }
    template<> static this_class wrap(float const&) = delete;
    template<> static this_class wrap(double const&) = delete;


    fixed_point() = default;
    fixed_point(this_class const& _x) = default;  // NOLINT


    /*!
     \brief Normalizes the input fixed-point number to be accepted by current
     format.
    */
    template<typename T1, std::size_t n1, std::size_t f1, int e1, class ... Ps>
    explicit fixed_point(fixed_point<T1, n1, f1, e1, Ps...> const& _x)
        : m_value(
            this_class::normalize(_x,
                                  std::integral_constant<bool, (int(f1) + e1 - int(this_class::bits_for_fractional) - this_class::scaling_factor_exponent > 0)>())) { // NOLINT
    }


    /*!
     \brief Creates the fixed-point number from any arithmetic object.
    */
    template<typename T>
    explicit fixed_point(T const& _value)
        : m_value(
            this_class::calc_stored_integer_from(_value,
                                                 std::integral_constant<bool, std::is_floating_point<T>::value>())) {  // NOLINT
    }


    this_class& operator =(this_class const& _x) = default;


    /// \brief assign operator in case of rvalue being of different fixed-point format
    template<typename T1, std::size_t n1, std::size_t f1, int e1, typename op1, typename up1>
    this_class& operator =(fixed_point<T1, n1, f1, e1, op1, up1> const& _x) {
        return
            this->set_value_to(
            this_class::normalize(_x, std::integral_constant<bool,
                (int(f1) + e1 - int(this_class::bits_for_fractional) - this_class::scaling_factor_exponent > 0)>())
            );
    }

    /// \brief assign operator in all other cases: rvalue is integer/floating-point number/arithmetic type
    template<typename T>
    void operator =(T const& _x) {
        static_assert(std::is_arithmetic<T>::value, "T must be of the arithmetic type");

        this->m_value = this_class::calc_stored_integer_from(_x, std::integral_constant<bool, std::is_floating_point<T>::value>());
    }

    /// \brief converts this fixed-point number to the single-precision floating-point number
    operator float() const{ return static_cast<float>(to_floating_point()); }

    /// \brief converts this fixed-point number to the double-precision floating-point number
    operator double() const{ return to_floating_point(); }

    /// \brief returns the stored integer
    storage_type value() const{ return this->m_value; }

/// to use boost!
#define cond_operator(op) \
    template<typename T> \
    bool operator op(T const& _x) const{ return this->value() op this_class(_x).value(); }

    cond_operator(<);
    cond_operator(<=);
    cond_operator(>);
    cond_operator(>=);
    cond_operator(==);
    cond_operator(!=);
#undef cond_operator

    bool operator !() const{ return this->value() == 0; }

    /*!
     widely-used constants
     naming conventions:
     1. CONST_2PI means 2*pi;
     2. CONST_2_PI means 2/pi;
     3. CONST PI_2 means pi/2.
    */
    static this_class const CONST_E, CONST_LOG2E, CONST_1_LOG2E, CONST_LOG10E,
        CONST_LOG102, CONST_LN2, CONST_LN10, CONST_2PI, CONST_PI, CONST_PI_2,
        CONST_PI_4, CONST_1_PI, CONST_2_PI, CONST_2_SQRTPI, CONST_SQRT2,
        CONST_SQRT1_2, CONST_2SQRT2;

    // arithmetics: summation
    template<typename T>
    inline typename libq::details::sum_traits<this_class>::promoted_type
        operator +(T const& _x) const {
        using sum_type = typename libq::details::sum_traits<this_class>::promoted_type;
        using word_type = typename sum_type::storage_type;

        this_class const converted(_x); // note, *this and converted have the same storage_type
        if (details::does_addition_overflow(*this, converted)) {
            overflow_policy::raise_event();
        }

        word_type const stored_integer = word_type(this->value()) + word_type(converted.value());
        return sum_type::wrap(stored_integer);
    }

    template<typename T>
    inline this_class& operator +=(T const& _x) {
        this_class const result(*this + _x);

        return this->set_value_to(result.value());
    }

    // arithmetics: subtraction
    template<typename T>
    typename libq::details::sum_traits<this_class>::promoted_type
        operator -(T const& _x) const {
        using diff_type = typename libq::details::sum_traits<this_class>::promoted_type;
        using word_type = typename diff_type::storage_type;

        this_class const converted(_x); // note, *this and converted have the same storage_type
        if (details::does_subtraction_overflow(*this, converted)) {
            overflow_policy::raise_event();
        }

        word_type const stored_integer = word_type(this->value()) - word_type(converted.value());
        return diff_type::wrap(stored_integer);
    }

    template<typename T>
    this_class operator -=(T const& _x) {
        this_class const result(*this - _x);

        return this->set_value_to(result.value());
    }

    // arithmetics: multiplication
    /*!
     \note you need not to do the proper promotion of fixed-point types by yourself.
     If no extra significant bits is available for the promoted type then operands are .
     In other words, std::common_type<L, R>::type is always equal to L
    */
    template<typename T1, std::size_t n1, std::size_t f1, int e1, class op1, class up1>
    typename libq::details::mult_of<this_class, libq::fixed_point<T1, n1, f1, e1, op1, up1> >::promoted_type
        operator *(libq::fixed_point<T1, n1, f1, e1, op1, up1> const& _x) const {
        using operand_type = typename libq::fixed_point<T1, n1, f1, e1, op1, up1>;
        using promotion_traits = libq::details::mult_of<this_class, operand_type>;
        using result_type = typename promotion_traits::promoted_type;
        using word_type = typename promotion_traits::promoted_storage_type;

        if (details::does_multiplication_overflow(*this, _x)) {
            overflow_policy::raise_event();
        }

        // do the exact/approximate multiplication of fixed-point numbers
        return result_type::wrap(
            (static_cast<word_type>(this->value()) * static_cast<word_type>(_x.value()))
                >> (promotion_traits::is_expandable ? 0 : operand_type::bits_for_fractional)
        );
    }

    template<typename T1, std::size_t n1, std::size_t f1, int e1, class op1, class up1>
    this_class operator *=(libq::fixed_point<T1, n1, f1, e1, op1, up1> const& _x) {
        this_class const result(*this * _x);

        return this->set_value_to(result.value());
    }

    // arithmetics: division
    template<typename T1, std::size_t n1, std::size_t f1, int e1, typename... Ps>
    typename libq::details::div_of<this_class, libq::fixed_point<T1, n1, f1, e1, Ps...> >::promoted_type
        operator /(libq::fixed_point<T1, n1, f1, e1, Ps...> const& _x) const {
        using operand_type = typename libq::fixed_point<T1, n1, f1, e1, Ps...>;
        using promotion_traits = libq::details::div_of<this_class, operand_type>;
        using result_type = typename promotion_traits::promoted_type;
        using word_type = typename promotion_traits::promoted_storage_type;

        if (details::does_division_overflow(*this, _x)) {
            overflow_policy::raise_event();
        }

        word_type const shifted = static_cast<word_type>(this->value()) << operand_type::number_of_significant_bits;
        if (!promotion_traits::is_expandable && _x.value() != (shifted >> operand_type::number_of_significant_bits)) {
            overflow_policy::raise_event();
        }

        return
            result_type::wrap(shifted / static_cast<word_type>(_x.value()));
    }

    template<typename T1, std::size_t n1, std::size_t f1, int e1, typename... Ps>
    this_class operator /=(libq::fixed_point<T1, n1, f1, e1, Ps...> const& _x) {
        this_class const result(*this / _x);

        return this->set_value_to(result.value());
    }

    // unary operator
    this_class operator -() const {
        if (details::does_unary_negation_overflow(*this)) {
            overflow_policy::raise_event();
        }
        if (!this_class::is_signed) {
            return this_class::wrap(this_class::largest_stored_integer - this->value());
        }

        return this_class::wrap(-this->value());
    }

private:

    /// \brief represents the input object being of the arithmetic type to a fixed-point number.
    /// It uses the rounding-to-nearest logics
    // in case of floating-point types
    template<typename T>
    static storage_type calc_stored_integer_from(T const& _x, std::true_type) {
        // double const value = double(_x) / std::exp2(static_cast<double>(this_class::scaling_factor_exponent));
        double const value = double(_x) * std::pow(2.0, static_cast<double>(this_class::scaling_factor_exponent));
        if (_x > T(0)) {
            storage_type const converted = static_cast<storage_type>(std::floor(value * this_class::scale + 0.5));
            if (converted < 0) {
                overflow_policy::raise_event();
            }

            return static_cast<storage_type>(std::floor(value * this_class::scale + 0.5));
        }

        return static_cast<storage_type>(std::ceil(value * this_class::scale - 0.5));
    }

    // in case of integral types
    template<typename T>
    static storage_type calc_stored_integer_from(T const& _x, std::false_type) {
        // double const value = double(_x) / std::exp2(static_cast<double>(this_class::scaling_factor_exponent));
        double const value = double(_x) * std::pow(2.0, static_cast<double>(this_class::scaling_factor_exponent));
        return storage_type(value) << this_class::bits_for_fractional;
    }

    // normalizes the input fixed-point number to one of the current format in case \f$f + e - f1 - e1 > 0\f$
    template<typename T1, std::size_t n1, std::size_t f1, int e1, typename... Ps>
    static storage_type normalize(fixed_point<T1, n1, f1, e1, Ps...> const& _x, std::false_type) {
        static std::size_t const shifts =
            (int(this_class::bits_for_fractional) + this_class::scaling_factor_exponent) - (int(e1) + f1);
        storage_type const normalized = storage_type(_x.value()) << shifts;
        
        if (_x.value() != (normalized >> shifts)) {
            overflow_policy::raise_event();
        }
        return normalized;
    }

    // normalizes the input fixed-point number to one of the current format in case \f$f + e - f1 - e1 < 0\f$
    template<typename T1, std::size_t n1, std::size_t f1, int e1, typename... Ps>
    static storage_type normalize(fixed_point<T1, n1, f1, e1, Ps...> const& _x, std::true_type) {
        static std::size_t const shifts =
            (int(e1) + f1) - (int(this_class::bits_for_fractional) + this_class::scaling_factor_exponent);
        storage_type const normalized =
            static_cast<storage_type>(_x.value() >> shifts);

        if (_x.value() && !normalized) {
            underflow_policy::raise_event();
        }
        return normalized;
    }

    // converts fixed-point number to the floating-point number
    double to_floating_point() const {
        return
            this->scaling_factor() * static_cast<double>(value()) / this_class::scale;
    }

    storage_type m_value;
    this_class& set_value_to(storage_type const _x) {
        if (_x < this_class::least_stored_integer || _x > this_class::largest_stored_integer) {
            overflow_policy::raise_event();
        }

        this->m_value = _x;
        return *this;
    }

    friend storage_type& lift<value_type, n, f, e, overflow_policy, underflow_policy>(this_class&);
};

/*!
*/
template<std::size_t n, std::size_t f, int e = 0, class op = libq::overflow_exception_policy, class up = libq::underflow_exception_policy>
using Q = libq::fixed_point<typename boost::int_t<n+1>::least, n-f, f, e, op, up>;

template<std::size_t n, std::size_t f, int e = 0, class op = libq::overflow_exception_policy, class up = libq::underflow_exception_policy>
using UQ = libq::fixed_point<typename boost::uint_t<n>::least, n-f, f, e, op, up>;

}  // namespace libq

// constant implementation
#define LIBQ_FIXED_POINT_CONSTANT(name, value) \
    template<typename T, std::size_t n, std::size_t f, int e, class op, class up> \
    libq::fixed_point<T, n, f, e, op, up> const libq::fixed_point<T, n, f, e, op, up>::name(value);

LIBQ_FIXED_POINT_CONSTANT(CONST_E, 2.71828182845904523536)
LIBQ_FIXED_POINT_CONSTANT(CONST_1_LOG2E, 0.6931471805599453)
LIBQ_FIXED_POINT_CONSTANT(CONST_LOG2E, 1.44269504088896340736)
LIBQ_FIXED_POINT_CONSTANT(CONST_LOG10E, 0.434294481903251827651)
LIBQ_FIXED_POINT_CONSTANT(CONST_LOG102, 0.301029995663981195214)
LIBQ_FIXED_POINT_CONSTANT(CONST_LN2, 0.693147180559945309417)
LIBQ_FIXED_POINT_CONSTANT(CONST_LN10, 2.30258509299404568402)
LIBQ_FIXED_POINT_CONSTANT(CONST_2PI, 6.283185307179586)
LIBQ_FIXED_POINT_CONSTANT(CONST_PI, 3.14159265358979323846)
LIBQ_FIXED_POINT_CONSTANT(CONST_PI_2, 1.57079632679489661923)
LIBQ_FIXED_POINT_CONSTANT(CONST_PI_4, 0.785398163397448309616)
LIBQ_FIXED_POINT_CONSTANT(CONST_1_PI, 0.318309886183790671538)
LIBQ_FIXED_POINT_CONSTANT(CONST_2_PI, 0.636619772367581343076)
LIBQ_FIXED_POINT_CONSTANT(CONST_2_SQRTPI, 1.12837916709551257390)
LIBQ_FIXED_POINT_CONSTANT(CONST_SQRT2, 1.41421356237309504880)
LIBQ_FIXED_POINT_CONSTANT(CONST_SQRT1_2, 0.707106781186547524401)
LIBQ_FIXED_POINT_CONSTANT(CONST_2SQRT2, 2.82842712474619009760)

#undef LIBQ_FIXED_POINT_CONSTANT

#include "details/sum_traits.inl"
#include "details/mult_of.inl"
#include "details/div_of.inl"

#include "details/sign.inl"

#include "details/ceil.inl"
#include "details/fabs.inl"
#include "details/floor.inl"
#include "details/round.inl"
#include "details/remainder.inl"
#include "details/fmod.inl"
#include "details/numeric_limits.inl"
#include "details/type_traits.inl"

#include "CORDIC/lut/lut.hpp"

#include "CORDIC/log.inl"
#include "CORDIC/sqrt.inl"

#include "CORDIC/sin.inl"
#include "CORDIC/cos.inl"
#include "CORDIC/tan.inl"

#include "CORDIC/exp.inl"

#include "CORDIC/sinh.inl"
#include "CORDIC/cosh.inl"
#include "CORDIC/tanh.inl"

#include "CORDIC/acos.inl"
#include "CORDIC/asin.inl"
#include "CORDIC/atan.inl"

#include "CORDIC/asinh.inl"
#include "CORDIC/acosh.inl"
#include "CORDIC/atanh.inl"

#endif // INC_LIBQ_FIXED_POINT_HPP_
