# Any
Implementation and optimization ideas for std::any

<a name="introduction"></a>
## Introduction

I'm fascinated by C++ [`std::any`](https://en.cppreference.com/w/cpp/utility/any), and by the prospect of using a strongly-typed language to create a class to hold values of any type. This document and the code in this repository describes my investigations into the details of making an "Any" implementation of my own. I based my work on a study of the standard library implementations of `std::any` in the [LLVM/libcxx](https://github.com/llvm-mirror/libcxx/blob/master/include/any) and [GCC/libstdc++](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/std/any) projects. In the process, I've developed what I believe to be new efficiencies, and I'm publishing them here for your comment and use, as well as your support or refutation.

NB. In this document, Any with a leading capital letter refers to the general idea of a class capable of holding values regardless of type, including my own implementation, while_ `std::any` refers to the specific embodiment of this notion, as offered in C++17.

## Files

A list and description of files in the repository. Details to come.

## Fundamentals

The `std::any` class is a type-safe container for "empty" values, as well as for values of types that are _[CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)_, the API provides a family of `any_cast` functions to access these stored values, and the standard encourages implementations to avoid heap allocations for small objects that are _[NoThrowMoveConstructible](https://en.cppreference.com/w/cpp/types/is_move_constructible)_.

The versions of `std::any` in LLVM/libcxx and GCC/libstdc++ share a similar plan for satisfying these design goals, and are built around the same fundamentals: 

* A type-erased _Storage_ union that includes a three-word buffer to store small values inline, a `void *` to hold the address of heap-allocated memory for larger values.

* A set of functions to manage type-checking and the lifecycle of values (e.g. copying, moving, destructing).

* A reliance on templates and compile-time checks to choose code paths to handle values of different types, thus limiting the need for runtime checks and the associated computation.

* A use of only one additional word (on top of the three already used by the _Storage_ union) to point to a function that handles the type-checking and value-lifecycle work, thus keeping the `sizeof(std::any)` as small as possible.

## Details

Both LLVM/libcxx and GCC/libstdc++ `std::any` implementations have essentially two code paths, one for small values copied into the internal storage buffer, and another for large values where the memory the hold the values lives "externally" on the heap. The selection of small/internal or large/external is done in the `std::any` constructor by using C++ [type support](https://en.cppreference.com/w/cpp/types) facilities and [SFNIAE](https://en.cppreference.com/w/cpp/language/sfinae) checks on the type of value passed into the constructor to instantiate a specific template. Each templates contains a static handle/manage function that switches on "actions" for type-checking, copying, moving, and destructing values based on whether the value is a small/internal or large/external type. The `std::any` constructor stores the pointer to this static function in the newly-created instance as it also completes the work to create the correct type-erased storage for the passed-in value. Later on, since none of the other functions in the `std::any` API are templates, and thus have no way to specify the means to deal with the type the `std::any` instance stores, it's the pointer to this static template function that holds the key to the type-appropriate code that makes a `std::any` instance work after it's been constructed.

A pseudo-code sketch of this design looks as follows:

<pre>
class PseudoAny {
public:
    
    template &lt;class T&gt; 
    PseudoAny(T &&t) : handler(&Handler&lt;T&gt;::handle) {...}

    ~PseudoAny() {
        // call the handler function, passing the appropriate action code
        handler(Action::Drop, ...);
    }

private:
    enum Action { Copy, Move, Drop... };

    template &lt;class T&gt; struct Handler {
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
</pre>

For a more complete embodiment of this design, see my lightly edited rewrite of the LLVM/libcxx `std::any` class. 

N.B. It has far fewer underscores and a more reader-friendly formatting than seems _de rigeur_ for standard library implementations.

## Optimization Ideas

As I studied this code, I thought of two possible improvements, which I've implemented in a class called Any in the file named `cyto-any.h` in this repository.

1. The addition of Action functions for values that are trivial in one of two ways. If a value is [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable), it can be copied and moved with `memcpy`. If a value is [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), its destructor function does no work. In effect, this creates an third _trivial/internal_ code path in addition to small/internal and large/external.

2. The replacement of the function pointer member variable with a pointer to an actions structure ("Actions") containing four function pointers. The "trick" behind this optimization is to arrange for the Actions structure to be `static constexpr`, and created by a templated "Traits" class that requires the type of the value passed to the Any constructor. It's the job of the Traits class to produce the Actions structure free of type dependencies, which makes it possible to store a `const Actions *` directly as a member variable in the Any class. From the standpoint of the source code, this has the effect of eliminating the `std::any` call to through the switch statement for copy, move, etc. actions, replacing it with a call through a function pointer stored in the Actions structure. This change seems to help the compiler generate smaller and more efficient code.

<pre>
class PseudoAny {
public:
    
    template &lt;class T&gt; 
    PseudoAny(T &&t) : actions(&Traits&lt;T&gt;::traits_actions) {}

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

    template &lt;class T&gt; struct Traits {
        
        template &lt;class X = T, std::enable_if_t&lt;IsSmall&lt;X&gt;...&gt;
        static void copy(void) {...}
        
        template &lt;class X = T, std::enable_if_t&lt;IsLarge&lt;X&gt;...&gt;
        static void copy(void) {...}

        template &lt;class X = T, std::enable_if_t&lt;IsSmall&lt;X&gt;...&gt;
        static void move(void) {...}
        
        template &lt;class X = T, std::enable_if_t&lt;IsLarge&lt;X&gt;...&gt;
        static void move(void) {...}

        template &lt;class X = T, std::enable_if_t&lt;IsSmall&lt;X&gt;...&gt;
        static void drop(void) {...}
        
        template &lt;class X = T, std::enable_if_t&lt;IsLarge&lt;X&gt;...&gt;
        static void drop(void) {...}
        
        static constexpr Actions traits_actions = Actions(copy&lt;T&gt;, move&lt;T&gt;, drop&lt;T&gt;);
    };
    
    const Actions *actions;
};
</pre>


## Benchmarks




