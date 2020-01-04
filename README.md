# Any

<a name=”introduction”></a>
I’m fascinated by C++ [`std::any`](https://en.cppreference.com/w/cpp/utility/any), and by the prospect of using a strongly-typed language to create a class to hold values of any type. This document and the code in this repository describes my investigations into making an “Any” class of my own, one that runs faster than the standard library implementations of `std::any` in the [LLVM/libcxx](https://github.com/llvm-mirror/libcxx/blob/master/include/any) and [GCC/libstdc++](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/std/any) projects. I started my work by reading through theirs, and I learned a lot by studying the experts.

The [`Cyto::Any`](https://github.com/kocienda/Any/blob/master/cyto-any.h) class I wrote does indeed run faster, approaching 2x faster for some common usage patterns according to my tests, even more in other cases. This is thanks to an optimization idea I describe below, a riff on [virtual function tables](https://en.wikipedia.org/wiki/Virtual_method_table). I’m publishing my code and results here for your comment and use, as well as for your confirmation or refutation.

NB. In this document, Any with a leading capital letter refers to the general idea of a class capable of holding values regardless of type, including my own implementation, while_ `std::any` refers to the specific API and embodiment of this notion as offered in C++17.

## Files

A list of files in the repository with descriptions.

* [`cyto-any.h`](https://github.com/kocienda/Any/blob/master/cyto-any.h): My implementation of an Any class based on `std::any`.
* [`xllvm-any.h`](https://github.com/kocienda/Any/blob/master/xllvm-any.h): My lightly-edited and reformatted version of `std::any` from the LLVM/libcxx project, version 11.0.0. This file is meant for study.
* [`llvm-any.h`](https://github.com/kocienda/Any/blob/master/llvm-any.h): The unedited `std::any` file from the LLVM/libcxx project, version 11.0.0.
* [`xgcc-any.h`](https://github.com/kocienda/Any/blob/master/xgcc-any.h): My lightly-edited and reformatted version of `std::any` from the GCC/libstdc++ project, version 9.2.0. This file is meant for study.
* [`gcc-any.h`](https://github.com/kocienda/Any/blob/master/gcc-any.h): The unedited `std::any` file from the GCC/libstdc++ project, version 9.2.0.
* [`any-types.h`](https://github.com/kocienda/Any/tree/master/any-types.h): Some simple structs used as Any values in tests.
* [`benchmark`](https://github.com/kocienda/Any/tree/master/benchmark): Micro benchmark tests that use [Google benchmark](https://github.com/google/benchmark).
* `README.md`: The file you’re reading now.

## Fundamentals

The `std::any` class is a type-safe container for “empty” values, as well as for values of types that are _[CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)_, the API provides a family of `any_cast` functions to access these stored values, and the standard encourages implementations to avoid heap allocations for small objects that are _[NoThrowMoveConstructible](https://en.cppreference.com/w/cpp/types/is_move_constructible)_.

The versions of `std::any` in LLVM/libcxx and GCC/libstdc++ share a similar plan for satisfying these design goals, and are built around the same fundamentals: 

* A type-erased _Storage_ union that includes a three-word buffer to store small values inline, a `void *` to hold the address of heap-allocated memory for larger values.

* A set of internal functions to manage type-checking and the lifecycle of values (e.g. constructing, copying, moving, destructing).

* A reliance on templates and compile-time checks to choose code paths to handle values of different types, thus limiting the need for runtime checks and the associated computational overhead.

* A use of only one additional machine word (on top of the three already used by the _Storage_ union) to point to a function that handles the type-checking and value-lifecycle work, thus keeping the `sizeof(std::any)` as small as possible.

## Details

Both LLVM/libcxx and GCC/libstdc++ `std::any` implementations have essentially two code paths, one for small values copied into the internal storage buffer, and another for large values where the memory the hold the values is dynamically allocated on the heap. The selection of small/internal or large/external strategies is done in the `std::any` constructor by using C++ [type support](https://en.cppreference.com/w/cpp/types) facilities and [SFNIAE](https://en.cppreference.com/w/cpp/language/sfinae) checks to instantiate a specific template. 

Each template contains a static handle/manage function that switches on “actions” for type-checking, copying, moving, and destructing values based on whether the value is a small/internal or large/external type. The `std::any` constructor stores the pointer to this static function in the newly-created instance as it also completes the work to create the correct type-erased storage for the passed-in value. 

As an `std::any` instance goes through its lifecycle, it’s the pointer to this static template function that provides the necessary type-appropriate code to do the work of the class. This function pointer needs to be set up in the constructor, since none of the other functions in the `std::any` API are templates, and thus have no efficient way to access the type that the `std::any` instance stores.

A pseudo-code sketch of this design looks as follows:

```c++
class PseudoAny {
public:
    
    template <class T> 
    PseudoAny(T &&t) : handler(&Handler<T>::handle) {...}

    ~PseudoAny() {
        // Call the handler function, passing the appropriate action code.
        handler(Action::Drop, ...);
    }

private:
    enum Action { Copy, Move, Drop... };

    // Calls the appropriate function by switching on the Action code.
    template <class T> struct Handler {
        static void handle(Action action, ...) {
            switch (action) {
                case Copy: { ...; break; }
                case Move: { ...; break; }
                case Drop: { ...; break; }
            }
        }
    };
    
    // Function pointer from the small/internal or large/external 
    // template is stored in the PseudoAny instance for later use.
    void (*handler)(Action, ...);
};
```

For more complete embodiments of this design, review the LLVM/libcxx and GCC/libstdc++ `std::any` classes I’ve included in this repository, as well as my lightly edited rewrites of the same: `xllvm-any.h` and `xgcc-any.h`.

N.B. As I did these rewrites, I tried to remain true to the flow of the originals, but I took the liberty of changing names and removing underscores to make a more reader-friendly formatting than seems customary for standard library implementations.

## Optimization

As I studied this code, I thought of two possible improvements, which I’ve implemented in a class called `Cyto::Any`. See `cyto-any.h` in this repository.

1. <a name=”optimization-1”>The</a> addition of Action functions for values that are trivial in one of two ways. If a value is [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable), it can be copied and moved with `memcpy`. If a value is [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), its destructor function does no work. In effect, this creates an third _trivial/internal_ code path in addition to small/internal and large/external.

2. <a name=”optimization-2”>The</a> replacement of the function pointer member variable with a pointer to an actions structure (“Actions”) containing a collection of function pointers. The “trick” behind this optimization is to arrange for the Actions structure to be `static constexpr`, and created by a templated “Traits” class that is instantiated with the type of the value passed to the Any constructor. It’s the job of the Traits class to produce the Actions structure free of type dependencies, which makes it possible to store a `const Actions *` directly as a member variable in the Any class. This eliminates the `std::any` call through the switch statement for copy, move, etc. actions, replacing it with a call through a function pointer stored in the Actions structure. This is like a hand-coded [vtable](https://en.wikipedia.org/wiki/Virtual_method_table). The crucial point is that member variables in the Any class must be of the same concrete type (so the code will compile), but must be able to work with values of all types (so the code runs). The compile-time setup of the Actions structure function pointers by the Traits template makes this possible.


```c++
class PseudoAny {
public:
    
    // Instantiate a Traits template and extract the pointer to its Actions structure,
    // setting it as a member variable of the instance.
    template <class T> 
    PseudoAny(T &&t) : actions(&Traits<T>::actions) {}

    ~PseudoAny() {
        // Call the appropriate function stored in the Actions struct.
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
        
        // The set of available functions for the copy operation
        // Each class T must resolve to one and only one function.
        template <class X = T, std::enable_if_t<IsSmall<X> && std::is_trivially_copyable_v<X>...>
        static void copy(void) {...}

        template <class X = T, std::enable_if_t<IsSmall<X> && !std::is_trivially_copyable_v<X>...>
        static void copy(void) {...}
        
        template <class X = T, std::enable_if_t<IsLarge<X>...>
        static void copy(void) {...}
        

        // The set of move functions enabled by type checks.
        template <class X = T, std::enable_if_t<...>
        static void move(void) {...}
        
        ...
        
        
        // The set of drop functions enabled by type checks.
        template <class X = T, std::enable_if_t<...>
        static void drop(void) {...}
        
        ...
        
        // Selects a function implementation for each action based on the type of T.
        static constexpr Actions actions = Actions(copy<T>, move<T>, drop<T>);
    };
    
    // This pointer can reference a variety of functions for different types
    // while also being a single concrete type itself so it can serve as 
    // a member variable for the non-templated Any class.
    const Actions *actions;
};
```


## Benchmarks

I used [Google benchmark](https://github.com/google/benchmark) to test my optimization ideas, however it was more difficult to come up with appropriate tests than I would have liked. Since `std::any` is merely a holder of data, rather than a function or algorithm that produces a result (like a sort routine), it’s easy to write tests that exercise the wrong things. For example, calling `malloc` is far more expensive than work done by the Any classes, so the benchmark winds up measuring `malloc`. I tried to avoid this by doing little work other than running Any instances through their lifecycle by constructing, assigning, copying, and destructing them.

The simple goal of running these benchmarks was to determine how well my two optimization ideas fared, i.e. the performance of `Cyto::Any` relative to the `std::any` implementations available in the LLVM and GCC standard libraries.


### Test Descriptions

* [`int-test.cpp`](https://github.com/kocienda/Any/blob/master/benchmark/int-test.cpp): Uses `int` values to test small-value code paths.
* [`trivial-test.cpp`](https://github.com/kocienda/Any/blob/master/benchmark/trivial-test.cpp): Uses a “trivial” structure that is [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable) and [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), again to small-value code paths.
* [`non-trivial-test.cpp`](https://github.com/kocienda/Any/blob/master/benchmark/non-trivial-test.cpp): Uses a “non-trivial” structure that is not [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable) or [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), but is `sizeof(2 * void *)`, to see how “small” an implementation’s small-value limit is.
* [`non-trivial-string-test.cpp`](https://github.com/kocienda/Any/blob/master/benchmark/non-trivial-string-test.cpp): Uses a “non-trivial” structure that is not [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable), or [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), but is `sizeof(4 * void *)`, and uses a `std::string`, surely a commonly-used type for an Any implementation.
* [`needs-alloc-test.cpp`](https://github.com/kocienda/Any/blob/master/benchmark/needs-alloc-test.cpp): Uses a “non-trivial” structure that is not [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable) and  [_TriviallyDestructible_](https://en.cppreference.com/w/cpp/types/is_destructible), but is `sizeof(4 * void *)`, to ensure that the “large” code path is taken, and heap allocations are done to store values in an Any instance.
* [`omnibus.cpp`](https://github.com/kocienda/Any/blob/master/benchmark/omnibus.cpp): Uses all the types mentioned above to see how well an Any instance can change from holding one type to another type during its lifetime.

### Tests Files

The tests I ran are included in the [`benchmark`](https://github.com/kocienda/Any/tree/master/benchmark) subdirectory of this repository for your study. I leave it as an exercise for you to set up compilers and Google benchmark if you’re interested in running these tests yourself.

### Host

I ran all tests on a MacBook Pro (16-inch, 2019), with macOS Catalina (10.15.2/19C57). Google benchmark reported my machine as having:
```
16 X 2300 MHz CPUs
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 256 KiB (x8)
  L3 Unified 16384 KiB (x1)
```

### Compilers

I compiled and ran the tests with two different compilers: 

1. ```Apple clang version 11.0.0 (clang-1100.0.33.12), Target: x86_64-apple-darwin19.2.0```
2. ```gcc version 9.2.0 (Homebrew GCC 9.2.0_3)```

### Methodology

Each compiler used the C++ standard library it shipped with, hence the baseline used for each compiler is what you, the programmer, would get if you did an `#include <any>` and created an instance of `std::any`.

To run the tests, I turned off all networking on my machine, and quit all applications other than the terminal, then I ran each test twenty (20) times. The numbers I report are the **minimum** value from the test run. That’s right, the **single fastest time seen**, not a mean, median, or mode, or any statistical calculation. My experience many years ago working at Apple trying to make the Safari 1.0 web browser as speedy as possible taught me that the fastest number is always what you want, since that number tells how fast the code can go when it’s as free from system noise as you can get. Barring other concerns, like bugs, as long as the other times are within a range of a couple percent, always use the fastest time for a given test. It’s a good number to chase.

### Results

The results show that `Cyto::Any` consistently equals or outperforms the standard library implementations in all tests with both compilers.

![Performance of Cyto::Any vs std::any from LLVM/libcxx and GCC/libstdc++](https://github.com/kocienda/Any/blob/master/resources/benchmark-chart.png)

The only surprising result is the GCC `std::any` performance on “smallish” values, attributable to the implementation choice to only consider values of one machine word as “small”. Obviously, this saves on size. However, in my opinion, it’s too miserly a tradeoff, given the performance cliff the code falls off once `malloc` is called.

Otherwise, `Cyto::Any` is fastest. GCC’s `std::any` is more efficient than LLVM’s `std::any` except where the latter benefits from its more liberal definition of “small” values.

Note that the speed improvement in `Cyto::Any` is wholly attributable to [optimization #2](#optimization-2), the vtable-ish _Actions structure_ optimization. It’s easy to see the code-generation improvement to go along with the benchmark numbers using tools like [Compiler Explorer](https://godbolt.org) or [Hopper Disassembler](https://www.hopperapp.com). Perhaps this code flow piggybacks on devirtualization optimizations compilers already have to make virtual function dispatch faster, but I don’t know enough about compiler internals to say for sure. In any case, replacing the switch statement helps the compiler to generate smaller and more efficient code. 

It turns out that [optimization #1](#optimization-1) yields no improvement, since the compilers are smart enough to generate code equivalent to the handwritten `memcpy` if the structures in the source code are [_TriviallyCopyable_](https://en.cppreference.com/w/cpp/types/is_trivially_copyable). I don’t show these results in the graphs or charts, since there’s nothing interesting to see, but it’s worth saying that compilers (and compiler-writers) are smart about trivial structures. In the end, I left the `memcpy` optimization in the `Cyto::Any` source, but it’s compiled out by default, controlled by the `ANY_USE_SMALL_MEMCPY_STRATEGY` macro.

## That’s All, Folks

If you’ve read down this far, I hope you’ve found this project interesting. If you have questions or comments, find me on Twitter: [@kocienda](https://twitter.com/kocienda).
