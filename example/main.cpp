/*
    MIT License

    Copyright (c) 2019-2020 Raúl Ramos

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#include <iostream>
#include <iomanip>
#include <utility>
#include "mpml.h"

MPML_DECLARE(REG_TYPES); // <--- Here we declare the type list which is initially empty

/*

== Our sample class hierarchy ==========

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
class A { };                   class F { };
class B : public A { };        class G : public F { };
class C : public A { };        class L : public G { };
class T : public B { };        class Z : public G { };
class D : public C { };        class H : public F { };
class E : public C { };        class I : public H { };
                               class J : public H { };
                               class K : public I, public J { };
                               class W : public K { };

// note: registry order or duplicated classes do not matter

MPML_ADD(C, REG_TYPES); // <--- Here we add our first type as the file is being compiled
MPML_ADD(D, REG_TYPES);
MPML_ADD(E, REG_TYPES);
MPML_ADD(T, REG_TYPES);
MPML_ADD(B, REG_TYPES);
MPML_ADD(A, REG_TYPES);
MPML_ADD(A, REG_TYPES);
MPML_ADD(A, REG_TYPES);

MPML_ADD(F, REG_TYPES);
MPML_ADD(G, REG_TYPES);
MPML_ADD(L, REG_TYPES);
MPML_ADD(Z, REG_TYPES);
MPML_ADD(H, REG_TYPES);
MPML_ADD(I, REG_TYPES);
MPML_ADD(J, REG_TYPES);
MPML_ADD(K, REG_TYPES);
MPML_ADD(W, REG_TYPES);

// This is the function called per instance of type 'auto' (one instance of this function per type)

auto instance_processor = [](auto _instance) {
    if (_instance) {
        std::cout << "instance addr " << _instance << ": as type '" << typeid(std::remove_pointer_t<decltype(_instance)>).name() << "'\n";
    }
};

// This is the hierarchy iterator general case

template<typename TYPELIST, unsigned LENGTH = TYPELIST::size, unsigned INDEX = 0>
struct hierarchy_iterator {
    static_assert(qcstudio::mpml::is_typelist<TYPELIST>::value, "Not a typelist");
    inline static void exec(void* _p) {
        instance_processor(static_cast<qcstudio::mpml::at_t<INDEX, TYPELIST>*>(_p));
        hierarchy_iterator<TYPELIST, LENGTH, INDEX + 1>::exec(_p);
    }
};

// This is the hierarchy iterator when we are at the end of the type-list

template<typename TYPELIST, unsigned LENGTH>
struct hierarchy_iterator<TYPELIST, LENGTH, LENGTH> {
    inline static void exec(void*) {
    }
};

// Put it all together!

auto main() -> int {

    using namespace qcstudio::mpml;

    // Test "contains"
    {
        static_assert( contains<std::pair<int, float>, typelist<int, float, std::pair<int, float>, double>>::value, "error");
        static_assert(!contains<std::pair<float, int>, typelist<int, float, std::pair<int, float>, double>>::value, "error");
        static_assert(!contains<short,                 typelist<int, int, unsigned>                       >::value, "error");
        static_assert(!contains<short,                 typelist<>                                         >::value, "error");
    }

    // Test get_ancestors
    {
        using D_ANCESTORS = get_ancestors_t<D, MPML_TYPES(REG_TYPES)>; // <--- Here we read registered types so far
        using K_ANCESTORS = get_ancestors_t<K, MPML_TYPES(REG_TYPES)>;
        using W_ANCESTORS = get_ancestors_t<W, MPML_TYPES(REG_TYPES)>;

        using D_EXPECTED = typelist<A, C, D>;
        using K_EXPECTED = typelist<F, H, J, I, K>;
        using W_EXPECTED = typelist<F, H, J, I, K, W>;

        static_assert(std::is_same<D_ANCESTORS, D_EXPECTED>::value, "Hierarchy of D test failed");
        static_assert(std::is_same<K_ANCESTORS, K_EXPECTED>::value, "Hierarchy of K test failed");
        static_assert(std::is_same<W_ANCESTORS, W_EXPECTED>::value, "Hierarchy of W test failed");

        auto d_instance = D{};
        std::cout << "\nThe hierarchy tree of class D is:" << std::endl;
        hierarchy_iterator<D_ANCESTORS>::exec(&d_instance);
        std::cout << "\n";

        auto k_instance = K{};
        std::cout << "The hierarchy tree of class K is:" << std::endl;
        hierarchy_iterator<K_ANCESTORS>::exec(&k_instance);
        std::cout << "\n";

        auto w_instance = W{};
        std::cout << "The hierarchy tree of class W is:" << std::endl;
        hierarchy_iterator<W_ANCESTORS>::exec(&w_instance);
        std::cout << std::endl;
    }

    return 0;
}
