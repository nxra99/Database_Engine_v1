#pragma once
#include "disk_manager.h"
#include "serialisation.h"

enum page_type : u8
{
  SLOTTED_PAGE,
  INDEX_PAGE,
  NONE
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
  page();
  explicit page(page_header& header);

  const u8* get_buffer() const;
  void load_from(const u8* src);
  
  
  page_header get_header() const;  
  void reset_header();
  void write_header(page_header& header);

  u32 get_page_id() const;
  void write_page_id(u32 page_id);
  u32 get_next_page_id() const;
  void write_next_page_id(u32 page_id);
  u8 get_page_type() const;
  void write_page_type(page_type type);
  u16 get_slot_count() const;
  void write_slot_count(u16 value);
  u16 get_free_upper() const;
  void write_free_upper(u16 value);
  u16 get_free_lower() const;
  void write_free_lower(u16 value);
  u16 get_free_space() const;
  void write_free_space(u16 value);

  
private:
  std::array<u8, PAGE_SIZE> buffer{};
  u8* mutate_buffer();
  friend class slotted_page;
};


