#pragma once

#include "serialisation.h"
#include "StatusOr.h"



class disk_manager
{
public:
  // use this instead of a constructor: a constructor can't return a Status, this can
  static StatusOr<std::unique_ptr<disk_manager>> open_path(std::string path = "data/data.db");
  ~disk_manager();

  // owns fd, so a copy would close the same file twice
  disk_manager(const disk_manager&) = delete;
  disk_manager& operator=(const disk_manager&) = delete;

  Status write_header(); //writes total_page_count into page 0
  ssize_t write_page(const u8* page_buffer, u32 page_id);
  ssize_t read_page(u8* page_buffer, u32 page_id); //buffer here is an output parameter
  StatusOr<u32> allocate(); //returns the id of the new page
  StatusOr<u32> read_header(); //just reads current page count.

private:
  disk_manager(std::string path, int file_fd); //only open_path() can create one
  
  const std::string file_path;
  int fd = -1;
  u32 total_page_count = 0; //includes the header page, so a fresh file has 1
  std::array<u8, PAGE_SIZE> header_page{}; //in-memory copy of page 0
};

