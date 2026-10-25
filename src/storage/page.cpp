#include "page.h"

page::page() = default; 

page::page(page_header& header) //explicit
{
  write_header(header);
}

const u8* page::get_buffer() const
{
  return buffer.data();
}

void page::load_from(const u8* src)
{
  memcpy(buffer.data(), src, PAGE_SIZE);
}

page_header page::get_header() const
{
  return {get_page_id(), get_next_page_id(), static_cast<page_type>(get_page_type()), 
  get_slot_count(), get_free_upper(), get_free_lower(), get_free_space()};
}

void page::write_header(page_header& header)
{
  write_page_id(header.page_id);
  write_next_page_id(header.next_page_id);
  write_page_type(header.type);
  write_slot_count(header.slot_count);
  write_free_upper(header.free_upper);
  write_free_lower(header.free_lower);
  write_free_space(header.free_space);
}

void page::reset_header()
{
  write_page_id(0xffffffff);   
  write_next_page_id(0xffffffff);
  write_page_type(page_type::NONE);
  write_slot_count(0);
  write_free_upper(4096);
  write_free_lower(24);
  write_free_space(4096-24);
}


u32 page::get_page_id() const
{
  return read_u32_le(buffer.data() + 0);
}

void page::write_page_id(u32 page_id)
{
  write_u32_le(buffer.data() + 0, page_id);
}

u32 page::get_next_page_id() const
{
  return read_u32_le(buffer.data() + 4);
}

void page::write_next_page_id(u32 value)
{
  write_u32_le(buffer.data() + 4, value);
}

u8 page::get_page_type() const
{
  return read_u8_le(buffer.data() + 8);
}

void page::write_page_type(page_type type)
{
  write_u8_le(buffer.data() + 8, static_cast<u8>(type));
}

u16 page::get_slot_count() const
{
  return read_u16_le(buffer.data() + 10);
}

void page::write_slot_count(u16 value) 
{
  write_u16_le(buffer.data() + 10, value);
}

u16 page::get_free_upper() const
{
  return read_u16_le(buffer.data() + 12);
}

void page::write_free_upper(u16 value)
{
  write_u16_le(buffer.data() + 12, value);
}

u16 page::get_free_lower() const
{
  return read_u16_le(buffer.data() + 14);
}

void page::write_free_lower(u16 value)
{
  write_u16_le(buffer.data() + 14, value);
}

u16 page::get_free_space() const
{
  return read_u16_le(buffer.data() + 16); 
}

void page::write_free_space(u16 value)
{
  write_u16_le(buffer.data() + 16, value);
}

u8* page::mutate_buffer()
{
  return buffer.data();
}
