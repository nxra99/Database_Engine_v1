#pragma once

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <type_traits>
#include <utility>

#include "Status.h"
// Holds either a value of type T, or a non-OK Status saying why there is no value.
template <typename T>
class [[nodiscard]] StatusOr
{
  static_assert(!std::is_same_v<T, Status>, "use Status directly, not StatusOr<Status>");

public:
  StatusOr(T value) : status_(Status::OK()), value_(std::move(value)) {}

  StatusOr(Status status) : status_(std::move(status))
  {
    // An OK Status with no value is a programmer bug, not a runtime error.
    if (status_.ok()) die("constructed from an OK Status with no value", "");
  }

  bool ok() const { return status_.ok(); }
  const Status& status() const { return status_; }

  // Calling value() on an error aborts, in release builds too.
  // Check ok() first, or use ASSIGN_OR_RETURN.
  T& value() &             { check(); return *value_; }
  const T& value() const & { check(); return *value_; }
  T&& value() &&           { check(); return std::move(*value_); }

  T& operator*() &             { return value(); }
  const T& operator*() const & { return value(); }
  T* operator->()              { return &value(); }
  const T* operator->() const  { return &value(); }

private:
  void check() const
  {
    if (!ok()) die("value() called on error: ", status_.to_string().c_str());
  }

  [[noreturn]] static void die(const char* msg, const char* detail)
  {
    std::fprintf(stderr, "StatusOr: %s%s\n", msg, detail);
    std::abort();
  }

  Status status_;
  std::optional<T> value_;
};

#define STATUSOR_CONCAT_INNER(a, b) a##b
#define STATUSOR_CONCAT(a, b) STATUSOR_CONCAT_INNER(a, b)

// ASSIGN_OR_RETURN(u32 id, pool.new_page());
// On error, returns the Status from the enclosing function. On success, assigns the value to lhs.
#define ASSIGN_OR_RETURN(lhs, expr) \
    ASSIGN_OR_RETURN_IMPL(STATUSOR_CONCAT(_status_or_, __LINE__), lhs, expr)

#define ASSIGN_OR_RETURN_IMPL(tmp, lhs, expr) \
    auto tmp = (expr); \
    if (!tmp.ok()) return tmp.status(); \
    lhs = std::move(tmp).value()
