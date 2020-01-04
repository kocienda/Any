//
// xllvm-any.h
//
// A lightly-edited rewrite of the std::any class from the
// LLVM Compiler project, version 11.0.0
// https://github.com/kocienda/Any
//

//===------------------------------ any -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVM-LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef XLLVM_ANY
#define XLLVM_ANY

#include <iostream>

#include <memory>
#include <new>
#include <typeinfo>
#include <type_traits>
#include <cstdlib>

namespace XLLVM {
class bad_any_cast : public std::bad_cast {};

#ifndef XLLVM_ALWAYS_INLINE
#ifdef NDEBUG
#define XLLVM_ALWAYS_INLINE inline __attribute__ ((__visibility__("hidden"), __always_inline__))
#else
#define XLLVM_ALWAYS_INLINE inline
#endif
#endif

#ifdef __cpp_rtti
#define XLLVM_USE_TYPEINFO 1
#else
#define XLLVM_USE_TYPEINFO 0
#endif

#ifdef __cpp_exceptions
#define XLLVM_USE_EXCEPTIONS 1
#else
#define XLLVM_USE_EXCEPTIONS 0
#endif

#define XLLVM_USE(FEATURE) (defined XLLVM_USE_##FEATURE  && XLLVM_USE_##FEATURE)

XLLVM_ALWAYS_INLINE
void throw_bad_any_cast()
{
#if XLLVM_USE(EXCEPTIONS)
    throw bad_any_cast();
#else
    abort();
#endif
}

template <class T>  struct IsInPlaceType_ : std::false_type {};
template <>         struct IsInPlaceType_<std::in_place_t> : std::true_type {};
template <class T>  struct IsInPlaceType_<std::in_place_type_t<T>> : std::true_type {};
template <size_t S> struct IsInPlaceType_<std::in_place_index_t<S>> : std::true_type {};

template <class T>
constexpr bool IsInPlaceType = IsInPlaceType_<T>::value;

class Any;

template <class V>
XLLVM_ALWAYS_INLINE
std::add_pointer_t<std::add_const_t<V>>
any_cast(Any const *) noexcept;

template <class V>
XLLVM_ALWAYS_INLINE
std::add_pointer_t<V> any_cast(Any *) noexcept;

namespace AnyImp
{
  using Buffer = std::aligned_storage_t<3 * sizeof(void *), std::alignment_of<void *>::value>;

    template <class T>
    using IsSmallObject = std::integral_constant<bool, sizeof(T) <= sizeof(Buffer) &&
        std::alignment_of<Buffer>::value % std::alignment_of<T>::value == 0 &&
            std::is_nothrow_move_constructible<T>::value>;

    enum class Action {
        Destroy,
        Copy,
        Move,
        Get,
        TypeInfo
    };

    template <class T> struct SmallHandler;
    template <class T> struct LargeHandler;

    template <class T>
    struct unique_typeinfo { static constexpr int id = 0; };
    template <class T> constexpr int unique_typeinfo<T>::id;

    template <class T>
    XLLVM_ALWAYS_INLINE
    constexpr const void *get_fallback_typeid() {
      return &unique_typeinfo<std::decay_t<T>>::id;
    }

    template <class T>
    XLLVM_ALWAYS_INLINE
    static bool compare_typeid(std::type_info const *id, const void *fallback_id) {
#if XLLVM_USE(TYPEINFO)
        if (id && *id == typeid(T)) {
            return true;
        }
#endif
        if (!id && fallback_id == AnyImp::get_fallback_typeid<T>()) {
            return true;
        }
        return false;
    }

    template <class T>
    using Handler = std::conditional_t<IsSmallObject<T>::value, 
        SmallHandler<T>, LargeHandler<T>>;

}  // namespace AnyImp

class Any
{
public:
    // construct/destruct
    XLLVM_ALWAYS_INLINE
    constexpr Any() noexcept : h(nullptr) {}

    XLLVM_ALWAYS_INLINE
    Any(Any const & other) : h(nullptr) {
        if (other.h) {
            other.call(Action::Copy, this);
        }
    }

    XLLVM_ALWAYS_INLINE
    Any(Any && other) noexcept : h(nullptr) {
        if (other.h) {
            other.call(Action::Move, this);
        }
    }

    template <class V , class T = std::decay_t<V> , class = std::enable_if_t<
        !std::is_same<T, Any>::value && !IsInPlaceType<V> && 
            std::is_copy_constructible<T>::value>>
    XLLVM_ALWAYS_INLINE
    Any(V && value);

    template <class V, class ...Args, class T = std::decay_t<V>, class = std::enable_if_t<
        std::is_constructible<T, Args...>::value &&
        std::is_copy_constructible<T>::value>>
    XLLVM_ALWAYS_INLINE
    explicit Any(std::in_place_type_t<V>, Args &&... args);

    template <class V, class U, class ...Args,
    class T = std::decay_t<V>,
    class = std::enable_if_t<
        std::is_constructible<T, std::initializer_list<U>&, Args...>::value &&
        std::is_copy_constructible<T>::value>>
    XLLVM_ALWAYS_INLINE
    explicit Any(std::in_place_type_t<V>, std::initializer_list<U>, Args &&... args);

    XLLVM_ALWAYS_INLINE
    ~Any() { this->reset(); }

    // assignments
    XLLVM_ALWAYS_INLINE
    Any &operator=(Any const &rhs) {
        Any(rhs).swap(*this);
        return *this;
    }

    XLLVM_ALWAYS_INLINE
    Any &operator=(Any &&rhs) noexcept {
        Any(std::move(rhs)).swap(*this);
        return *this;
    }

    template <class V, class T = std::decay_t<V>, 
        class = std::enable_if_t<!std::is_same<T, Any>::value &&
            std::is_copy_constructible<T>::value>>
    XLLVM_ALWAYS_INLINE
    Any &operator=(V &&rhs);

    template <class V, class ...Args, class T = std::decay_t<V>,
        class = std::enable_if_t<std::is_constructible<T, Args...>::value &&
            std::is_copy_constructible<T>::value>>
    XLLVM_ALWAYS_INLINE
    T &emplace(Args &&... args);

    template <class V, class U, class ...Args,
    class T = std::decay_t<V>,
    class = std::enable_if_t<
        std::is_constructible<T, std::initializer_list<U>&, Args...>::value &&
        std::is_copy_constructible<T>::value>>
    XLLVM_ALWAYS_INLINE
    T &emplace(std::initializer_list<U>, Args &&...);

    // 6.3.3 any modifiers
    XLLVM_ALWAYS_INLINE
    void reset() noexcept { 
        if (h) {
            this->call(Action::Destroy); 
        }
    }

    XLLVM_ALWAYS_INLINE
    void swap(Any &rhs) noexcept;

    // 6.3.4 any observers
    XLLVM_ALWAYS_INLINE
    bool has_value() const noexcept { return h != nullptr; }

#if XLLVM_USE(TYPEINFO)
    XLLVM_ALWAYS_INLINE
    const std::type_info &type() const noexcept {
        if (h) {
            return *static_cast<std::type_info const *>(this->call(Action::TypeInfo));
        } 
        else {
            return typeid(void);
        }
    }
#endif

private:
    typedef AnyImp::Action Action;
    using HandleFuncPtr =  void *(*)(Action, Any const *, Any *, const std::type_info *,
        const void *fallback_id);

    union Storage {
        constexpr Storage() : ptr(nullptr) {}
        void *ptr;
        AnyImp::Buffer buf;
    };

    XLLVM_ALWAYS_INLINE
    void *call(Action a, Any *other_any = nullptr, std::type_info const * info = nullptr,
        const void *fallback_id = nullptr) const {
        return h(a, this, other_any, info, fallback_id);
    }

    XLLVM_ALWAYS_INLINE
    void *call(Action a, Any * other_any = nullptr, std::type_info const * info = nullptr,
        const void *fallback_id = nullptr) {
        return h(a, this, other_any, info, fallback_id);
    }

    template <class>
    friend struct AnyImp::SmallHandler;

    template <class>
    friend struct AnyImp::LargeHandler;

    template <class V>
    friend std::add_pointer_t<std::add_const_t<V>>
    any_cast(Any const *) noexcept;

    template <class V>
    friend std::add_pointer_t<V>
    any_cast(Any *) noexcept;

    HandleFuncPtr h = nullptr;
    Storage s;
};

namespace AnyImp
{

template <class T>
struct SmallHandler 
{
    XLLVM_ALWAYS_INLINE
    static void *handle(Action action, Any const *this_any, Any *other_any,
        std::type_info const *info, const void *fallback_id) {
        switch (action) {
            case Action::Destroy:
                destroy(const_cast<Any &>(*this_any));
                return nullptr;
            case Action::Copy:
                copy(*this_any, *other_any);
                return nullptr;
            case Action::Move:
                move(const_cast<Any &>(*this_any), *other_any);
                return nullptr;
            case Action::Get:
                return get(const_cast<Any &>(*this_any), info, fallback_id);
            case Action::TypeInfo:
                return get_type_info();
        }
        return nullptr;
    }

    template <class ...Args>
    XLLVM_ALWAYS_INLINE
    static T &create(Any &dst, Args &&... args) {
        T *ret = ::new (static_cast<void *>(&dst.s.buf)) T(std::forward<Args>(args)...);
        dst.h = &SmallHandler::handle;
        return *ret;
    }

private:
    XLLVM_ALWAYS_INLINE
    static void destroy(Any &this_any) {
        T &value = *static_cast<T *>(static_cast<void *>(&this_any.s.buf));
        value.~T();
        this_any.h = nullptr;
    }

    XLLVM_ALWAYS_INLINE
    static void copy(Any const &this_any, Any &dst) {
        SmallHandler::create(dst, *static_cast<T const *>(static_cast<void const *>(&this_any.s.buf)));
    }

    XLLVM_ALWAYS_INLINE
    static void move(Any &this_any, Any &dst) {
        SmallHandler::create(dst, std::move(*static_cast<T*>(static_cast<void *>(&this_any.s.buf))));
        destroy(this_any);
    }

    XLLVM_ALWAYS_INLINE
    static void *get(Any &this_any, std::type_info const *info, const void *fallback_id) {
        if (AnyImp::compare_typeid<T>(info, fallback_id)) {
            return static_cast<void *>(&this_any.s.buf);
        }
        return nullptr;
    }

    XLLVM_ALWAYS_INLINE
    static void *get_type_info() {
#if XLLVM_USE(TYPEINFO)
        return const_cast<void *>(static_cast<void const *>(&typeid(T)));
#else
        return nullptr;
#endif
    }
};

template <class T>
struct LargeHandler
{
    XLLVM_ALWAYS_INLINE
    static void *handle(Action action, Any const * this_any, Any *other_any, 
        std::type_info const *info, void const *fallback_id) {
        switch (action) {
        case Action::Destroy:
            destroy(const_cast<Any &>(*this_any));
            return nullptr;
        case Action::Copy:
            copy(*this_any, *other_any);
            return nullptr;
        case Action::Move:
            move(const_cast<Any &>(*this_any), *other_any);
            return nullptr;
        case Action::Get:
            return get(const_cast<Any &>(*this_any), info, fallback_id);
        case Action::TypeInfo:
            return get_type_info();
        }
        return nullptr;
    }

    template <class ...Args>
    XLLVM_ALWAYS_INLINE
    static T &create(Any & dst, Args &&... args) {
#if defined(__clang__)
        typedef std::allocator<T> _Alloc;
        typedef std::__allocator_destructor<_Alloc> D;
        _Alloc a;
        std::unique_ptr<T, D> hold(a.allocate(1), D(a, 1));
#else
        typedef std::allocator<T> _Alloc;
        _Alloc a;
        std::unique_ptr<T> hold(a.allocate(1));
#endif
        T* ret = ::new ((void *)hold.get()) T(std::forward<Args>(args)...);
        dst.s.ptr = hold.release();
        dst.h = &LargeHandler::handle;
        return *ret;
    }

private:
    XLLVM_ALWAYS_INLINE
    static void destroy(Any &this_any){
        delete static_cast<T*>(this_any.s.ptr);
        this_any.h = nullptr;
    }

    XLLVM_ALWAYS_INLINE
    static void copy(Any const &this_any, Any &dst) {
        LargeHandler::create(dst, *static_cast<T const *>(this_any.s.ptr));
    }

    XLLVM_ALWAYS_INLINE
    static void move(Any &this_any, Any &dst) {
        dst.s.ptr = this_any.s.ptr;
        dst.h = &LargeHandler::handle;
        this_any.h = nullptr;
    }

    XLLVM_ALWAYS_INLINE
    static void *get(Any &this_any, std::type_info const *info, const void *fallback_id) {
        if (AnyImp::compare_typeid<T>(info, fallback_id)) {
            return static_cast<void *>(this_any.s.ptr);
        }
        return nullptr;
    }

    XLLVM_ALWAYS_INLINE
    static void *get_type_info() {
#if XLLVM_USE(TYPEINFO)
        return const_cast<void *>(static_cast<void const *>(&typeid(T)));
#else
        return nullptr;
#endif
    }
};

} // namespace AnyImp

template <class V, class T, class>
Any::Any(V && v) : h(nullptr) {
    AnyImp::Handler<T>::create(*this, std::forward<V>(v));
}

template <class V, class ...Args, class T, class>
Any::Any(std::in_place_type_t<V>, Args&&... args) {
    AnyImp::Handler<T>::create(*this, std::forward<Args>(args)...);
};

template <class V, class U, class ...Args, class T, class>
Any::Any(std::in_place_type_t<V>, std::initializer_list<U> il, Args&&... args) {
    AnyImp::Handler<T>::create(*this, il, std::forward<Args>(args)...);
}

template <class V, class, class>
XLLVM_ALWAYS_INLINE
Any & Any::operator=(V && v) {
    Any(std::forward<V>(v)).swap(*this);
    return *this;
}

template <class V, class ...Args, class T, class>
XLLVM_ALWAYS_INLINE
T& Any::emplace(Args&&... args) {
    reset();
    return AnyImp::Handler<T>::create(*this, std::forward<Args>(args)...);
}

template <class V, class U, class ...Args, class T, class>
XLLVM_ALWAYS_INLINE
T& Any::emplace(std::initializer_list<U> il, Args&&... args) {
    reset();
    return AnyImp::Handler<T>::create(*this, il, std::forward<Args>(args)...);
}

XLLVM_ALWAYS_INLINE
void Any::swap(Any & rhs) noexcept {
    if (this == &rhs) {
        return;
    }
    if (h && rhs.h) {
        Any tmp;
        rhs.call(Action::Move, &tmp);
        this->call(Action::Move, &rhs);
        tmp.call(Action::Move, this);
    }
    else if (h) {
        this->call(Action::Move, &rhs);
    }
    else if (rhs.h) {
        rhs.call(Action::Move, this);
    }
}

// 6.4 Non-member functions

XLLVM_ALWAYS_INLINE
void swap(Any &lhs, Any &rhs) noexcept {
    lhs.swap(rhs);
}

template <class T, class ...Args>
XLLVM_ALWAYS_INLINE
Any make_any(Args&&... args) {
    return Any(std::in_place_type<T>, std::forward<Args>(args)...);
}

template <class T, class U, class ...Args>
XLLVM_ALWAYS_INLINE
Any make_any(std::initializer_list<U> il, Args&&... args) {
    return Any(std::in_place_type<T>, il, std::forward<Args>(args)...);
}

template <class V>
XLLVM_ALWAYS_INLINE
V any_cast(Any const & v)
{
    using T = std::remove_cv_t<std::remove_reference_t<V>>;
    static_assert(std::is_constructible<V, T const &>::value,
                  "V is required to be a const lvalue reference "
                  "or a CopyConstructible type");
    auto tmp = any_cast<std::add_const_t<T>>(&v);
    if (tmp == nullptr) {
        throw_bad_any_cast();
    }
    return static_cast<V>(*tmp);
}

template <class V>
XLLVM_ALWAYS_INLINE
V any_cast(Any & v)
{
    using T = std::remove_cv_t<std::remove_reference_t<V>>;
    static_assert(std::is_constructible<V, T &>::value,
                  "V is required to be an lvalue reference "
                  "or a CopyConstructible type");
    auto tmp = any_cast<T>(&v);
    if (tmp == nullptr) {
        throw_bad_any_cast();
    }
    return static_cast<V>(*tmp);
}

template <class V>
XLLVM_ALWAYS_INLINE
V any_cast(Any && v)
{
    using T = std::remove_cv_t<std::remove_reference_t<V>>;
    static_assert(std::is_constructible<V, T>::value,
                  "V is required to be an rvalue reference "
                  "or a CopyConstructible type");
    auto tmp = any_cast<T>(&v);
    if (tmp == nullptr) {
        throw_bad_any_cast();
    }
    return static_cast<V>(std::move(*tmp));
}

template <class V>
XLLVM_ALWAYS_INLINE
std::add_pointer_t<std::add_const_t<V>>
any_cast(Any const *any) noexcept
{
    static_assert(!std::is_reference<V>::value, "V may not be a reference.");
    return any_cast<V>(const_cast<Any *>(any));
}

template <class R>
XLLVM_ALWAYS_INLINE
R pointer_or_func_cast(void *ptr, /*IsFunction*/std::false_type) noexcept {
    return static_cast<R>(ptr);
}

template <class R>
XLLVM_ALWAYS_INLINE
R pointer_or_func_cast(void *, /*IsFunction*/std::true_type) noexcept {
    return nullptr;
}

template <class V>
std::add_pointer_t<V>
any_cast(Any *any) noexcept
{
    using AnyImp::Action;
    static_assert(!std::is_reference<V>::value, "V may not be a reference.");
    typedef typename std::add_pointer<V>::type R;
    if (any && any->h) {
        void *ptr = any->call(Action::Get, nullptr,
#if XLLVM_USE(TYPEINFO)
        &typeid(V),
#else
        nullptr,
#endif
        AnyImp::get_fallback_typeid<V>());
        return pointer_or_func_cast<R>(ptr, std::is_function<V>{});
    }
    return nullptr;
}

}  // namespace XLLVM

#endif // XLLVM_ANY
