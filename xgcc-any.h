//
// xgcc-any.h
//
// A lightly-edited rewrite of the std::any class from the
// GNU ISO C++ Library, version 9.2.0
//

// <any> -*- C++ -*-

// Copyright (C) 2014-2019 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#ifndef XGCC_ANY
#define XGCC_ANY 1

#include <typeinfo>
#include <new>
#include <utility>
#include <type_traits>

namespace XGCC {

template <class T>  struct IsInPlaceType_ : std::false_type {};
template <>         struct IsInPlaceType_<std::in_place_t> : std::true_type {};
template <class T>  struct IsInPlaceType_<std::in_place_type_t<T>> : std::true_type {};
template <size_t S> struct IsInPlaceType_<std::in_place_index_t<S>> : std::true_type {};
template <class T>  constexpr bool IsInPlaceType = IsInPlaceType_<T>::value;

class bad_any_cast : public std::bad_cast {};

#ifndef XGCC_ALWAYS_INLINE
#ifdef NDEBUG
#define XGCC_ALWAYS_INLINE inline __attribute__ ((__visibility__("hidden"), __always_inline__))
#else
#define XGCC_ALWAYS_INLINE inline
#endif
#endif

#ifdef __cpp_rtti
#define XGCC_USE_TYPEINFO 1
#else
#define XGCC_USE_TYPEINFO 0
#endif

#ifdef __cpp_exceptions
#define XGCC_USE_EXCEPTIONS 1
#else
#define XGCC_USE_EXCEPTIONS 0
#endif

#define XGCC_USE(FEATURE) (defined XGCC_USE_##FEATURE  && XGCC_USE_##FEATURE)


inline void throw_bad_any_cast() {
#if XGCC_USE(EXCEPTIONS)
    throw bad_any_cast{};
#else
    abort();
#endif
}

class Any 
{
    union Storage {
        constexpr Storage() : ptr{nullptr} {}

        Storage(const Storage &) = delete;
        Storage &operator=(const Storage &) = delete;

        void *ptr;
        std::aligned_storage<sizeof(ptr), alignof(void *)>::type buffer;
    };

    template <typename T,
        bool Fits = (sizeof(T) <= sizeof(Storage)) && (alignof(T) <= alignof(Storage))>
    using Internal = std::bool_constant<std::is_nothrow_move_constructible_v<T> && Fits>;

    template <typename T>
    struct Manager_internal; // uses small-object optimization

    template <typename T>
    struct Manager_external; // creates contained object on the heap

    template <typename T>
    using Manager = std::conditional_t<Internal<T>::value, Manager_internal<T>, Manager_external<T>>;

    template <typename T, typename Decayed = std::decay_t<T>>
    using Decay = std::enable_if_t<!std::is_same_v<Decayed, Any>, Decayed>;

    template <typename T, typename... Args, typename Mgr = Manager<T>>
    void do_emplace(Args &&... args) {
        reset();
        Mgr::create(storage, std::forward<Args>(args)...);
        manager = &Mgr::manage;
    }

    template <typename T, typename U, typename... Args, typename Mgr = Manager<T>>
    void do_emplace(std::initializer_list<U> il, Args &&... args) {
        reset();
        Mgr::create(storage, il, std::forward<Args>(args)...);
        manager = &Mgr::manage;
    }

public:
    constexpr Any() noexcept : manager(nullptr) {}

    Any(const Any &other) {
        if (!other.has_value()) {
            manager = nullptr;
        }
        else {
            Arg arg;
            arg.a = this;
            other.manager(Action::Copy, &other, &arg);
        }
    }

    Any(Any&& other) noexcept {
        if (!other.has_value()) {
            manager = nullptr;
        }
        else {
            Arg arg;
            arg.a = this;
            other.manager(Action::Move, &other, &arg);
        }
    }

    template <typename Res, typename T, typename... Args>
    using any_constructible =
        std::enable_if<std::__and_<std::is_copy_constructible<T>, std::is_constructible<T, Args...>>::value, Res>;

    template <typename T, typename... Args>
    using any_constructible_t = typename any_constructible<bool, T, Args...>::type;

    template <typename V, typename T = Decay<V>, typename Mgr = Manager<T>,
        any_constructible_t<T, V&&> = true, std::enable_if_t<!IsInPlaceType<T>, bool> = true>
    Any(V &&v) : manager(&Mgr::manage) {
        Mgr::create(storage, std::forward<V>(v));
    }

    template <typename V, typename T = Decay<V>, typename Mgr = Manager<T>,
        std::enable_if_t<std::__and_<std::is_copy_constructible<T>,
				 std::__not_<std::is_constructible<T, V &&>>,
			         std::__not_<std::in_place_type_t<T>>>::value,
			  bool> = false>
    Any(V &&v) : manager(&Mgr::manage) {
        Mgr::create(storage, v);
    }

    template <typename V, typename... Args, typename T = Decay<V>, typename Mgr = Manager<T>,
        any_constructible_t<T, Args &&...> = false>
    explicit Any(std::in_place_type_t<V>, Args &&... args) : manager(&Mgr::manage) {
        Mgr::create(storage, std::forward<Args>(args)...);
    }

    template <typename V, typename U, typename... Args, typename T = Decay<V>, typename Mgr = Manager<T>,
        any_constructible_t<T, std::initializer_list<U>, Args &&...> = false>
    explicit Any(std::in_place_type_t<V>, std::initializer_list<U> il, Args &&... args) : manager(&Mgr::manage) {
        Mgr::create(storage, il, std::forward<Args>(args)...);
    }

    ~Any() { reset(); }

    Any &operator=(const Any &rhs) {
        *this = Any(rhs);
        return *this;
    }

    Any &operator=(Any&& rhs) noexcept {
        if (!rhs.has_value()) {
            reset();
        }
        else if (this != &rhs) {
            reset();
            Arg arg;
            arg.a = this;
            rhs.manager(Action::Move, &rhs, &arg);
        }
        return *this;
    }

    template <typename V>
    std::enable_if_t<std::is_copy_constructible<Decay<V>>::value, Any &>
    operator=(V &&rhs) {
        *this = Any(std::forward<V>(rhs));
        return *this;
    }

    template <typename V, typename... Args> 
    typename any_constructible<Decay<V> &, Decay<V>, Args &&...>::type
    emplace(Args &&... args) {
        do_emplace<Decay<V>>(std::forward<Args>(args)...);
        Any::Arg arg;
        this->manager(Any::Action::Get, this, &arg);
        return *static_cast<Decay<V>*>(arg.o);
    }

    template <typename V, typename U, typename... Args>
    typename any_constructible<Decay<V> &, Decay<V>, std::initializer_list<U>, Args &&...>::type
    emplace(std::initializer_list<U> il, Args &&... args) {
        do_emplace<Decay<V>, U>(il, std::forward<Args>(args)...);
        Any::Arg arg;
        this->manager(Any::Action::Get, this, &arg);
        return *static_cast<Decay<V>*>(arg.o);
    }

    void reset() noexcept {
        if (has_value()) {
            manager(Action::Drop, this, nullptr);
            manager = nullptr;
        }
    }

    void swap(Any &rhs) noexcept {
        if (!has_value() && !rhs.has_value()) {
            return;
        }

        if (has_value() && rhs.has_value()) {
            if (this == &rhs) {
                return;
            }

            Any tmp;
            Arg arg;
            arg.a = &tmp;
            rhs.manager(Action::Move, &rhs, &arg);
            arg.a = &rhs;
            manager(Action::Move, this, &arg);
            arg.a = this;
            tmp.manager(Action::Move, &tmp, &arg);
        }
        else {
            Any *empty = !has_value() ? this : &rhs;
            Any *full = !has_value() ? &rhs : this;
            Arg arg;
            arg.a = empty;
            full->manager(Action::Move, full, &arg);
        }
    }

    bool has_value() const noexcept { return manager != nullptr; }

#if XGCC_USE(TYPEINFO)
    const std::type_info &type() const noexcept {
        if (!has_value()) {
            return typeid(void);
        }
        Arg arg;
        manager(Action::TypeInfo, this, &arg);
        return *arg.t;
    }
#endif

    template <typename T>
    static constexpr bool is_valid_cast() { return std::is_reference_v<T> || std::is_copy_constructible_v<T>; }

private:
    enum class Action {
    	Get, TypeInfo, Copy, Drop, Move
    };

    union Arg {
        void *o;
        const std::type_info *t;
        Any *a;
    };

    void (*manager)(Action, const Any *, Arg *);
    Storage storage;

    template <typename T>
    friend void *any_caster(const Any *any);

    template <typename T>
    struct Manager_internal {
        static void manage(Action which, const Any *any, Arg *arg);

        template <typename U>
        static void
        create(Storage &storage, U &&v) {
            void *addr = &storage.buffer;
            ::new (addr) T(std::forward<U>(v));
        }

	    template <typename... Args>
        static void
        create(Storage &storage, Args &&... args) {
            void *addr = &storage.buffer;
            ::new (addr) T(std::forward<Args>(args)...);
        }
    };

    // Manage external contained object.
    template <typename T>
    struct Manager_external {
        static void
        manage(Action which, const Any *any, Arg *arg);

        template <typename U>
        static void
        create(Storage &storage, U&& v) {
            storage.ptr = new T(std::forward<U>(v));
        }

        template <typename... Args>
        static void
        create(Storage &storage, Args &&... args) {
            storage.ptr = new T(std::forward<Args>(args)...);
        }
    };
};

inline void swap(Any &x, Any &y) noexcept { x.swap(y); }

template <typename T, typename... Args>
Any make_any(Args &&... args) {
    return Any(std::in_place_type<T>, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
Any make_any(std::initializer_list<U> il, Args &&... args) {
    return Any(std::in_place_type<T>, il, std::forward<Args>(args)...);
}

template <typename V>
inline V any_cast(const Any &any) {
    using U = std::remove_cv_t<std::remove_reference_t<V>>;
    static_assert(Any::is_valid_cast<V>(),
        "Template argument must be a reference or CopyConstructible type");
    static_assert(std::is_constructible_v<V, const U&>,
        "Template argument must be constructible from a const value.");
    auto p = any_cast<U>(&any);
    if (!p) {
        throw_bad_any_cast();
    }
    return static_cast<V>(*p);
}

template <typename V>
inline V any_cast(Any &any) {
    using U = std::remove_cv_t<std::remove_reference_t<V>>;
    static_assert(Any::is_valid_cast<V>(),
        "Template argument must be a reference or CopyConstructible type");
    static_assert(std::is_constructible_v<V, U&>,
        "Template argument must be constructible from an lvalue.");
    auto p = any_cast<U>(&any);
    if (!p) {
        throw_bad_any_cast();
    }
    return static_cast<V>(*p);
}

template <typename V>
inline V any_cast(Any &&any) {
    using U = std::remove_cv_t<std::remove_reference_t<V>>;
    static_assert(Any::is_valid_cast<V>(),
        "Template argument must be a reference or CopyConstructible type");
    static_assert(std::is_constructible_v<V, U>,
        "Template argument must be constructible from an rvalue.");
    auto p = any_cast<U>(&any);
    if (!p) {
        throw_bad_any_cast();
    }
    return static_cast<V>(std::move(*p));
}

template <typename T>
void *any_caster(const Any *any) {
    // any_cast<T> returns non-null if any->type() == typeid(T) and
    // typeid(T) ignores cv-qualifiers so remove them:
    using U = std::remove_cv_t<T>;
    // The contained value has a decayed type, so if decay_t<U> is not U,
    // then it's not possible to have a contained value of type U:
    if constexpr (!std::is_same_v<std::decay_t<U>, U>) {
        return nullptr;
    }
    // Only copy constructible types can be used for contained values:
    else if constexpr (!std::is_copy_constructible_v<U>) {
        return nullptr;
    }
    // First try comparing function addresses, which works without RTTI
    else if (any->manager == &Any::Manager<U>::manage
#if XGCC_USE(TYPEINFO)
        || any->type() == typeid(T)
#endif
    ) {
        Any::Arg arg;
        any->manager(Any::Action::Get, any, &arg);
        return arg.o;
    }
    return nullptr;
}

template <typename V>
inline const V* any_cast(const Any *any) noexcept {
    if constexpr (std::is_object_v<V>) {
        if (any) {
            return static_cast<V *>(any_caster<V>(any));
        }
    }
    return nullptr;
}

template <typename V>
inline V* any_cast(Any *any) noexcept {
    if constexpr (std::is_object_v<V>) {
        if (any) {
            return static_cast<V *>(any_caster<V>(any));
        }
    }
    return nullptr;
}

template <typename T>
void Any::Manager_internal<T>::manage(Action which, const Any *any, Arg *arg) {
    auto ptr = reinterpret_cast<const T*>(&any->storage.buffer);
    switch (which) {
        case Action::Get:
            arg->o = const_cast<T *>(ptr);
            break;
        case Action::TypeInfo:
#if XGCC_USE(TYPEINFO)
            arg->t = &typeid(T);
#endif
            break;
        case Action::Copy:
            ::new(&arg->a->storage.buffer) T(*ptr);
            arg->a->manager = any->manager;
            break;
        case Action::Move:
            ::new(&arg->a->storage.buffer) T(std::move(*const_cast<T*>(ptr)));
            ptr->~T();
            arg->a->manager = any->manager;
            const_cast<Any*>(any)->manager = nullptr;
            break;
        case Action::Drop:
            ptr->~T();
            break;
    }
}

template <typename T>
void Any::Manager_external<T>::manage(Action which, const Any *any, Arg *arg) {
    auto ptr = static_cast<const T*>(any->storage.ptr);
    switch (which) {
        case Action::Get:
            arg->o = const_cast<T*>(ptr);
            break;
        case Action::TypeInfo:
#if XGCC_USE(TYPEINFO)
            arg->t = &typeid(T);
#endif
            break;
        case Action::Copy:
            arg->a->storage.ptr = new T(*ptr);
            arg->a->manager = any->manager;
            break;
        case Action::Move:
            arg->a->storage.ptr = any->storage.ptr;
            arg->a->manager = any->manager;
            const_cast<Any*>(any)->manager = nullptr;
            break;
        case Action::Drop:
            delete ptr;
            break;
    }
}

} // namespace std

#endif // XGCC_ANY
