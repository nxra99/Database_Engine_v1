#pragma once

#include "typdefs.h"
#include "StatusOr.h"


class disk_manager
{
public:
  explicit disk_manager(std::string path = "data/data.db");
  ~disk_manager();

  // owns fd, so a copy would close the same file twice
  disk_manager(const disk_manager&) = delete;
  disk_manager& operator=(const disk_manager&) = delete;

  Status write_header(); //writes total_page_count into page 0
  ssize_t write_page(u8* page_buffer, u32 page_id);
  ssize_t read_page(u8* page_buffer, u32 page_id); //buffer here is an output parameter
  StatusOr<u32> allocate(); //returns the id of the new page
  StatusOr<u32> read_header(); //just reads current page count.

private:
  const std::string file_path;
  int fd = -1;
  u32 total_page_count = 0; //includes the header page, so a fresh file has 1
  std::array<u8, PAGE_SIZE> header_page{}; //in-memory copy of page 0
};
