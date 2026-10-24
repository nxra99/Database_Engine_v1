#include "Status.h"

#include <cstring>
#include <utility>

Status::Status(Code code, std::string_view msg1, std::string_view msg2)
{
  uint32_t len1 = static_cast<uint32_t>(msg1.size());
  uint32_t len2 = static_cast<uint32_t>(msg2.size());
  uint32_t size = len1 + (len2 ? (2 + len2) : 0);
  char* result = new char[size + 5];
  std::memcpy(result + 0, &size, sizeof(size));
  result[4] = static_cast<char>(code);
  std::memcpy(result + 5, msg1.data(), len1);
  if (len2)
  {
    result[5 + len1] = ':';
    result[6 + len1] = ' ';
    std::memcpy(result + 7 + len1, msg2.data(), len2);
  }
  state_ = result;
}

const char* Status::copy_state(const char* state)
{
  uint32_t size;
  std::memcpy(&size, state, sizeof(size));
  char* result = new char[size + 5];
  std::memcpy(result, state, size + 5);
  return result;
}

Status::Status(const Status& other)
{
  state_ = other.state_ == nullptr ? nullptr : copy_state(other.state_);
}

Status& Status::operator=(const Status& other)
{
  if (state_ != other.state_)
  {
    delete[] state_;
    state_ = other.state_ == nullptr ? nullptr : copy_state(other.state_);
  }

  return *this;
}

Status::Status(Status&& other) noexcept
{
  state_ = other.state_;
  other.state_ = nullptr;
}

Status& Status::operator= (Status&& other) noexcept
{
  // other's destructor frees our old state.
  std::swap(state_, other.state_);
  return *this;
}

std::string Status::to_string() const
{
  if (state_ == nullptr)
  {
    return "OK";
  }

  const char* type;
  switch (code())
  {
    case Code::Ok:               type = "OK"; break;
    case Code::not_found:        type = "Not found: "; break;
    case Code::corruption:       type = "Corruption: "; break;
    case Code::not_supported:    type = "Not supported: "; break;
    case Code::invalid_argument: type = "Invalid argument: "; break;
    case Code::IO_Error:         type = "IO error: "; break;
    default:                     type = "Unknown code: "; break;
  }

  uint32_t size;
  std::memcpy(&size, state_, sizeof(size));
  std::string result(type);
  result.append(state_ + 5, size);
  return result;
}

Status Status::OK()
{
  return Status();
}
Status Status::not_found(std::string_view msg, std::string_view msg2)
{
  return Status(Code::not_found, msg, msg2);
}
Status Status::corruption(std::string_view msg, std::string_view msg2)
{
  return Status(Code::corruption, msg, msg2);
}
Status Status::not_supported(std::string_view msg, std::string_view msg2)
{
  return Status(Code::not_supported, msg, msg2);
}
Status Status::invalid_argument(std::string_view msg, std::string_view msg2)
{
  return Status(Code::invalid_argument, msg, msg2);
}
Status Status::IO_error(std::string_view msg, std::string_view msg2)
{
  return Status(Code::IO_Error, msg, msg2);
}

Code Status::code() const
{
  return (state_ == nullptr) ? Code::Ok : static_cast<Code>(state_[4]);
}
bool Status::ok() const
{
  return code() == Code::Ok;
}
bool Status::is_not_found() const
{
  return code() == Code::not_found;
}
bool Status::is_corruption() const
{
  return code() == Code::corruption;
}
bool Status::is_not_supported() const
{
  return code() == Code::not_supported;
}
bool Status::is_invalid_argument() const
{
  return code() == Code::invalid_argument;
}
bool Status::is_IO_error() const
{
  return code() == Code::IO_Error;
}
