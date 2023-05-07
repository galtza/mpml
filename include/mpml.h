/*
    SPDX-License-Identifier: BSD-3-Clause
    Copyright (C) 2016-2023 Raúl Ramos
    See the LICENSE file in the project root for more information.
*/
#pragma once

#include <type_traits>

// Normalize macros

#ifndef MPML_ENABLED
#    define MPML_ENABLED 0
#else
#    undef MPML_ENABLED
#    define MPML_ENABLED 1
#endif

#ifndef MPML_UNIT_TEST
#    define MPML_UNIT_TEST 0
#else
#    undef MPML_UNIT_TEST
#    define MPML_UNIT_TEST 1
#endif

/*
    == Meta-Programming Mini Library ====================

    Version: 1.1

    Basic concepts:

    - Type list definition
    - Simple operations: 'push_back', 'pop_front', 'at', etc.
    - Complex operations: 'get_filtered', 'get_the_best' or 'get_ancestors'
    - Macros to handle type lists

    More:
    - https://github.com/galtza/hierarchy-inspector
*/

#if MPML_ENABLED

namespace qcstudio::mpml {

    using namespace std;

    /*
        'typelist' definition
    */

    template<typename... TS>
    struct typelist {
        static constexpr auto size = sizeof...(TS);
    };
    using emptytypelist = typelist<>;

    /*
        creation
    */

    template<typename... ARGS>
    constexpr auto make_typelist(const ARGS&...) -> typelist<ARGS...>;

    /*
        conversion
    */

    template<typename... TS>
    constexpr auto make_tuple_from_typelist() -> tuple<TS...>;

    template<typename TYPELIST>
    using to_tuple = decltype(foo(declval<TYPELIST>()));

    /*
        querying
    */

    template<typename TYPE>
    struct is_typelist : false_type {};

    template<typename... TS>
    struct is_typelist<typelist<TS...>> : true_type {};

    template<typename TYPE>
    inline constexpr bool is_typelist_v = is_typelist<TYPE>::value;

    // clang-format off

    /*
        BASIC operations
    */

    // 'find' get the index of the first occurrence of TYPE inside the TYPELIST

    template<typename TYPE, typename TYPELIST, int N = TYPELIST::size> struct find;
    template<typename TYPE, typename TYPELIST, int N = TYPELIST::size> constexpr auto find_v = find<TYPE, TYPELIST, N>::value;
    template<typename TYPE, int N>                                     struct find<TYPE, emptytypelist, N> : integral_constant<int, -(N + 1)> {};
    template<typename TYPE, int N, typename... TS>                     struct find<TYPE, typelist<TYPE, TS...>, N> : integral_constant<int, 0> {};
    template<typename TYPE, typename OTHER, int N, typename... TS>     struct find<TYPE, typelist<OTHER, TS...>, N> : integral_constant<int, 1 + find_v<TYPE, typelist<TS...>, N>> {};

    // 'contains' check if TYPE is inside TYPELIST

    template<typename TYPE, typename TYPELIST> struct contains : bool_constant<find_v<TYPE, TYPELIST> != -1> {};
    template<typename TYPE, typename TYPELIST> constexpr auto contains_v = contains<TYPE, TYPELIST>::value;

    // 'concat'

    template<typename TL1, typename TL2> struct concat;
    template<typename TL1, typename TL2> using concat_t = typename concat<TL1, TL2>::type;
    template<typename... ARGS1, typename... ARGS2> 
    struct concat<typelist<ARGS1...>, typelist<ARGS2...>> {
        using type = typelist<ARGS1..., ARGS2...>;
    };

    // 'invert'

    template<typename TYPELIST> struct invert;
    template<typename TYPELIST> using invert_t = typename invert<TYPELIST>::type;
    template<> struct invert<typelist<>> {
        using type = typelist<>;
    };
    template<typename TYPE, typename... TS> struct invert<typelist<TYPE, TS...>> {
        using type = concat_t<invert_t<typelist<TS...>>, typelist<TYPE>>;
    };

    // 'push_front'

    template<typename TYPE, typename TYPELIST> struct push_front;
    template<typename TYPE, typename TYPELIST> using  push_front_t = typename push_front<TYPE, TYPELIST>::type;
    template<typename TYPE, typename... TS>    struct push_front<TYPE, typelist<TS...>> {
        using type = typelist<TYPE, TS...>;
    };

    // 'push_back'

    template<typename TYPE, typename TYPELIST> struct push_back;
    template<typename TYPE, typename TYPELIST> using  push_back_t = typename push_back<TYPE, TYPELIST>::type;
    template<typename TYPE, typename... TS>    struct push_back<TYPE, typelist<TS...>> {
        using type = typelist<TS..., TYPE>;
    };

    // 'pop_front'

    template<typename TYPELIST> struct pop_front;
    template<typename TYPELIST> using  pop_front_t = typename pop_front<TYPELIST>::type;
    template<> struct pop_front<emptytypelist> {
        using type = emptytypelist;
    };
    template<typename TYPE, typename... TS> struct pop_front<typelist<TYPE, TS...>> {
        using type = typelist<TS...>;
    };

    // 'pop_back'

    template<typename TYPELIST> using pop_back   = pop_front<invert_t<TYPELIST>>;
    template<typename TYPELIST> using pop_back_t = typename pop_back<TYPELIST>::type;

    // 'at'

    template<size_t IDX, typename TYPELIST>             struct at;
    template<size_t IDX, typename TYPELIST>             using  at_t = typename at<IDX, TYPELIST>::type;
    template<typename... TS>                            struct at<0, typelist<TS...>>         { static_assert(sizeof...(TS) > 0, "Empty typelist"); };
    template<typename TYPE, typename... TS>             struct at<0, typelist<TYPE, TS...>>   { using type = TYPE; };
    template<typename... TS, size_t IDX>                struct at<IDX, typelist<TS...>>       { static_assert(IDX < (sizeof...(TS)), "Out of bounds"); };
    template<typename TYPE, typename... TS, size_t IDX> struct at<IDX, typelist<TYPE, TS...>> { using type = at_t<IDX - 1, typelist<TS...>>; 
                                                                                                static_assert(IDX < (1 + sizeof...(TS)), "Out of bounds"); };

    // 'back'

    template<typename TYPELIST> struct back;
    template<typename TYPELIST> using  back_t = typename at<TYPELIST::size - 1, TYPELIST>::type;

    // 'front'

    template<typename TYPELIST> struct front;
    template<typename TYPELIST> using  front_t = typename at<0, TYPELIST>::type;

    /*
        COMPLEX operations
    */

    // 'remove_all' remove all TYPE from the TYPELIST

    template<typename TYPE, typename TYPELIST> struct remove_all;
    template<typename TYPE, typename TYPELIST> using  remove_all_t = typename remove_all<TYPE, TYPELIST>::type;
    template<typename TYPE>                    struct remove_all<TYPE, emptytypelist> { using type = emptytypelist; };

    template<typename TYPE, typename U, typename... TS>
    struct remove_all<TYPE, typelist<U, TS...>> {
        using type = conditional_t<
            is_same_v<TYPE, U>,
            remove_all_t<TYPE, typelist<TS...>>,
            concat_t<typelist<U>, remove_all_t<TYPE, typelist<TS...>>>
        >;
    };

    /*
        'remove_duplicates' -> createes another typelist whith duplicates removed

        given a type list TYPELIST, it return another type list where there are no
        duplicated types
    */

    template<typename TYPELIST> struct remove_duplicates;
    template<typename TYPELIST> using  remove_duplicates_t = typename remove_duplicates<TYPELIST>::type;
    template<>                  struct remove_duplicates<emptytypelist> { using type = emptytypelist; };
    template<typename T>        struct remove_duplicates<typelist<T>>   { using type = typelist<T>; };

    template<typename T, typename... TS>
    struct remove_duplicates<typelist<T, TS...>> {
        using type = conditional_t<
            contains_v<T, typelist<TS...>>,
            concat_t<typelist<T>, remove_duplicates_t<remove_all_t<T, typelist<TS...>>>>,
            concat_t<typelist<T>, remove_duplicates_t<typelist<TS...>>>
        >;
    };

    /*
        'get_filtered' -> returns a new typelist where elements are filtered with the provided TRAIT that eventually resolves to a TRAIT::value
    */

    template<typename TYPELIST, template<typename> typename TRAIT> struct get_filtered;
    template<typename TYPELIST, template<typename> typename TRAIT> using  get_filtered_t = typename get_filtered<TYPELIST, TRAIT>::type;
    template<template<typename> typename TRAIT>                    struct get_filtered<emptytypelist, TRAIT> { using type = emptytypelist; };

    template<typename TYPE, typename... TS, template<typename> typename TRAIT>
    struct get_filtered<typelist<TYPE, TS...>, TRAIT> {
        using remaining = get_filtered_t<typelist<TS...>, TRAIT>;
        using type      = conditional_t<
            TRAIT<TYPE>::value,
            push_front_t<TYPE, remaining>,
            remaining
        >;
    };

    /*
        'get_the_best' -> returns a new typelist with the best elements

        given a type list TYPELIST and a type comparator CMP,. it returns the best type according to the comparator
    */

    template<typename TYPELIST, template<typename, typename> typename CMP>
    struct get_the_best;

    template<typename TYPELIST, template<typename, typename> typename CMP>
    using get_the_best_t = typename get_the_best<TYPELIST, CMP>::type;

    template<typename TYPE, template<typename, typename> typename CMP>
    struct get_the_best<typelist<TYPE>, CMP> {
        using type = TYPE;
    };

    template<typename... TS, template<typename, typename> typename CMP>
    struct get_the_best<typelist<TS...>, CMP> {
        using first_type     = at_t<0, typelist<TS...>>;
        using remaining_best = get_the_best_t<pop_front_t<typelist<TS...>>, CMP>;
        using type           = conditional_t<
            CMP<first_type, remaining_best>::value,
            first_type, remaining_best
        >;
    };

    /*
        'get_ancestors' (See implementation below (1) )

        given a type TYPE and a type list TYPELIST containing types from various hierarchies
        (any type in any order), it return the family tree of TYPE among the types in TYPELIST
        in descending order.
    */

    template<typename TYPE, typename TYPELIST>
    struct get_ancestors;

    template<typename TYPE, typename TYPELIST>
    using get_ancestors_t = typename get_ancestors<TYPE, TYPELIST>::type;

    /*
        'get_ancestors' implementation details (1)
    */

    namespace details {
        template<typename SRCLIST, typename DESTLIST>
        struct get_ancestors;
    }

    template<typename TYPE, typename TYPELIST>
    struct get_ancestors {
        static_assert(is_typelist_v<TYPELIST>, "The second parameter type must be mpml::typelist");

        template<typename U>
        using trait             = typename is_base_of<U, TYPE>::type;
        using filtered_typelist = get_filtered_t<remove_all_t<TYPE, TYPELIST>, trait>;
        using type              = typename details::get_ancestors<
            filtered_typelist,
            emptytypelist
        >::type;
    };

    namespace details {

        template<typename SRCLIST, typename DESTLIST>
        struct get_ancestors {
            template<bool B>
            using negation_t = typename integral_constant<bool, !B>::type;

            template<typename T1, typename T2>
            using CMP          = typename is_base_of<T1, T2>::type;
            using most_ancient = get_the_best_t<SRCLIST, CMP>;

            template<typename T>
            using not_most_ancient_t = negation_t<is_same_v<most_ancient, T>>;

            using all_but_most_ancient = get_filtered_t<SRCLIST, not_most_ancient_t>;

            using type = typename details::get_ancestors<
                all_but_most_ancient,
                push_back_t<most_ancient, DESTLIST>
            >::type;
        };

        template<typename DESTLIST>
        struct get_ancestors<emptytypelist, DESTLIST> {
            using type = DESTLIST;
        };

    }  // namespace details

    // clang-format on

    /*
        typelist iterator
    */

    template<typename TYPELIST, typename PRED, unsigned LENGTH = TYPELIST::size, unsigned INDEX = 0>
    struct typelist_iterator {
        static_assert(qcstudio::mpml::is_typelist_v<TYPELIST>, "Not a typelist");
        inline static void step(void* _p) {
            using type = qcstudio::mpml::at_t<INDEX, TYPELIST>;
            PRED::callback(static_cast<type*>(_p));
            typelist_iterator<TYPELIST, PRED, LENGTH, INDEX + 1>::step(_p);
        }
    };

    template<typename TYPELIST, typename PRED, unsigned LENGTH>
    struct typelist_iterator<TYPELIST, PRED, LENGTH, LENGTH> {
        inline static void step(void*) {
        }
    };

    /*
        misc help functions
    */

    template<typename T, size_t = sizeof(T)>
    auto is_defined_impl(T*) -> true_type;
    auto is_defined_impl(...) -> false_type;

    template<typename T>
    using is_defined = decltype(is_defined_impl(declval<T*>()));

    template<typename T>
    constexpr auto is_defined_v = is_defined<T>::value;

}  // namespace qcstudio::mpml

/*
    == Macro based interface ====================

    Series of macros to DECLARE, MODIFY and READ type-lists

    MPML_DECLARE(_name)    -> Equivalent to defining the type-list history
    MPML_ADD(_type, _name) -> Equivalent to defining a new entry in the history of the type-list with the types added
    MPML_TYPES(_name)      -> Equivalent to the type that represents the most updated version of the type-list
*/

#    define MPML_WRAP(...)              __VA_ARGS__
#    define MPML_DECLARE(_name)         INTERNAL_MPML_DECLARE(_name, __COUNTER__)
#    define MPML_ADD(_type, _name)      INTERNAL_MPML_ADD(MPML_WRAP(_type), _name, __COUNTER__)
#    define MPML_CONTAINS(_type, _name) INTERNAL_MPML_CONTAINS(MPML_WRAP(_type), _name)
#    define MPML_TYPES(_name)           typename qcstudio::mpml::_name##_mpml_read<__COUNTER__ - 1>::type

/*
    == Macro based implementation ====================

    Note: it is recommended to read the mentioned articles
*/

#    define INTERNAL_MPML_DECLARE(_name, _idx)                                              \
        namespace qcstudio {                                                                \
            namespace mpml {                                                                \
                using namespace std;                                                        \
                                                                                            \
                /* Declare the type-list history starting at entry _idx with an empty one*/ \
                                                                                            \
                template<size_t IDX>                                                        \
                struct _name##_mpml_history;                                                \
                                                                                            \
                template<> struct _name##_mpml_history<_idx> {                              \
                    using type = emptytypelist;                                             \
                };                                                                          \
                                                                                            \
                template<size_t IDX>                                                        \
                using _name##_mpml_history##_t = typename _name##_mpml_history<IDX>::type;  \
                                                                                            \
                /* Define the reader base struct for index 'IDX' */                         \
                                                                                            \
                template<size_t IDX, bool = is_defined_v<_name##_mpml_history<IDX>>>        \
                struct _name##_mpml_read;                                                   \
                                                                                            \
                template<size_t IDX>                                                        \
                using _name##_mpml_read##_t = typename _name##_mpml_read<IDX>::type;        \
                                                                                            \
                /* When the history entry does exist */                                     \
                                                                                            \
                template<size_t IDX>                                                        \
                struct _name##_mpml_read<IDX, true> {                                       \
                    using type = _name##_mpml_history##_t<IDX>;                             \
                };                                                                          \
                                                                                            \
                /* When the history entry does NOT exist, we need to go back until  */      \
                /* we get an existing entry or back to the original entry which has */      \
                /* the empty type list (I know, it blew my mind too)                */      \
                                                                                            \
                template<size_t IDX>                                                        \
                struct _name##_mpml_read<IDX, false> {                                      \
                    using type = conditional_t<                                             \
                        (IDX > _idx),                   /* more specializations? */         \
                        _name##_mpml_read##_t<IDX - 1>, /* yes */                           \
                        emptytypelist                   /* no => failed => empty TL */      \
                        >;                                                                  \
                };                                                                          \
            }                                                                               \
        }

#    define INTERNAL_MPML_ADD(_type, _name, _idx)                                           \
        /* Define the current type-list at index _idx (entries might not be consecutive) */ \
        template<>                                                                          \
        struct qcstudio::mpml::_name##_mpml_history<_idx> {                                 \
            using type = qcstudio::mpml::push_back_t<                                       \
                _type,                                                                      \
                qcstudio::mpml::_name##_mpml_read##_t<_idx - 1>>;                           \
        }

#    define INTERNAL_MPML_CONTAINS(_type, _name) qcstudio::mpml::contains_v<_type, MPML_TYPES(_name)>

#    if MPML_UNIT_TEST

MPML_DECLARE(MPML_TEST_TL);  // <--- Here we declare the named type 'MPML_TEST_TL' (initially empty)

namespace qcstudio::mpml::unit_testing {

    /*

    == Our unit test hierarchies ==========

                                        F
                                       / \
         A                            H   \
        / \                          / \   \
       B   C                        I   J   G
      /   / \                        \ /   / \
     T   D   E                        K   L   Z
                                      |
                                      W
    */
    class A {};
    class F {};
    class B : public A {};
    class G : public F {};
    class C : public A {};
    class L : public G {};
    class T : public B {};
    class Z : public G {};
    class D : public C {};
    class H : public F {};
    class E : public C {};
    class I : public H {};
    class J : public H {};
    class K : public I, public J {};
    class W : public K {};

    class ZZ {};

    // Test 'emptytypelist'

    static_assert(emptytypelist::size == 0);

    // Test 'make_typelist'

    static_assert(is_same_v<decltype(make_typelist(1, 2.0, 3.f, '4', "Hello World!")), typelist<int, double, float, char, char[13]>>);

    // clang-format off

    // Test all predicates:
    /* 
        ================================================================================================================
                             PREDICATE                       PARAMS                                             EXPECTED
        ================================================================================================================
    */
    static_assert(           is_typelist_v                  <typelist<A, B>>                                    == true);
    static_assert(           is_typelist_v                  <A>                                                 == false);
    static_assert(                                           emptytypelist::size                                == 0);
    static_assert( is_same_v<invert_t                       <emptytypelist>,                                    emptytypelist>);
    static_assert( is_same_v<invert_t                       <typelist<A>>,                                      typelist<A>>);
    static_assert( is_same_v<invert_t                       <typelist<A, C>>,                                   typelist<C, A>>);
    static_assert( is_same_v<invert_t                       <typelist<A, B, C>>,                                typelist<C, B, A>>);
    static_assert( is_same_v<push_back_t                    <A, emptytypelist>,                                 typelist<A>>);
    static_assert( is_same_v<push_back_t                    <B, typelist<A>>,                                   typelist<A, B>>);
    static_assert( is_same_v<push_front_t                   <B, emptytypelist>,                                 typelist<B>>);
    static_assert( is_same_v<push_front_t                   <B, typelist<A>>,                                   typelist<B, A>>);
    static_assert( is_same_v<pop_back_t                     <emptytypelist>,                                    emptytypelist>);
    static_assert( is_same_v<pop_back_t                     <typelist<A>>,                                      emptytypelist>);
    static_assert( is_same_v<pop_back_t                     <typelist<A, B>>,                                   typelist<A>>);
    static_assert( is_same_v<pop_front_t                    <emptytypelist>,                                    emptytypelist>);
    static_assert( is_same_v<pop_front_t                    <typelist<A>>,                                      emptytypelist>);
    static_assert( is_same_v<pop_front_t                    <typelist<A, B>>,                                   typelist<B>>);
    static_assert( is_same_v<at_t                           <0, typelist<A, B, C>>,                             A>);
    static_assert( is_same_v<at_t                           <2, typelist<A, B, C>>,                             C>);
    static_assert( is_same_v<front_t                        <typelist<A>>,                                      A>);
    static_assert( is_same_v<front_t                        <typelist<Z, B, C>>,                                Z>);
    static_assert( is_same_v<back_t                         <typelist<A, B, Z>>,                                Z>);
    static_assert( is_same_v<back_t                         <typelist<A>>,                                      A>);
    static_assert(           contains_v                     <Z, typelist<A, Z, C>>                              == true);
    static_assert(           contains_v                     <Z, typelist<A, E, C>>                              == false);
    static_assert(           contains_v                     <Z, emptytypelist>                                  == false);
    static_assert( is_same_v<remove_all_t                   <A, emptytypelist>,                                 emptytypelist>);
    static_assert( is_same_v<remove_all_t                   <A, typelist<A>>,                                   emptytypelist>);
    static_assert( is_same_v<remove_all_t                   <A, typelist<B>>,                                   typelist<B>>);
    static_assert( is_same_v<remove_all_t                   <A, typelist<A, B, C, A>>,                          typelist<B, C>>);
    static_assert( is_same_v<remove_duplicates_t            <emptytypelist>,                                    emptytypelist>);
    static_assert( is_same_v<remove_duplicates_t            <typelist<A>>,                                      typelist<A>>);
    static_assert( is_same_v<remove_duplicates_t            <typelist<A, A>>,                                   typelist<A>>);
    static_assert( is_same_v<remove_duplicates_t            <typelist<B, A, B>>,                                typelist<B, A>>);
    static_assert( is_same_v<remove_duplicates_t            <typelist<A, B, C, A, A, B, D, D, D, A, B>>,        typelist<A, B, C, D>>);
    static_assert(           find_v                         <Z, emptytypelist>                                  == -1);
    static_assert(           find_v                         <Z, typelist<Z>>                                    == 0);
    static_assert(           find_v                         <Z, typelist<A, C, D, D, Z, C, A, D, C>>            == 4);
    static_assert(           find_v                         <Z, typelist<A, C, D, D, D, C, A, D, C>>            == -1);
    static_assert( is_same_v<concat_t                       <typelist<A, B, C>, typelist<D, E>>,                typelist<A, B, C, D, E>>);
    static_assert( is_same_v<invert_t                       <typelist<A, B, C, D, E>>,                          typelist<E, D, C, B, A>>);

    // clang-format on

}  // namespace qcstudio::mpml::unit_testing

// Add new types to our typelist named MPML_TEST_TL

MPML_ADD(qcstudio::mpml::unit_testing::C, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::D, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::Z, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::H, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::I, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::I, MPML_TEST_TL); // Repeated
MPML_ADD(qcstudio::mpml::unit_testing::E, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::T, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::L, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::B, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::A, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::J, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::A, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::G, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::K, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::A, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::F, MPML_TEST_TL);
MPML_ADD(qcstudio::mpml::unit_testing::W, MPML_TEST_TL);

namespace qcstudio::mpml::unit_testing {

    using namespace std;

    static_assert(is_same_v<get_ancestors_t<D, MPML_TYPES(MPML_TEST_TL)>, typelist<A, C>>);
    static_assert(is_same_v<get_ancestors_t<K, MPML_TYPES(MPML_TEST_TL)>, typelist<F, H, J, I>>);
    static_assert(is_same_v<get_ancestors_t<W, MPML_TYPES(MPML_TEST_TL)>, typelist<F, H, J, I, K>>);

    // Test contain macros

    static_assert(!MPML_CONTAINS(ZZ, MPML_TEST_TL));
    static_assert(MPML_CONTAINS(Z, MPML_TEST_TL));

    // Runtime Test

    struct TestPredicate {
        template<typename T>
        inline static void callback(T* _instance) {
            if (_instance) {
                cout << "instance with addr " << _instance << ": as type '" << typeid(T).name() << "'\n";
            }
        }
    };

    void execute() {
        cout << "\nClass D hierarchy iteration..." << endl;
        auto d_instance = D{};
        typelist_iterator<get_ancestors_t<D, MPML_TYPES(MPML_TEST_TL)>, TestPredicate>::step(&d_instance);
        cout << "\n";

        cout << "\nClass K hierarchy iteration..." << endl;
        auto k_instance = K{};
        typelist_iterator<get_ancestors_t<K, MPML_TYPES(MPML_TEST_TL)>, TestPredicate>::step(&k_instance);
        cout << "\n";

        cout << "\nClass W hierarchy iteration..." << endl;
        auto w_instance = W{};
        typelist_iterator<get_ancestors_t<W, MPML_TYPES(MPML_TEST_TL)>, TestPredicate>::step(&w_instance);
        cout << endl;
    }

}  // namespace qcstudio::mpml::unit_testing

#    else

namespace qcstudio::mpml::unit_testing {
    void execute() {
    }
}  // namespace qcstudio::mpml::unit_testing

#    endif  // MPML_UNIT_TEST

#endif  // MPML_ENABLED