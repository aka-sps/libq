Motivation:
Simulations for algorithms in constraints of underlying hardware makes developers
to re-implement their code.
The flexibility of C++ templates can reduce this problem: developers should
write code in the most general way, using constructors, C++ casts, explicit
defining of all types and so on.
In this case, the switching from floating-point computations to the fixed-point
computations provided by fixed_point_lib is very easy and of no pain.

Library features:
1.	Lib was implemented in the most general (templates, constructors and so on)
way to have the easy float-point/fixed-point and vice versa translations (w/o
time-consuming editing/refactoring of your code);
2.	Lib supports any Q-format of fixed-point (not only power of two as its
analogs do);
3.	Lib does the compile-time type inference for resulting type of addition,
subtraction, multiplication, division, all elementary functions (exp, log, sin,
cos, tan, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh).
Existing fixed-point libs do not do this stuff at all.
They assume fixed-point types to be closed under all operations;
4.	Lib enables capturing the overflow/underflow behavior in run-time (BTW, it
can be ignored);
5.	Lib provides the implementation for all math functions for fixed-point
numbers (CORDIC-based algorithms);
6.	Lib provides the compile-time scheduling for multiplication/division in goal
to achieve max possible precision of the result;
7.	Lib is easy-scalable for any computation platform (24-bit, 28-bit, 32-bit,
64-bit, 128-bit and so on).
8. Lib allows to capture the numbers range explicitly. For example, using
wordlength = 10 and fractional bits = 32, we define the pre-factor 2^-22.