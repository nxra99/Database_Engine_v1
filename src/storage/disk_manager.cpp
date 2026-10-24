#include "disk_manager.h"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <system_error>

StatusOr<std::unique_ptr<disk_manager>> disk_manager::open_path(std::string path)
{
  // create_directories throws on failure; the error_code version reports it instead
  std::error_code ec;
  std::filesystem::path parent = std::filesystem::path(path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent, ec);
  if (ec) return Status::IO_error("create folder " + parent.string(), ec.message());

  int file_fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
  if (file_fd < 0) return Status::IO_error("open " + path, std::strerror(errno));

  // make_unique can't call a private constructor, so use new directly.
  // from here on, an early return closes file_fd through the destructor
  std::unique_ptr<disk_manager> disk(new disk_manager(std::move(path), file_fd));

  off_t size = lseek(file_fd, 0, SEEK_END);
  if (size < 0) return Status::IO_error("lseek " + disk->file_path, std::strerror(errno));

  if (size == 0)
  {
    disk->total_page_count = 1;
    RETURN_IF_ERROR(disk->write_header());
  }
  else
  {
    // read_header() sets total_page_count itself; we only need to pass on its error
    RETURN_IF_ERROR(disk->read_header().status());
  }

  return disk;
}

disk_manager::disk_manager(std::string path, int file_fd) : file_path(std::move(path)), fd(file_fd) {}

disk_manager::~disk_manager()
{
  fdatasync(fd);
  close(fd);
}

ssize_t disk_manager::write_page(u8* page_buffer, u32 page_id)
{
  off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
  ssize_t write = pwrite(fd, page_buffer, PAGE_SIZE, offset);

  return write;
}

ssize_t disk_manager::read_page(u8* page_buffer, u32 page_id)
{
  off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
  ssize_t read = pread(fd, page_buffer, PAGE_SIZE, offset);

  return read;
}

StatusOr<u32> disk_manager::allocate()
{
  u32 page_id = total_page_count;

  if (ftruncate(fd, static_cast<off_t>(page_id + 1) * PAGE_SIZE) < 0) return Status::corruption("ftruncate " + file_path);

  total_page_count = page_id + 1;
  RETURN_IF_ERROR(write_header()); 

  return page_id;
}

Status disk_manager::write_header()
{
  write_u32_le(header_page.data() + 0, total_page_count); //write page_count
  if (write_page(header_page.data(), 0) != PAGE_SIZE) return Status::IO_error("write header " + file_path);

  return Status::OK();
}

StatusOr<u32> disk_manager::read_header()
{
  if (read_page(header_page.data(), 0) != PAGE_SIZE) return Status::not_found("header page is missing or short in " + file_path);

  total_page_count = read_u32_le(header_page.data() + 0);
  if (total_page_count == 0) return Status::not_found("corrupt header (page count 0) in " + file_path);

  return total_page_count;
}

