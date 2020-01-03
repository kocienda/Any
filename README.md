# Any
Implementation and optimization of std::any

<a name="introduction"></a>
## Introduction

I'm fascinated by C++ [`std::any`](https://en.cppreference.com/w/cpp/utility/any), and by the prospect of using a strongly-typed language to create a class to hold values of any type. This document and the code in this repository describes my investigations into the details of making an "Any" implementation of my own. I based my work on a study of the standard library implementations of `std::any` in the [LLVM/libcxx](https://github.com/llvm-mirror/libcxx/blob/master/include/any) and [GCC/libstdc++](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/std/any) projects. In the process, I've developed what I believe to be new efficiencies, and I'm publishing them here for your comment and use, as well as your support or refutation.

NB. In this document, Any with a leading capital letter refers to the general idea of a class capable of holding values regardless of type, including my own implementation, while_ `std::any` refers to the specific API and embodiment of this notion as offered in C++17.

## Files

A list of files in the repository with descriptions.

* `cyto-any.h`: My implementation of an Any class based on `std::any`.
* `xllvm-any.h`: My lightly-editing and reformatted version of `std::any` from the LLVM/libcxx project, version 11.0.0.
* `llvm-any.h`: The `std::any` file from the LLVM/libcxx project, version 11.0.0.
* `xgcc-any.h`: My lightly-editing and reformatted version of `std::any` from the GCC/libstdc++ project, version 9.2.0.
* `gcc-any.h`: The `std::any` file from the GCC/libstdc++ project, version 9.2.0.
* `benchmark`: Micro benchmark tests that use [Google benchmark](https://github.com/google/benchmark).
* `README.md`: The file you're reading now.

## Fundamentals

The `std::any` class is a type-safe container for "empty" values, as well as for values of types that are _[CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)_, the API provides a family of `any_cast` functions to access these stored values, and the standard encourages implementations to avoid heap allocations for small objects that are _[NoThrowMoveConstructible](https://en.cppreference.com/w/cpp/types/is_move_constructible)_.

The versions of `std::any` in LLVM/libcxx and GCC/libstdc++ share a similar plan for satisfying these design goals, and are built around the same fundamentals: 

* A type-erased _Storage_ union that includes a three-word buffer to store small values inline, a `void *` to hold the address of heap-allocated memory for larger values.

* A set of internal functions to manage type-checking and the lifecycle of values (e.g. constructing, copying, moving, destructing).

* A reliance on templates and compile-time checks to choose code paths to handle values of different types, thus limiting the need for runtime checks and the associated computation.

* A use of only one additional machine word (on top of the three already used by the _Storage_ union) to point to a function that handles the type-checking and value-lifecycle work, thus keeping the `sizeof(std::any)` as small as possible.

## Details

Both LLVM/libcxx and GCC/libstdc++ `std::any` implementations have essentially two code paths, one for small values copied into the internal storage buffer, and another for large values where the memory the hold the values is dynamically allocated on the heap. The selection of small/internal or large/external is done in the `std::any` constructor by using C++ [type support](https://en.cppreference.com/w/cpp/types) facilities and [SFNIAE](https://en.cppreference.com/w/cpp/language/sfinae) checks on the type of value passed into the constructor to instantiate a specific template. 

Each template contains a static handle/manage function that switches on "actions" for type-checking, copying, moving, and destructing values based on whether the value is a small/internal or large/external type. The `std::any` constructor stores the pointer to this static function in the newly-created instance as it also completes the work to create the correct type-erased storage for the passed-in value. 

As an `std::any` instance goes through its lifecycle, it's the pointer to this static template function that provides the necessary type-appropriate code. This function pointer needs to be set up in the constructor, since none of the other functions in the `std::any` API are templates, and thus have no way to specify the type that the `std::any` instance stores.

A pseudo-code sketch of this design looks as follows:

```c++
class PseudoAny {
public:
    
    template <class T> 
    PseudoAny(T &&t) : handler(&Handler<T>::handle) {...}

    ~PseudoAny() {
        // call the handler function, passing the appropriate action code
        handler(Action::Drop, ...);
    }

private:
    enum Action { Copy, Move, Drop... };

    template <class T> struct Handler {
        static void handle(Action action, ...) {
            switch (action) {
                case Copy: { ...; break; }
                case Move: { ...; break; }
                case Drop: { ...; break; }
            }
        }
    };
    
    // function pointer from the small/internal or large/external 
    // template is stored in the PseudoAny instance for later use
    void (*handler)(Action, ...);
};
```

For a more complete embodiment of this design, see my lightly edited rewrites of the LLVM/libcxx and GCC/libstdc++ `std::any` classes. See the `xllvm-any.h` and `xgcc-any.h` files in this repository, respectively.

N.B. As I did these rewrites, I tried to remain true to the flow of the originals, but I took the liberty of changing names and removing underscores to make a more reader-friendly formatting than seems _de rigeur_ for standard library implementations.

## Optimization

As I studied this code, I thought of two possible improvements, which I've implemented in a class called Any in `cyto-any.h` in this repository.

1. The addition of Action functions for values that are trivial in one of two ways. If a value is [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable), it can be copied and moved with `memcpy`. If a value is [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), its destructor function does no work. In effect, this creates an third _trivial/internal_ code path in addition to small/internal and large/external.

2. The replacement of the function pointer member variable with a pointer to an actions structure ("Actions") containing four function pointers. The "trick" behind this optimization is to arrange for the Actions structure to be `static constexpr`, and created by a templated "Traits" class that requires the type of the value passed to the Any constructor. It's the job of the Traits class to produce the Actions structure free of type dependencies, which makes it possible to store a `const Actions *` directly as a member variable in the Any class. From the standpoint of the source code, this has the effect of eliminating the `std::any` call to through the switch statement for copy, move, etc. actions, replacing it with a call through a function pointer stored in the Actions structure. This change seems to help the compiler generate smaller and more efficient code.

```c++
class PseudoAny {
public:
    
    template <class T> 
    PseudoAny(T &&t) : actions(&Traits<T>::actions) {}

    ~PseudoAny() {
        // call the appropriate function stored in the Actions struct
        actions->drop(...);
    }

private:
    enum Action { A, B, C, D };

    struct Actions {
        using Copy = void(*)(...);
        using Move = void(*)(...);
        using Drop = void(*)(...);
        
        constexpr Actions(Copy c, Move m, Drop d) : copy(c), move(m), drop(d) {}
        
        Copy copy;
        Move move;
        Drop drop;
    };

    template <class T> struct Traits {
        
        template <class X = T, std::enable_if_t<IsSmall<X> && std::is_trivially_copyable_v<X>...>
        static void copy(void) {...}

        template <class X = T, std::enable_if_t<IsSmall<X> && !std::is_trivially_copyable_v<X>...>
        static void copy(void) {...}
        
        template <class X = T, std::enable_if_t<IsLarge<X>...>
        static void copy(void) {...}

        // set of move functions enabled by type checks
        template <class X = T, std::enable_if_t<...>
        static void move(void) {...}
        
        ...
        
        // set of drop functions enabled by type checks
        template <class X = T, std::enable_if_t<...>
        static void drop(void) {...}
        
        ...
        
        static constexpr Actions actions = Actions(copy<T>, move<T>, drop<T>);
    };
    
    // 
    const Actions *actions;
};
```


## Benchmarks




