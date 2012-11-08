/// @brief provides CORDIC for cos function
/// @ref see H. Dawid, H. Meyr, "CORDIC Algorithms and Architectures"

#include "./../../fixed_point_lib/src/CORDIC/lut/lut.hpp"

namespace std {
    template<typename T, size_t n, size_t f, class op, class up>
    core::fixed_point<T, n, f, op, up> sin(core::fixed_point<T, n, f, op, up> const& val)
    {
        BOOST_STATIC_ASSERT(n - f >= 3);

        typedef core::fixed_point<T, n, f, op, up> fp;
        using core::cordic::lut;
        BOOST_STATIC_ASSERT(std::numeric_limits<fp>::is_signed);

        // convergence interval for CORDIC rotations is [-pi/2, pi/2].
        // So one has to map input angle to that interval
        fp arg(0);
        int sign(1);
        {
            // reduce argument to interval [-pi, +pi] with sign preserving
            fp const x = fp::CONST_PI - std::fmod(val, fp::CONST_2PI);
            if (x < -fp::CONST_PI_2) {
                arg = x + fp::CONST_PI;

                sign = -1;
            }
            else if (x > fp::CONST_PI_2) {
                arg = x - fp::CONST_PI;

                sign = -1;
            }
            else {
                arg = x;
            }
        }

        typedef lut<f, fp> lut_type;
        static lut_type const angles = lut_type::build_arctan_lut();

        // normalization factor: see page 10, table 24.1 and pages 4-5, equations
        // (5)-(6)
        // factor converges to the limit 1.64676 very fast: it tooks 8 iterations
        // only. 8 iterations correpsonds to precision of size 0.007812 for
        // angle approximation
        static fp norm_factor(1.0 / lut_type::compute_circular_scale(f));

        // rotation mode: see page 6
        // shift sequence is just 0, 1, ... (circular coordinate system)
        fp x(norm_factor), y(0.0), z(arg);
        fp x1, y1, z1;
        BOOST_FOREACH(size_t i, boost::irange<size_t>(0, f, 1))
        {
            int const sign = (z > fp(0)) ? 1 : -1;
            fp const x_scaled = fp::wrap(sign * (x.value() >> i));
            fp const y_scaled = fp::wrap(sign * (y.value() >> i));

            x1 = fp(x - y_scaled);
            y1 = fp(y + x_scaled);
            z1 = fp(z - fp((sign > 0) ? angles[i] : -angles[i]));

            x = x1; y = y1; z = z1;
        }

        return (sign > 0) ? y : -y;
    }
}
