#pragma once

#include <cstdint>
#include <string>
#include <string_view>

enum class Code : uint8_t
{
  Ok = 0,
  not_found = 1,
  corruption = 2,
  not_supported = 3,
  invalid_argument = 4,
  IO_Error = 5
};

class [[nodiscard]] Status
{
public:
  Status() noexcept : state_(nullptr) {}
  ~Status() { delete[] state_; }

  Status(const Status& other);
  Status& operator= (const Status& other);

  Status(Status&& other) noexcept;
  Status& operator= (Status&& other) noexcept;

  std::string to_string() const;

  static Status OK();
  static Status not_found(std::string_view msg, std::string_view msg2 = std::string_view());
  static Status corruption(std::string_view msg, std::string_view msg2 = std::string_view());
  static Status not_supported(std::string_view msg, std::string_view msg2 = std::string_view());
  static Status invalid_argument(std::string_view msg, std::string_view msg2 = std::string_view());
  static Status IO_error(std::string_view msg, std::string_view msg2 = std::string_view());

  Code code() const;
  bool ok() const;
  bool is_not_found() const;
  bool is_corruption() const;
  bool is_not_supported() const;
  bool is_invalid_argument() const;
  bool is_IO_error() const;
  
private:
  Status(Code code, std::string_view msg1, std::string_view msg2);
  static const char* copy_state(const char* state);

  // nullptr means OK. Otherwise:
  //   state_[0..3] == length of message
  //   state_[4]    == code
  //   state_[5..]  == message
  const char* state_;
};

#define RETURN_IF_ERROR(expr) \
    do { \
        Status _s = (expr); \
        if (!_s.ok()) return _s; \
    } while(0)

