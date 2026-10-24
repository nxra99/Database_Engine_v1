#include "disk_manager.h"

#include <cerrno>
#include <filesystem>
#include <stdexcept>
#include <system_error>


disk_manager::disk_manager(std::string path) : file_path(std::move(path))
{
  std::filesystem::path parent = std::filesystem::path(file_path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);

  fd = open(file_path.c_str(), O_RDWR | O_CREAT, 0644);
  assert(fd >= 0);
  
  off_t size = lseek(fd, 0, SEEK_END);
  if (size == 0)
  {
    total_page_count = 1;  
    write_header();
  }
  else
  {
    read_header();  
  }
}

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

