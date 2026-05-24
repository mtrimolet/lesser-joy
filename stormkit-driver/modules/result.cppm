module;

#include <string.h>

export module result;

import meta;
import utilities;
import allocation;

namespace lj::details {
    enum class Status : char {
        EMPTY,
        VALUE,
        ERROR,
    };
} // namespace lj::details

export namespace lj {
    template<typename E>
    struct Unexpected {
        E error;
    };

    template<typename T, typename E>
    class result;

    template<typename T, typename E>
    struct result_monadic_operations {
        template<typename Self, typename Func>
        constexpr auto and_then(this Self&& self, Func&& then) -> decltype(then(forward<Self>(self))) {
            if (not self.has_value()) return forward<Self>(self);

            return then(forward<Self>(self));
        }

        template<typename Self, typename Func>
        constexpr auto map(this Self&& self, Func&& then) -> result<decltype(then(self.value())), E> {
            using To = result<decltype(then(self.value())), E>;
            if (not self.has_value()) return To { forward<Self>(self).error() };

            if constexpr (meta::IsAnyOf<Self, result<T, E>&&, const result<T, E>&&>) {
                self.m_status = details::Status::EMPTY;
                return then(move(self.value()));
            } else {
                return then(self.value());
            }
        }

        template<typename Self, typename Func>
        constexpr auto or_else(this Self&& self, Func&& then) -> decltype(then(forward<Self>(self))) {
            if (not self.has_error()) return forward<Self>(self);

            return then(forward<Self>(self));
        }

        template<typename Self, typename Func>
        constexpr auto map_error(this Self&& self, Func&& then) -> result<T, decltype(then(self.error()))> {
            if (not self.has_error()) return forward<Self>(self);

            auto& value = *self.error_ptr();
            if constexpr (meta::IsAnyOf<Self, result<T, E>&&, const result<T, E>&&>) {
                self.m_status = details::Status::EMPTY;
                return Unexpected{ then(move(value)) };
            } else {
                return Unexpected{ then(value) };
            }
        }
    };

    template<typename T, typename E>
    class result: public result_monadic_operations<T, E> {
        static constexpr auto BYTE_COUNT = [] static {
            if constexpr (sizeof(T) > sizeof(E)) return sizeof(T);
            else
                return sizeof(E);
        }();
        using AlignedType = meta::If<(sizeof(T) > sizeof(E)), T, E>;

      public:
        constexpr result() = default;

        constexpr ~result() { destroy(); }

        constexpr result(result&& other)
            requires(meta::IsTriviallyCopyable<T> or meta::IsMovable<T> and meta::IsTriviallyCopyable<E> or meta::IsMovable<E>)
        {
            if (other.m_status == details::Status::EMPTY) destroy();

            if constexpr (meta::IsTriviallyCopyable<T> and meta::IsTriviallyCopyable<E>) {
                memcpy(m_data, other.m_data, BYTE_COUNT);
            } else {
                if (other.has_value()) {
                    if constexpr (meta::IsTriviallyCopyable<T>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) T { move(other.value()) };
                } else {
                    if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) E { move(other.error()) };
                }
            }
            m_status = exchange(other.m_status, details::Status::EMPTY);
        }

        constexpr result(const result& other)
            requires(meta::IsCopyable<T> and meta::IsCopyable<E>)
        {
            if (other.m_status == details::Status::EMPTY) destroy();

            if constexpr (meta::IsTriviallyCopyable<T> and meta::IsTriviallyCopyable<E>) {
                memcpy(m_data, other.m_data, BYTE_COUNT);
            } else {
                if (other.has_value()) {
                    if constexpr (meta::IsTriviallyCopyable<T>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) T { other.value() };
                } else {
                    if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) E { other.error() };
                }
            }
            m_status = other.m_status;
        }

        constexpr auto operator=(result&& other) -> result&
            requires(meta::IsTriviallyCopyable<T> or meta::IsMovable<T> and meta::IsTriviallyCopyable<E> or meta::IsMovable<E>)
        {
            if (&other == this) return *this;

            destroy();

            if (other.m_status == details::Status::EMPTY) return *this;

            if constexpr (meta::IsTriviallyCopyable<T> and meta::IsTriviallyCopyable<E>) {
                memcpy(m_data, other.m_data, BYTE_COUNT);
            } else {
                if (other.has_value()) {
                    if constexpr (meta::IsTriviallyCopyable<T>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) T { move(other.value()) };
                } else {
                    if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) E { move(other.error()) };
                }
            }
            m_status = exchange(other.m_status, details::Status::EMPTY);
            return *this;
        }

        constexpr auto operator=(const result& other) -> result&
            requires(meta::IsCopyable<T> and meta::IsCopyable<E> or meta::IsMovable<T> and meta::IsMovable<E>)
        {
            if (&other == this) return *this;

            destroy();

            if (other.m_status == details::Status::EMPTY) return *this;

            if constexpr (meta::IsTriviallyCopyable<T> and meta::IsTriviallyCopyable<E>) {
                memcpy(m_data, other.m_data, BYTE_COUNT);
            } else {
                if (other.has_value()) {
                    if constexpr (meta::IsTriviallyCopyable<T>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) T { other.value() };
                } else {
                    if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, BYTE_COUNT);
                    else
                        new (m_data) E { other.error() };
                }
            }
            m_status = exchange(other.m_status, details::Status::EMPTY);
            return *this;
        }

        template<typename U = T>
        constexpr result(U&& value) : m_status { details::Status::VALUE } {
            new (m_data) T { forward<U>(value) };
        }

        template<typename... Args>
        constexpr result(in_place_t, Args&&... args) : m_status { details::Status::VALUE } {
            new (m_data) T { forward<Args>(args)... };
        }

        constexpr result(const Unexpected<E>& value) : m_status { details::Status::ERROR } { new (m_data) E { value.error }; }

        constexpr result(Unexpected<E>&& value) : m_status { details::Status::ERROR } { new (m_data) E { move(value.error) }; }

        constexpr auto operator=(const T& value) -> result& {
            destroy();
            new (m_data) T { value };
            m_status = details::Status::VALUE;
            return *this;
        }

        constexpr auto operator=(T&& value) -> result& {
            destroy();
            new (m_data) T { move(value) };
            m_status = details::Status::VALUE;
            return *this;
        }

        constexpr auto operator=(const Unexpected<E>& value) -> result& {
            destroy();
            new (m_data) T { value };
            m_status = details::Status::ERROR;
            return *this;
        }

        constexpr auto operator=(Unexpected<E>&& value) -> result& {
            destroy();
            new (m_data) T { move(value) };
            m_status = details::Status::ERROR;
            return *this;
        }

        constexpr auto has_value() const -> bool { return m_status == details::Status::VALUE; }

        constexpr auto has_error() const -> bool { return m_status == details::Status::ERROR; }

        constexpr operator bool() const { return has_value(); }

        template<typename Self>
        constexpr auto error(this Self&& self) -> decltype(forward_like<Self>(*self.error_ptr())) {
            return forward_like<Self>(*self.error_ptr());
        }

        template<typename Self>
        constexpr auto value(this Self&& self) -> decltype(forward_like<Self>(*self.value_ptr())) {
            return forward_like<Self>(*self.value_ptr());
        }

        template<typename Self>
        constexpr auto operator*(this Self&& self) -> decltype(forward_like<Self>(*self.value_ptr())) {
            return forward_like<Self>(*self.value_ptr());
        }

      private:
        friend class result_monadic_operations<T, E>;

        constexpr auto value_ptr() -> T* { return launder(bit_cast<T*>(&m_data[0])); }

        constexpr auto error_ptr() -> E* { return launder(bit_cast<E*>(&m_data[0])); }

        constexpr auto destroy() -> void {
            if (has_value())
                value_ptr()->~T();
            else if (has_error())
                error_ptr()->~E();

            m_status = details::Status::EMPTY;
        }

        details::Status m_status = details::Status::EMPTY;
        alignas(AlignedType) byte m_data[BYTE_COUNT];
    };

    template<typename E>
    class result<void, E>: public result_monadic_operations<void, E> {
      public:
        constexpr result() = default;

        constexpr ~result() { destroy(); }

        constexpr result(result&& other)
            requires(meta::IsTriviallyCopyable<E> or meta::IsMovable<E>)
        {
            destroy();

            if (other.has_error()) {
                if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, sizeof(E));
                else
                    new (m_data) E { move(other.error()) };
            }

            m_status = exchange(other.m_status, details::Status::VALUE);
        }

        constexpr result(const result& other)
            requires(meta::IsCopyable<E>)
        {
            destroy();

            if (other.has_error()) {
                if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, sizeof(E));
                else
                    new (m_data) E { other.error() };
            }
            m_status = other.m_status;
        }

        constexpr auto operator=(result&& other) -> result&
            requires(meta::IsTriviallyCopyable<E> or meta::IsMovable<E>)
        {
            if (&other == this) return *this;

            destroy();

            if (other.has_error()) {
                if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, sizeof(E));
                else
                    new (m_data) E { move(other.error()) };
            }
            m_status = exchange(other.m_status, details::Status::VALUE);

            return *this;
        }

        constexpr auto operator=(const result& other) -> result&
            requires(meta::IsCopyable<E>)
        {
            if (&other == this) return *this;

            destroy();

            if (other.has_error()) {
                if constexpr (meta::IsTriviallyCopyable<E>) memcpy(m_data, other.m_data, sizeof(E));
                else
                    new (m_data) E { other.error() };
            }
            m_status = other.m_status;

            return *this;
        }

        constexpr result(const Unexpected<E>& value) : m_status { details::Status::ERROR } { new (m_data) E { value.error }; }

        constexpr result(Unexpected<E>&& value) : m_status { details::Status::ERROR } { new (m_data) E { move(value.error) }; }

        constexpr auto operator=(const Unexpected<E>& value) -> result& {
            destroy();
            new (m_data) E { value.error };
            m_status = details::Status::ERROR;
            return *this;
        }

        constexpr auto operator=(Unexpected<E>&& value) -> result& {
            destroy();
            new (m_data) E { move(value.error) };
            m_status = details::Status::ERROR;
            return *this;
        }

        constexpr auto has_value() const -> bool { return m_status == details::Status::VALUE; }
        constexpr auto has_error() const -> bool { return m_status == details::Status::ERROR; }

        constexpr operator bool() const { return has_value(); }

        template<typename Self>
        constexpr auto error(this Self&& self) -> decltype(forward_like<Self>(*self.error_ptr())) {
            return forward_like<Self>(*self.error_ptr());
        }

        constexpr auto value() const -> void {}

      private:
        friend class result_monadic_operations<void, E>;

        constexpr auto error_ptr() -> E* { return launder(bit_cast<E*>(&m_data[0])); }

        constexpr auto destroy() -> void {
            if (has_error()) error_ptr()->~E();

            m_status = details::Status::VALUE;
        }

        details::Status m_status = details::Status::VALUE;
        alignas(E) byte m_data[sizeof(E)];
    };
} // namespace lj
