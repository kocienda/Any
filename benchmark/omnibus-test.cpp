//
// omnibus-test.cpp
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
    for (auto _ : state) {
        A a1(3);    
        A a2(4.6f);
        A a3(std::in_place_type<Trivial>, 1);
        A a4(std::in_place_type<NonTrivial>, 2);
        A a5(a4);
        A a6(a1);
        A a7 = a2;
        A a8 = a3;
        A a9(a7);
        A a10 = a4;
        A a11(a3);
        A a12(a4);
        int v1 = std::any_cast<int>(a1);
        float v2 = std::any_cast<float>(a2);
        int v3 = std::any_cast<Trivial>(a3).i;
        int v4 = std::any_cast<NonTrivial>(a4).i;
        int v5 = std::any_cast<NonTrivial>(a5).i;
        int v6 = std::any_cast<int>(a6);
        float v7 = std::any_cast<float>(a7);
        float v8 = std::any_cast<Trivial>(a8).i;
        float v9 = std::any_cast<float>(a9);
        float v10 = std::any_cast<NonTrivial>(a10).i;
        float v11 = std::any_cast<Trivial>(a11).i;
        float v12 = std::any_cast<NonTrivial>(a12).i;
        benchmark::DoNotOptimize(v1);
        benchmark::DoNotOptimize(v2);
        benchmark::DoNotOptimize(v3);
        benchmark::DoNotOptimize(v4);
        benchmark::DoNotOptimize(v5);
        benchmark::DoNotOptimize(v6);
        benchmark::DoNotOptimize(v7);
        benchmark::DoNotOptimize(v8);
        benchmark::DoNotOptimize(v9);
        benchmark::DoNotOptimize(v10);
        benchmark::DoNotOptimize(v11);
        benchmark::DoNotOptimize(v12);
    }
}

static void xllvm_any_test(benchmark::State &state)
{
    using namespace XLLVM;
    using A = XLLVM::Any;
    for (auto _ : state) {
        A a1(3);    
        A a2(4.6f);
        A a3(std::in_place_type<Trivial>, 1);
        A a4(std::in_place_type<NonTrivial>, 2);
        A a5(a4);
        A a6(a1);
        A a7 = a2;
        A a8 = a3;
        A a9(a7);
        A a10 = a4;
        A a11(a3);
        A a12(a4);
        int v1 = XLLVM::any_cast<int>(a1);
        float v2 = XLLVM::any_cast<float>(a2);
        int v3 = XLLVM::any_cast<Trivial>(a3).i;
        int v4 = XLLVM::any_cast<NonTrivial>(a4).i;
        int v5 = XLLVM::any_cast<NonTrivial>(a5).i;
        int v6 = XLLVM::any_cast<int>(a6);
        float v7 = XLLVM::any_cast<float>(a7);
        float v8 = XLLVM::any_cast<Trivial>(a8).i;
        float v9 = XLLVM::any_cast<float>(a9);
        float v10 = XLLVM::any_cast<NonTrivial>(a10).i;
        float v11 = XLLVM::any_cast<Trivial>(a11).i;
        float v12 = XLLVM::any_cast<NonTrivial>(a12).i;
        benchmark::DoNotOptimize(v1);
        benchmark::DoNotOptimize(v2);
        benchmark::DoNotOptimize(v3);
        benchmark::DoNotOptimize(v4);
        benchmark::DoNotOptimize(v5);
        benchmark::DoNotOptimize(v6);
        benchmark::DoNotOptimize(v7);
        benchmark::DoNotOptimize(v8);
        benchmark::DoNotOptimize(v9);
        benchmark::DoNotOptimize(v10);
        benchmark::DoNotOptimize(v11);
        benchmark::DoNotOptimize(v12);
    }
}

static void xgcc_any_test(benchmark::State &state)
{
    using namespace XGCC;
    using A = XGCC::Any;
    for (auto _ : state) {
        A a1(3);    
        A a2(4.6f);
        A a3(std::in_place_type<Trivial>, 1);
        A a4(std::in_place_type<NonTrivial>, 2);
        A a5(a4);
        A a6(a1);
        A a7 = a2;
        A a8 = a3;
        A a9(a7);
        A a10 = a4;
        A a11(a3);
        A a12(a4);
        int v1 = XGCC::any_cast<int>(a1);
        float v2 = XGCC::any_cast<float>(a2);
        int v3 = XGCC::any_cast<Trivial>(a3).i;
        int v4 = XGCC::any_cast<NonTrivial>(a4).i;
        int v5 = XGCC::any_cast<NonTrivial>(a5).i;
        int v6 = XGCC::any_cast<int>(a6);
        float v7 = XGCC::any_cast<float>(a7);
        float v8 = XGCC::any_cast<Trivial>(a8).i;
        float v9 = XGCC::any_cast<float>(a9);
        float v10 = XGCC::any_cast<NonTrivial>(a10).i;
        float v11 = XGCC::any_cast<Trivial>(a11).i;
        float v12 = XGCC::any_cast<NonTrivial>(a12).i;
        benchmark::DoNotOptimize(v1);
        benchmark::DoNotOptimize(v2);
        benchmark::DoNotOptimize(v3);
        benchmark::DoNotOptimize(v4);
        benchmark::DoNotOptimize(v5);
        benchmark::DoNotOptimize(v6);
        benchmark::DoNotOptimize(v7);
        benchmark::DoNotOptimize(v8);
        benchmark::DoNotOptimize(v9);
        benchmark::DoNotOptimize(v10);
        benchmark::DoNotOptimize(v11);
        benchmark::DoNotOptimize(v12);
    }
}

static void cyto_any_test(benchmark::State &state)
{
    using namespace Cyto;
    using A = Cyto::Any;
    for (auto _ : state) {
        A a1(3);    
        A a2(4.6f);
        A a3(std::in_place_type<Trivial>, 1);
        A a4(std::in_place_type<NonTrivial>, 2);
        A a5(a4);
        A a6(a1);
        A a7 = a2;
        A a8 = a3;
        A a9(a7);
        A a10 = a4;
        A a11(a3);
        A a12(a4);
        int v1 = Cyto::any_cast<int>(a1);
        float v2 = Cyto::any_cast<float>(a2);
        int v3 = Cyto::any_cast<Trivial>(a3).i;
        int v4 = Cyto::any_cast<NonTrivial>(a4).i;
        int v5 = Cyto::any_cast<NonTrivial>(a5).i;
        int v6 = Cyto::any_cast<int>(a6);
        float v7 = Cyto::any_cast<float>(a7);
        float v8 = Cyto::any_cast<Trivial>(a8).i;
        float v9 = Cyto::any_cast<float>(a9);
        float v10 = Cyto::any_cast<NonTrivial>(a10).i;
        float v11 = Cyto::any_cast<Trivial>(a11).i;
        float v12 = Cyto::any_cast<NonTrivial>(a12).i;
        benchmark::DoNotOptimize(v1);
        benchmark::DoNotOptimize(v2);
        benchmark::DoNotOptimize(v3);
        benchmark::DoNotOptimize(v4);
        benchmark::DoNotOptimize(v5);
        benchmark::DoNotOptimize(v6);
        benchmark::DoNotOptimize(v7);
        benchmark::DoNotOptimize(v8);
        benchmark::DoNotOptimize(v9);
        benchmark::DoNotOptimize(v10);
        benchmark::DoNotOptimize(v11);
        benchmark::DoNotOptimize(v12);
    }
}

BENCHMARK(std_any_test)->Unit(benchmark::kNanosecond);
BENCHMARK(xllvm_any_test)->Unit(benchmark::kNanosecond);
BENCHMARK(xgcc_any_test)->Unit(benchmark::kNanosecond);
BENCHMARK(cyto_any_test)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
