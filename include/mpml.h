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
namespace mpml {

    // type list definition
    template<typename ...TS>
    struct typelist {
        static constexpr auto size = sizeof...(TS);
    };
    using emptylist = typelist<>;

    // creation
    template<typename ...ARGS>
    constexpr auto make_typelist(const ARGS&...)
    -> typelist<ARGS...>;

    // detect type list types
    template<typename T>
    struct is_typelist : std::false_type { };

    template<typename ...TS>
    struct is_typelist<typelist<TS...>> : std::true_type { };

    // basic operations
    template<typename T, typename TL>    struct push_back;
    template<typename T, typename TL>    struct push_front;
    template<typename TL>                struct pop_front;
    template<std::size_t I, typename TL> struct at;

    template<typename T, typename TL>    using push_back_t  = typename push_back<T, TL>::type;
    template<typename T, typename TL>    using push_front_t = typename push_front<T, TL>::type ;
    template<typename TL>                using pop_front_t  = typename pop_front<TL>::type;
    template<std::size_t I, typename TL> using at_t         = typename at<I, TL>::type;

    template<typename T, typename ...TS> struct push_back <T, typelist<TS...>> { using type = typelist<TS..., T>; };
    template<typename T, typename ...TS> struct push_front<T, typelist<TS...>> { using type = typelist<T, TS...>; };
    template<typename T, typename ...TS> struct pop_front <typelist<T, TS...>> { using type = typelist<TS...>;    };

    template<typename T, typename ...TS> 
    struct at<0, typelist<T, TS...>> { 
        using type = T; 
    };

    template <typename T, typename ...TS, std::size_t I>
    struct at<I, typelist<T, TS...>> {
        static_assert(I < (1 + sizeof...(TS)), "Out of bounds access");
        using type = at_t<I - 1, typelist<TS...>>;
    };

    // 'get_filtered': given a type list, return another type list where the contained types are filtered according to a predicate
    template<typename TL, template<typename>class PRED>
    struct get_filtered;

    template<typename TL, template<typename>class PRED>
    using filter_t = typename get_filtered<TL, PRED>::type;

    template <template<typename>class PRED>
    struct get_filtered<emptylist, PRED> {
        using type = emptylist;
    };

    template <typename T, typename ...TS, template<typename>class PRED>
    struct get_filtered<typelist<T, TS...>, PRED> {
        using remaining_ = filter_t<typelist<TS...>, PRED>;
        using type = std::conditional_t<
            PRED<T>::value,
            push_front_t<T, remaining_>,
            remaining_
        >;
    };

    // 'get_the_best': given a type list and a type comparator, return the best type according to the comparator
    template<typename TL, template<typename,typename>class CMP>
    struct get_the_best;

    template<typename TL, template<typename,typename>class CMP>
    using best_t = typename get_the_best<TL, CMP>::type;

    template <typename T, template<typename,typename>class CMP>
    struct get_the_best<typelist<T>, CMP> {
        using type = T;
    };

    template <typename ...TS, template<typename,typename>class CMP>
    struct get_the_best<typelist<TS...>, CMP> {
        using first_type = at_t<0, typelist<TS...>>;
        using renaining_types = best_t<pop_front_t<typelist<TS...>>, CMP>;
        using type  = std::conditional_t<
            CMP<first_type, renaining_types>::value,
            first_type, renaining_types
        >;
    };

    // declaration of 'get_ancestors'

    template<typename TYPE, typename TYPELIST>
    struct get_ancestors;

    template<typename TYPE, typename TYPELIST>
    using get_ancestors_t = typename get_ancestors<TYPE, TYPELIST>::type;

    // implementation details for 'get_ancestors' (see below)

    namespace implementation {

        template<typename SRCLIST, typename DESTLIST>
        struct get_ancestors_helper;

        template<typename SRCLIST, typename DESTLIST>
        struct get_ancestors_helper {

            template<typename B>
            using negation_t = typename std::integral_constant<bool, !bool(B::value)>::type;

            template<typename T, typename U>
            using cmp = typename std::is_base_of<T, U>::type;
            using most_ancient = best_t<SRCLIST, cmp>;

            template<typename T>
            using not_most_ancient_t = negation_t<std::is_same<most_ancient, T>>;

            using all_but_most_ancient = filter_t<SRCLIST, not_most_ancient_t>;

            using type = typename implementation::get_ancestors_helper<
                all_but_most_ancient,
                push_back_t<most_ancient, DESTLIST>
            >::type;

        };

        template<typename DESTLIST>
        struct get_ancestors_helper<emptylist, DESTLIST> {
            using type = DESTLIST;
        };

    } // namespace implementation

    // 'get_ancestors': given a type and a typelist it returns a typelist with the class hierarchy from most ancient to the type

    template<typename TYPE, typename TYPELIST>
    struct get_ancestors {
        static_assert(is_typelist<TYPELIST>::value, "The second parameter type must be mpml::typelist");

        template<typename U>
        using base_of_T = typename std::is_base_of<U, TYPE>::type;
        using src_list = filter_t<TYPELIST, base_of_T>;
        using type = typename implementation::get_ancestors_helper<
            src_list,
            emptylist
        >::type;
    };

    // misc help functions

    template <typename T, std::size_t = sizeof(T)>
    auto is_defined(T*)  -> std::true_type;
    auto is_defined(...) -> std::false_type;

} // namespace mpml

/*
    == Macro based interface ====================

    Series of macros to DECLARE, MODIFY and READ type-lists

    MPML_DECLARE(_name)    -> Equivalent to defining the type-list history
    MPML_ADD(_type, _name) -> Equivalent to defining a new entry in the history of the type-list with the types added
    MPML_TYPES(_name)      -> Equivalent to the type that represents the most updated version of the type-list

*/

#define MPML_DECLARE(_name)    INTERNAL_MPML_DECLARE(_name, __COUNTER__)
#define MPML_ADD(_type, _name) INTERNAL_MPML_ADD(_name, _type, __COUNTER__)
#define MPML_TYPES(_name)      mpml::_name##_mpml_read_t<__COUNTER__ - 1>

/*
    == Macro based implementation ====================

    Note: it is recommended to read the mentioned articles
*/

#define INTERNAL_MPML_DECLARE(_name, _idx)\
    namespace mpml {\
        /* Declare the type-list history starting at entry _idx with an empty one*/\
        template<std::size_t IDX>\
        struct _name##_mpml_history;\
        \
        template<std::size_t IDX>\
        using _name##_mpml_history_t = typename _name##_mpml_history<IDX>::type;\
        \
        template<> struct _name##_mpml_history<_idx> {\
            using type = mpml::emptylist;\
        };\
        \
        /* Alias to check if an entry at IDX exists */\
        template <std::size_t IDX>\
        using _name##_mpml_is_defined = decltype(mpml::is_defined(std::declval<_name##_mpml_history<IDX>*>()));\
        \
        /* Define the reader base struct for index 'IDX' */\
        template<std::size_t IDX, bool = std::is_same<std::true_type, _name##_mpml_is_defined<IDX>>::value>\
        struct _name##_mpml_read;\
        \
        template<std::size_t IDX>\
        using _name##_mpml_read_t = typename _name##_mpml_read<IDX, std::is_same<std::true_type, _name##_mpml_is_defined<IDX>>::value>::type;\
        \
        /* When the history entry does exist */\
        template<std::size_t IDX>\
        struct _name##_mpml_read<IDX, true> {\
            using type = _name##_mpml_history_t<IDX>;\
        };\
        \
        /* When the history entry does NOT exist, we need to go back until  */\
        /* we get an existing entry or back to the original entry which has */\
        /* the empty type list                                              */\
        template<std::size_t IDX>\
        struct _name##_mpml_read<IDX, false> {\
            using type = std::conditional_t< \
                (IDX > _idx),                 /* Are there more specializations to check? */\
                _name##_mpml_read_t<IDX - 1>, /* yes */\
                mpml::emptylist               /* no => failed => empty typelist */\
            >;\
        };\
    }

#define INTERNAL_MPML_ADD(_name, _type, _idx)\
    /* Define the current type-list at index _idx (entries might not be consecutive) */\
    namespace mpml {\
        template<>\
        struct _name##_mpml_history<_idx> {\
            using type = push_back_t<_type, _name##_mpml_read_t<_idx - 1>>;\
        };\
    }

