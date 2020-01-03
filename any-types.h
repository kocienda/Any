//
// any-types.h
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
#include <initializer_list>
#include <string>
#include <vector>

//
// A structure that is trivially-copyable and trivially-destructible
//
struct Trivial
{
    explicit Trivial(int _i) : i(_i) {}
    int i;
};

std::ostream &operator<<(std::ostream &os, const Trivial &v)
{
    return os << v.i;
}


//
// A structure that is two machine words in size and not trivially-destructible
//
struct NonTrivial
{
    explicit NonTrivial(int _i) : i(_i) {}
    ~NonTrivial() {
        free(p);
    }
    int i;
    void *p = nullptr;
};

std::ostream &operator<<(std::ostream &os, const NonTrivial &v)
{
    return os << v.i;
}

//
// A structure that is three machine words in size and not trivially-destructible
//
struct NonTrivialString
{
    explicit NonTrivialString(const std::string &_s) : s(_s) {}
    ~NonTrivialString() {
        free(p);
    }
    std::string s;
    void *p = nullptr;
};

std::ostream &operator<<(std::ostream &os, const NonTrivialString &v)
{
    return os << v.s;
}


//
// A structure that is four machine words in size
//
struct NeedsAlloc
{
    NeedsAlloc(int i) : n1(i), n2(i), n3(i), n4(i) {}
    NonTrivial n1;
    NonTrivial n2;
    NonTrivial n3;
    NonTrivial n4;
};

std::ostream &operator<<(std::ostream &os, const NeedsAlloc &v)
{
    return os << v.n1 << ":" << v.n2 << ":" << v.n3 << ":" << v.n4;
}


//
// A structure with an initializer list constructor
//
template <class T>
struct InitList {
    InitList(std::initializer_list<T> l) : v(l) {}
    std::vector<T> v;
};

template <class T>
std::ostream &operator<<(std::ostream &os, const InitList<T> &v)
{
    for (const auto i : v.v) {
        os << i;
    }
    return os;
}
