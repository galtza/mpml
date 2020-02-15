# Meta-Programming Mini Library (mpml)

## Overview

Small meta-programming library that is mainly used to generate the list of parents of a determined type. Basically from lass **K** we want to retrieve ``` [ F, H, J, I, K ]```

```
                   F
      A          // \
     / \         H   \
    B   C      // \\  \
   /   / \     I   J   G
  T   D   E    \\ //  / \
                <K>  L   Z
                 | 
                 W
```

On the core level, it has the following common meta-programming features:
1. Type-list definition
2. Basic operations like _**push_back**_, _**pop_front**_, _**at**_, etc.
3. More complex operations like _**get_filtered**_ by predicate, _**get_the_best**_ according to a comparator or _**get_ancestors**_ given a type and a type-list
4. Helper macros to help use some features

## Usage

As a compilation unit is being processed we can create, add and read typelists. This can be used for many purposes like class registry.

Use **_MPML_DECLARE_**, on the global or namespace scope in order to declare a list:
```
#include <iostream>
#include <iomanip>
#include "mpml.h"

MPML_DECLARE(REG_TYPES); // <--- Here we declare the type list which is initially empty

...
```

After that, and again in the global or namespace scope, we can keep on adding types to the typelist by using **_MPML_ADD_** like this:

```
struct AType {
    ...
};

MPML_ADD(AType, REG_TYPES); // <--- Here we add the type 'AType'

struct AnotherType {
    ...
};

MPML_ADD(AnotherType, REG_TYPES); // <--- Here we add the another type called 'AnotherType'

```

Finally, we can use **_MPML_TYPES_** anywhere to get the typelist so far. 

In the example, given an instance of a class X we calculate the ancestors and iterate over the types. On each iteration we cast our pointer to the current iteration type and print out the type info. However, we can do more productive things like for instance serializing the class.

## References

For further explanation about how this library was built, please refer to [**hierarchy-inspector article**](https://github.com/galtza/hierarchy-inspector), where you can find a thorough explanation.

