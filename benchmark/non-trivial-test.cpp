//
// non-trivial-test.cpp
//
// MIT License
// 
// Copyright (c) 2020 Ken Kocienda
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <any>

#include <any-types.h>
#include <xllvm-any.h>
#include <xgcc-any.h>
#include <cyto-any.h>

static void std_any_test(benchmark::State &state)
{
    using namespace std;
    using A = std::any;
    A r;
    for (auto _ : state) {
        int i = 0;
        benchmark::DoNotOptimize(i += 1);
        NonTrivial v1(i);
        A a1 = v1;    
        A a2(a1);    
        A a3 = a1;
        NonTrivial v2 = std::any_cast<NonTrivial>(a3);
        r = v2;
    }
    int x;
    benchmark::DoNotOptimize(x = std::any_cast<NonTrivial>(r).i);
}

static void xllvm_any_test(benchmark::State &state)
{
    using namespace XLLVM;
    using A = XLLVM::Any;
    A r;
    for (auto _ : state) {
        int i = 0;
        benchmark::DoNotOptimize(i += 1);
        NonTrivial v1(i);
        A a1 = v1;    
        A a2(a1);    
        A a3 = a1;
        NonTrivial v2 = XLLVM::any_cast<NonTrivial>(a3);
        r = v2;
    }
    int x;
    benchmark::DoNotOptimize(x = XLLVM::any_cast<NonTrivial>(r).i);
}

static void xgcc_any_test(benchmark::State &state)
{
    using namespace XGCC;
    using A = XGCC::Any;
    A r;
    for (auto _ : state) {
        int i = 0;
        benchmark::DoNotOptimize(i += 1);
        NonTrivial v1(i);
        A a1 = v1;    
        A a2(a1);    
        A a3 = a1;
        NonTrivial v2 = XGCC::any_cast<NonTrivial>(a3);
        r = v2;
    }
    int x;
    benchmark::DoNotOptimize(x = XGCC::any_cast<NonTrivial>(r).i);
}

static void cyto_any_test(benchmark::State &state)
{
    using namespace Cyto;
    using A = Cyto::Any;
    A r;
    for (auto _ : state) {
        int i = 0;
        benchmark::DoNotOptimize(i += 1);
        NonTrivial v1(i);
        A a1 = v1;    
        A a2(a1);    
        A a3 = a1;
        NonTrivial v2 = Cyto::any_cast<NonTrivial>(a3);
        r = v2;
    }
    int x;
    benchmark::DoNotOptimize(x = Cyto::any_cast<NonTrivial>(r).i);
}

BENCHMARK(std_any_test)->Unit(benchmark::kNanosecond);
BENCHMARK(xllvm_any_test)->Unit(benchmark::kNanosecond);
BENCHMARK(xgcc_any_test)->Unit(benchmark::kNanosecond);
BENCHMARK(cyto_any_test)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
