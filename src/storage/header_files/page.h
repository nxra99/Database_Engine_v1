#pragma once
#include "disk_manager.h"

enum page_type : u8
{
  SLOTTED_PAGE,
  INDEX_PAGE
};

struct page_header
{
  u32 page_id;
  u32 next_page_id;
  page_type type;
  u16 slot_count;
  u16 free_upper;
  u16 free_lower;
  u16 free_space;
};


class page
{
public:
  page() = default;
  
  u8* get_buffer();
  
  page_header get_header();  
  void write_header(page_header& header);

  u32 get_page_id();
  void write_page_id(u32 page_id);
  u32 get_next_page_id();
  void write_next_page_id(u32 page_id);
  u8 get_page_type();
  void write_page_type(page_type type);
  u16 get_slot_count();
  void write_slot_count(u16 value);
  u16 get_free_upper();
  void write_free_upper(u16 value);
  u16 get_free_lower();
  void write_free_lower(u16 value);
  u16 get_free_space();
  void write_free_space(u16 value);
  

private:
  std::array<u8, PAGE_SIZE> buffer{};
  page_header header;
  
};