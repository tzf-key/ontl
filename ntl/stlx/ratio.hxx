/**\file*********************************************************************
 *                                                                     \brief
 *  Compile-time rational arithmetic [20.4 ratio]
 *
 ****************************************************************************
 */
#ifndef NTL__STLX_RATIO
#define NTL__STLX_RATIO
#pragma once

#ifndef NTL__STLX_TYPE_TRAITS
#include "type_traits.hxx"
#endif

namespace std
{
/**\addtogroup  lib_utilities *** 20 General utilities library [utilities]
 *@{
 **/

  /**\defgroup lib_ratio ****** 20.4 Compile-time rational arithmetic [ratio]
   *  The ratio library provides a class template ratio which exactly represents any finite rational number 
   *  with a numerator and denominator representable by compile-time constants of type intmax_t.
   *@{
   **/

#if _INTEGRAL_MAX_BITS >= 64
  #define RATIO_MAX     LLONG_MAX
  #define RATIO_MIN     LLONG_MIN
  typedef intmax_t      ratio_t;
  typedef uintmax_t     uratio_t;
#else
  #define RATIO_MAX       INT_MAX
  #define RATIO_MIN     (-2147483647 - 1)
  typedef int           ratio_t;
  typedef unsigned int  uratio_t;
#endif

  template <ratio_t N, ratio_t D = 1> class ratio;

  // ratio arithmetic
  template <class R1, class R2> struct ratio_add;
  template <class R1, class R2> struct ratio_subtract;
  template <class R1, class R2> struct ratio_multiply;
  template <class R1, class R2> struct ratio_divide;

  // ratio comparison
  template <class R1, class R2> struct ratio_equal;
  template <class R1, class R2> struct ratio_not_equal;
  template <class R1, class R2> struct ratio_less;
  template <class R1, class R2> struct ratio_less_equal;
  template <class R1, class R2> struct ratio_greater;
  template <class R1, class R2> struct ratio_greater_equal;

  // convenience SI typedefs
#if _INTEGRAL_MAX_BITS >= 128
  typedef ratio<1, 1000000000000000000000000> yocto; // see 20.4.4
  typedef ratio<1,    1000000000000000000000> zepto; // see 20.4.4
#elif _INTEGRAL_MAX_BITS >= 64
  typedef ratio<1,       1000000000000000000> atto;   // 10^-18
  typedef ratio<1,          1000000000000000> femto;  // 10^-15
  typedef ratio<1,             1000000000000> pico;   // 10^-12
#endif
  typedef ratio<1,                1000000000> nano;   // 10^-9
  typedef ratio<1,                   1000000> micro;  // 10^-6
  typedef ratio<1,                      1000> milli;  // 10^-3
  typedef ratio<1,                       100> centi;  // 10^-2
  typedef ratio<1,                        10> deci;   // 10^-1
  typedef ratio<                       10, 1> deca;   // 10^1
  typedef ratio<                      100, 1> hecto;  // 10^2
  typedef ratio<                     1000, 1> kilo;   // 10^3
  typedef ratio<                  1000000, 1> mega;   // 10^6
  typedef ratio<               1000000000, 1> giga;   // 10^9
#if _INTEGRAL_MAX_BITS >= 64
  typedef ratio<            1000000000000, 1> tera;   // 10^12
  typedef ratio<         1000000000000000, 1> peta;   // 10^15
  typedef ratio<      1000000000000000000, 1> exa;    // 10^18
#elif _INTEGRAL_MAX_BITS >= 128
  typedef ratio<   1000000000000000000000, 1> zetta; // see 20.4.4
  typedef ratio<1000000000000000000000000, 1> yotta; // see 20.4.4
#endif

  namespace __ { namespace static_evaluation {

    // sign functions
    template<ratio_t a>
    struct sign: integral_constant<ratio_t, (a < 0) ? -1 : 1>
    {};

    template<ratio_t a>
    struct is_negative: integral_constant<bool, (a < 0)>
    {};

    template<ratio_t a>
    struct abs: integral_constant<ratio_t, (a >= 0) ? a : 0-a>
    {};

    // gcd by euclidean algorithm
    template <ratio_t a, ratio_t b>
    struct gcd_impl:       gcd_impl<b, a % b>
    {};

    template <ratio_t a>
    struct gcd_impl<a, 0>: integral_constant<ratio_t, a>
    {};

    template <ratio_t b>
    struct gcd_impl<0, b>: integral_constant<ratio_t, b>
    {};

    template <ratio_t a, ratio_t b>
    struct gcd: gcd_impl< abs<a>::value, abs<b>::value >
    {};

    // power of 2
    template<ratio_t v>
    struct power2:
      integral_constant<ratio_t, (1 << v)>
    {};

    // log2 
    template<ratio_t v, bool strict = false>
    struct log2
    {
      static const ratio_t value = 1 + log2<(v >> 1)>::value;
      typedef ratio_t value_type;
      typedef integral_constant<ratio_t, value> type;
    };

    template<>
    struct log2<1>:
      integral_constant<ratio_t, 0>
    {};

    // is power of 2?
    template<ratio_t v>
    struct is_power2: 
      integral_constant<bool, power2<log2<v>::value>::value == v>
    {};

    // strict log2
    template<ratio_t v>
    struct log2<v, true>
      :log2<v, false>
    {
      static_assert(is_power2<v>::value, "specified value isn't a power of 2");
    };
  }}

  /**
   *	@brief Class template ratio [20.4.1 ratio.ratio]
   *
   *  \code typedef std::ratio<-2,6> one_third; one_third::num == -1, one_third::den == 3 \endcode
   *
   *  @tparam N numenator of the ratio
   *  @tparam D denominator of the ratio, which are always positive and default to 1
   *  @note Negative ratios represented as negative numenator and \e positive denominator:
   *  ratio is always normalized such that it is expressed in lowest terms, and the denominator is always positive.
   **/
  template <ratio_t N, ratio_t D>
  class ratio
  {
    static constexpr const ratio_t gcd_value = __::static_evaluation::gcd<N, D>::value;
  public:
    static_assert(D != 0, "template argument D shall not be zero");
    static_assert((N >= RATIO_MIN && N <= RATIO_MAX) && (D >= RATIO_MIN && D <= RATIO_MAX), "out of range");

    /**
     *	@brief Numerator of the ratio
     *
     *  \c num shall have the value \f$ \frac{sign(N)*sign(D)*abs(N)}{gcd} \f$,
     *  but \f$ sign(N)*abs(N) == N \f$.
     **/
    static constexpr const ratio_t num = N * __::static_evaluation::sign<D>::value / gcd_value;

    /**
     *	@brief Denominator of the ratio
     *
     *  \c den shall have the value \f$ \frac{sign(N)*sign(D)*abs(D)}{gcd} \f$, after simplification it would be
     *  \f$ \frac{D * sign(N)}{gcd} \f$, but we use \f$ \frac{abs(D)}{gcd} \f$ (see N2661).
     **/
    static constexpr const ratio_t den = __::static_evaluation::abs<D>::value / gcd_value;

    /** The \c ratio type */
    typedef ratio type;
  };



  // 20.4.2 Arithmetic on ratio types [ratio.arithmetic]
  namespace __ {

    /*
       there are 4 ways of addition:
    (1)   a + b = a + b;    constraint: max - b >= a
    (2)  -a + b = b - a;    constraint: min + a <  b
    (3)   a +-b = a - b;    constraint: min + b <  a
    (4)  -a +-b = -(a + b); constraint: min + a >= -b OR min + b >= -a

       also there are 4 ways of substraction:
          a - b             (3)
         -a - b = -(a + b)  (4)
          a --b =   a + b   (1)
         -a --b =   b - a   (2)
   */

    /* 1 */
    template <ratio_t a, ratio_t b, bool negative_values>
    struct check_add_overflow_impl:
      integral_constant<bool, (static_evaluation::abs<a>::value <= RATIO_MAX - static_evaluation::abs<b>::value)>
    {
      //char x[value];
    };

    /* 4 */
    template <ratio_t a, ratio_t b>
    struct check_add_overflow_impl<a, b, true>:
      integral_constant<bool, (RATIO_MIN + static_evaluation::abs<a>::value <= b)>
    {
      //char x[value];
    };

    /* 2 */
    template <ratio_t a, ratio_t b>
    struct check_sub_underflow_impl:
      integral_constant<bool, (RATIO_MIN + static_evaluation::abs<a>::value < static_evaluation::abs<b>::value)>
    {};

    /* 1 and 4 */
    template <ratio_t a, ratio_t b>
    struct check_add_overflow:
      check_add_overflow_impl<a, b, static_evaluation::is_negative<a>::value == static_evaluation::is_negative<b>::value == true>
    {};

    /* 2 and 3 */
    template <ratio_t a, ratio_t b>
    struct check_sub_underflow:
      conditional<
        static_evaluation::is_negative<a>::value,
          check_sub_underflow_impl<a, b>,
          check_sub_underflow_impl<b, a>
                 >::type
    {};

    template <ratio_t a, ratio_t b>
    struct check_add_flows:
      conditional< static_evaluation::is_negative<a>::value || static_evaluation::is_negative<b>::value,
        typename conditional< static_evaluation::is_negative<a>::value && static_evaluation::is_negative<b>::value,
          check_add_overflow_impl<a, b, true>,
          check_sub_underflow<a, b>
                   >::type,
        check_add_overflow_impl<a, b, false>
                 >::type
    {
      /*
        if(a < 0 || b < 0){
          if(a < 0 && b < 0)
            check_4(a,b);
          else
            check_23(a,b);
        }else{
          check_1(a,b);
        }
       */
    };

    template <ratio_t a, ratio_t b>
    struct check_sub_flows:
      conditional< static_evaluation::is_negative<a>::value,
        typename conditional< static_evaluation::is_negative<b>::value,
          check_sub_underflow_impl<a, b>,
          check_add_overflow_impl<a, b, true>
                            >::type,
        typename conditional< static_evaluation::is_negative<b>::value,
          check_add_overflow_impl<a, b, false>,
          check_sub_underflow_impl<b, a>
                            >::type
                 >::type
    {
      /*
        if(a < 0){
          if(b < 0)
            check_2
          else
            check_4
        }else{
          if(b < 0)
            check_1
          else
            check_3
       */
    };

  }

  /// 20.4.2 Arithmetic on ratio types [ratio.arithmetic]
  /** static addition with overflow detection */
  template <ratio_t a, ratio_t b>
  struct ratio_checked_add
  {
    static_assert((__::check_add_flows<a, b>::value), "add overflow");

    static constexpr const ratio_t value = a + b;
  };

  /** static substraction with underflow detection */
  template <ratio_t a, ratio_t b>
  struct ratio_checked_sub
  {
    static_assert((__::check_sub_flows<a, b>::value), "sub underflow");

    static constexpr const ratio_t value = a - b;
  };

  /** static multiplication with overflow detection */
  template <ratio_t a, ratio_t b>
  struct ratio_checked_multiply
  {
  private:
    static constexpr const uratio_t absa = __::static_evaluation::abs<a>::value;
    static constexpr const uratio_t absb = __::static_evaluation::abs<b>::value;
    static constexpr const uratio_t c = 1LL << (sizeof(uratio_t)*4);

    static constexpr const uratio_t a1 = absa % c;
    static constexpr const uratio_t a2 = absa / c;
    static constexpr const uratio_t b1 = absb % c;
    static constexpr const uratio_t b2 = absb / c;

    static_assert(a2 == 0 || b2 == 0, "mul overflow");
    static_assert(a1 * b2 + b1 * a2 < (c >> 1), "mul overflow");
    static_assert(a1 * b1 <= RATIO_MAX, "mul overflow");
    static_assert((a1 * b2 + b1 * a2) * c <= RATIO_MAX - a1 * b1, "mul overflow");

  public:
    static constexpr const ratio_t value = a * b;
  };

  /**
   *	static addition with overflow detection and simplification
   *  @internal To prevent a potential but unnecessary overflow a simplification algorithm used here
   **/
  template <class R1, class R2>
  struct ratio_add
  {
  private:
    static const intmax_t dens_gcd = __::static_evaluation::gcd<R1::den, R2::den>::value;
  public:
    typedef ratio<
      ratio_checked_add<
        ratio_checked_multiply<R1::num, R2::den / dens_gcd>::value,
        ratio_checked_multiply<R2::num, R1::den / dens_gcd>::value
      >::value,
      ratio_checked_multiply<R1::den,   R2::den / dens_gcd>::value
                 > type;
    static constexpr const ratio_t num = type::num, den = type::den;
  };

  /** static substraction with overflow detection and simplification */
  template <class R1, class R2>
  struct ratio_subtract
  {
    typedef typename
      ratio_add<R1, ratio<-R2::num, R2::den> >::type type;
    static constexpr const ratio_t num = type::num, den = type::den;
  };

  /** static multiplication with overflow detection and simplification */
  template <class R1, class R2>
  struct ratio_multiply
  {
  private:
    static constexpr const intmax_t gcd1 = __::static_evaluation::gcd<R1::num, R2::den>::value,
      gcd2 = __::static_evaluation::gcd<R2::num, R1::den>::value;
  public:
    typedef ratio<
      ratio_checked_multiply<R1::num / gcd1, R2::num / gcd2>::value,
      ratio_checked_multiply<R1::den / gcd2, R2::den / gcd1>::value
                 > type;
    static constexpr const ratio_t num = type::num, den = type::den;
  };

  /** static division with overflow detection and simplification */
  template <class R1, class R2>
  struct ratio_divide
  {
    static_assert(R2::num != 0, "division by zero");

    typedef typename ratio_multiply<R1, ratio<R2::den, R2::num> >::type type;
    static constexpr const ratio_t num = type::num, den = type::den;
  };



  /// 20.4.3 Comparison of ratio types [ratio.comparison]

  /** Check is two ratios are equal */
  template <class R1, class R2>
  struct ratio_equal:
    integral_constant<bool, R1::num == R2::num && R1::den == R2::den>
  { };

  /** Check is two ratios are not equal */
  template <class R1, class R2>
  struct ratio_not_equal:
    integral_constant<bool, !ratio_equal<R1, R2>::value>
  { };

  namespace __ {
    template<class R1, class R2>
    struct ratio_less_common:
      integral_constant<bool, ((ratio_checked_multiply<R1::num, R2::den>::value) < (ratio_checked_multiply<R2::num, R1::den>::value)) >
    {};
  }

  /** Check is first ratio are less than second */
  template <class R1, class R2>
  struct ratio_less:
    conditional<
      R1::den == R2::den,
      integral_constant<bool, ((R1::num) < (R2::num)) >,
      __::ratio_less_common<R1, R2>
               >::type
  { };

  /** Check is first ratio are less or equal to second */
  template <class R1, class R2>
  struct ratio_less_equal:
    integral_constant<bool, !ratio_less<R2, R1>::value>
  { };

  /** Check is first ratio are greater than second */
  template <class R1, class R2>
  struct ratio_greater:
    integral_constant<bool, ratio_less<R2, R1>::value>
  { };

  /** Check is first ratio are greater or equal than second */
  template <class R1, class R2>
  struct ratio_greater_equal:
    integral_constant<bool, !ratio_less<R1, R2>::value>
  { };

  /**@} lib_ratio */
  /**@} lib_utilities */
} // namespace std

#endif // NTL__STLX_RATIO
