/*
    MIT License
  
    Copyright (c) 2016-2020 Raúl Ramos
  
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
  
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
  
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <type_traits>

/*
    -- Version 1.0 --

    == Meta-Programming Mini Library ====================

    Basic concepts:

    - a type-list and associated basic functions to populate and access types ('push_back', 'pop_front', 'at', etc.)
    - more complex functions like 'get_filtered', 'get_the_best' or 'get_ancestors'

    Features around it:
    - a series of macros to declare a type-list, add types to it and eventually reading from it.

    One usage is to get all ancestors of a class so that given an instance process calls to all parents with the
    given instance.

    Read more about it at:
    - https://github.com/galtza/hierarchy-inspector and 
*/
namespace qcstudio {
namespace mpml {

    using namespace std;

    /*
        type list definition
    */

    template<typename ...TS>
    struct typelist {
        static constexpr auto size = sizeof...(TS);
    };
    using emptylist = typelist<>;

    /*
        creation
    */

    template<typename ...ARGS>
    constexpr auto make_typelist(const ARGS&...)
    -> typelist<ARGS...>;

    /*
        detection
    */

    template<typename TYPE>
    struct is_typelist : false_type { };

    template<typename ...TS>
    struct is_typelist<typelist<TS...>> : true_type { };

    /*
        basic operations
    */

    template<typename TYPE, typename TYPELIST> struct push_back;
    template<typename TYPE, typename TYPELIST> struct push_front;
    template<typename TYPELIST>                struct pop_front;
    template<size_t IDX, typename TYPELIST>    struct at;
    template<typename TYPELIST>                struct back;
    template<typename TYPELIST>                struct front;

    template<typename TYPE, typename TYPELIST> using push_back_t  = typename push_back<TYPE, TYPELIST>::type;
    template<typename TYPE, typename TYPELIST> using push_front_t = typename push_front<TYPE, TYPELIST>::type ;
    template<typename TYPELIST>                using pop_front_t  = typename pop_front<TYPELIST>::type;
    template<size_t IDX, typename TYPELIST>    using at_t         = typename at<IDX, TYPELIST>::type;
    template<typename TYPELIST>                using back_t       = typename at<TYPELIST::size - 1, TYPELIST>::type;
    template<typename TYPELIST>                using front_t      = typename at<0, TYPELIST>::type;

    template<typename TYPE, typename ...TS> struct push_back <TYPE, typelist<TS...>> { using type = typelist<TS..., TYPE>; };
    template<typename TYPE, typename ...TS> struct push_front<TYPE, typelist<TS...>> { using type = typelist<TYPE, TS...>; };
    template<typename TYPE, typename ...TS> struct pop_front <typelist<TYPE, TS...>> { using type = typelist<TS...>; };

    template<typename ...TS> 
    struct at<0, typelist<TS...>> { 
        static_assert(sizeof...(TS) > 0, "Empty typelist access");
    };

    template<typename TYPE, typename ...TS> 
    struct at<0, typelist<TYPE, TS...>> { 
        using type = TYPE; 
    };

    template <typename ...TS, size_t IDX>
    struct at<IDX, typelist<TS...>> {
        static_assert(IDX < (sizeof...(TS)), "Out of bounds access");
    };

    template <typename TYPE, typename ...TS, size_t IDX>
    struct at<IDX, typelist<TYPE, TS...>> {
        static_assert(IDX < (1 + sizeof...(TS)), "Out of bounds access");
        using type = at_t<IDX - 1, typelist<TS...>>;
    };

    /*
        'get_filtered'

        given a type list TYPELIST and a predicate PRED, it return another type list with only those elements that satisfy the predicate
    */

    template<typename TYPELIST, template<typename>class PRED>
    struct get_filtered;

    template<typename TYPELIST, template<typename>class PRED>
    using filter_t = typename get_filtered<TYPELIST, PRED>::type;

    template <template<typename>class PRED>
    struct get_filtered<emptylist, PRED> {
        using type = emptylist;
    };

    template <typename TYPE, typename ...TS, template<typename>class PRED>
    struct get_filtered<typelist<TYPE, TS...>, PRED> {
        using remaining_ = filter_t<typelist<TS...>, PRED>;
        using type = conditional_t<
            PRED<TYPE>::value,
            push_front_t<TYPE, remaining_>,
            remaining_
        >;
    };

    /*
        'get_the_best'

        given a type list TYPELIST and a type comparator CMP,. it returns the best type according to the comparator
    */

    template<typename TYPELIST, template<typename,typename>class CMP>
    struct get_the_best;

    template<typename TYPELIST, template<typename,typename>class CMP>
    using best_t = typename get_the_best<TYPELIST, CMP>::type;

    template <typename TYPE, template<typename,typename>class CMP>
    struct get_the_best<typelist<TYPE>, CMP> {
        using type = TYPE;
    };

    template <typename ...TS, template<typename,typename>class CMP>
    struct get_the_best<typelist<TS...>, CMP> {
        using first_type = at_t<0, typelist<TS...>>;
        using renaining_types = best_t<pop_front_t<typelist<TS...>>, CMP>;
        using type  = conditional_t<
            CMP<first_type, renaining_types>::value,
            first_type, renaining_types
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
        'contains' (See implementation below (2) )

        given a type TYPE and a typelist TYPELIST returns the true_type or false_type depending
        on whether TYPE is contained in TYPELIST or not.
    */

    namespace details {
        template<typename TYPE, typename TYPELIST>
        struct contains;
    }

    template<typename TYPE, typename TYPELIST>
    using contains = typename details::contains<TYPE, TYPELIST>::type;

    /*
        'get_ancestors' implementation details (1)
    */

    namespace details {
        template<typename SRCLIST, typename DESTLIST>
        struct get_ancestors;
    }

    template<typename TYPE, typename TYPELIST>
    struct get_ancestors {
        static_assert(is_typelist<TYPELIST>::value, "The second parameter type must be mpml::typelist");

        template<typename U>
        using base_of_T = typename is_base_of<U, TYPE>::type;
        using src_list = filter_t<TYPELIST, base_of_T>;
        using type = typename details::get_ancestors<
            src_list,
            emptylist
        >::type;
    };

    namespace details {

        template<typename SRCLIST, typename DESTLIST>
        struct get_ancestors {

            template<typename B>
            using negation_t = typename integral_constant<bool, !bool(B::value)>::type;

            template<typename TYPE1, typename TYPE2>
            using cmp = typename is_base_of<TYPE1, TYPE2>::type;
            using most_ancient = best_t<SRCLIST, cmp>;

            template<typename TYPE>
            using not_most_ancient_t = negation_t<is_same<most_ancient, TYPE>>;

            using all_but_most_ancient = filter_t<SRCLIST, not_most_ancient_t>;

            using type = typename details::get_ancestors<
                all_but_most_ancient,
                push_back_t<most_ancient, DESTLIST>
            >::type;

        };

        template<typename DESTLIST>
        struct get_ancestors<emptylist, DESTLIST> {
            using type = DESTLIST;
        };

    } // namespace details

    /*
        implementation details for 'contains' (2)
    */

    namespace details {

        template<typename TYPE>
        struct contains<TYPE, typelist<>> {
            using type = false_type;
        };

        template<typename TYPE, typename U, typename ...TS>
        struct contains<TYPE, typelist<U, TS...>> {
            using remaining_ = typename details::contains<TYPE, typelist<TS...>>::type;
            using type = typename conditional_t<
                is_same<TYPE, U>::value,
                true_type,
                remaining_
            >;
        };

    } // namespace details

    /*
        misc help functions
    */

    template <typename TYPE, size_t = sizeof(TYPE)>
    auto is_defined(TYPE*)  -> true_type;
    auto is_defined(...) -> false_type;

} // namespace mpml
} // namespace qcstudio

/*
    == Macro based interface ====================

    Series of macros to DECLARE, MODIFY and READ type-lists

    MPML_DECLARE(_name)    -> Equivalent to defining the type-list history
    MPML_ADD(_type, _name) -> Equivalent to defining a new entry in the history of the type-list with the types added
    MPML_TYPES(_name)      -> Equivalent to the type that represents the most updated version of the type-list
*/

#define MPML_DECLARE(_name)         INTERNAL_MPML_DECLARE(_name, __COUNTER__)
#define MPML_ADD(_type, _name)      INTERNAL_MPML_ADD(_name, _type, __COUNTER__)
#define MPML_CONTAINS(_type, _name) INTERNAL_MPML_CONTAINS(_name, _type)
#define MPML_TYPES(_name)           qcstudio::mpml::_name##_mpml_read_t<__COUNTER__ - 1>

/*
    == Macro based implementation ====================

    Note: it is recommended to read the mentioned articles
*/

#define INTERNAL_MPML_DECLARE(_name, _idx)\
    namespace qcstudio {\
    namespace mpml {\
        using namespace std;\
        \
        /* Declare the type-list history starting at entry _idx with an empty one*/\
        template<size_t IDX>\
        struct _name##_mpml_history;\
        \
        template<size_t IDX>\
        using _name##_mpml_history_t = typename _name##_mpml_history<IDX>::type;\
        \
        template<> struct _name##_mpml_history<_idx> {\
            using type = emptylist;\
        };\
        \
        /* Alias to check if an entry at IDX exists */\
        template <size_t IDX>\
        using _name##_mpml_is_defined = decltype(is_defined(declval<_name##_mpml_history<IDX>*>()));\
        \
        /* Define the reader base struct for index 'IDX' */\
        template<size_t IDX, bool = is_same<true_type, _name##_mpml_is_defined<IDX>>::value>\
        struct _name##_mpml_read;\
        \
        template<size_t IDX>\
        using _name##_mpml_read_t = typename _name##_mpml_read<IDX, is_same<true_type, _name##_mpml_is_defined<IDX>>::value>::type;\
        \
        /* When the history entry does exist */\
        template<size_t IDX>\
        struct _name##_mpml_read<IDX, true> {\
            using type = _name##_mpml_history_t<IDX>;\
        };\
        \
        /* When the history entry does NOT exist, we need to go back until  */\
        /* we get an existing entry or back to the original entry which has */\
        /* the empty type list                                              */\
        template<size_t IDX>\
        struct _name##_mpml_read<IDX, false> {\
            using type = conditional_t< \
                (IDX > _idx),                 /* Are there more specializations to check? */\
                _name##_mpml_read_t<IDX - 1>, /* yes */\
                emptylist                     /* no => failed => empty typelist */\
            >;\
        };\
    }}

#define INTERNAL_MPML_ADD(_name, _type, _idx)\
    /* Define the current type-list at index _idx (entries might not be consecutive) */\
    template<>\
    struct qcstudio::mpml::_name##_mpml_history<_idx> {\
        using type = qcstudio::mpml::push_back_t<_type, qcstudio::mpml::_name##_mpml_read_t<_idx - 1>>;\
    }

#define INTERNAL_MPML_CONTAINS(_name, _type) qcstudio::mpml::contains<_type, MPML_TYPES(_name)>::value
