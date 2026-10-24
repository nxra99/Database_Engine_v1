#include "slottedpage.h"

slotted_page::slotted_page (page* pg)
{
  this->pg = pg;
}

Status slotted_page::add_record (record& rec)
{
  u16 free_space = pg->get_free_space();
  if (!(free_space >= rec.record_length + SLOT_SIZE))
  {
    return Status::corruption("insufficient space to store record on page");
  }

  u8* buffer = pg->get_buffer();
  u16 free_lower = pg->get_free_lower();
  u16 slot_count = pg->get_slot_count();
  u16 slot_id = slot_count + 1;

  // slot to buffer
  write_u16_le(buffer + free_lower + 0, slot_id); //slot_count increases by one...
  
  //record to buffer
  u16 offset = pg->get_free_upper() - rec.record_length;
  memcpy(buffer + offset, rec.buffer, rec.record_length);
  
  //slot to buffer
  write_u16_le(buffer + free_lower + 2, offset);
  write_u16_le(buffer + free_lower + 4, rec.record_length); 
  write_u16_le(buffer + free_lower + 6, true); 

  //update page metadata
  pg->write_free_lower(free_lower + SLOT_SIZE);
  pg->write_free_upper(offset);
  pg->write_free_space(offset - free_lower);
  pg->write_slot_count(slot_id);
  pg->write_page_type(page_type::SLOTTED_PAGE);

  return Status::OK();

  // assumption here: page_id and next_page_id have already been implemented
}

StatusOr<record> slotted_page::get_record (u16 slot_id)
{
  u8* buffer = pg->get_buffer();
  u16 slot_count = pg->get_slot_count();
  
  if (slot_id > slot_count)
  {
    return Status::not_found("slot_id does not exist");
  }
  
  u16 offset = 24 + (slot_id * SLOT_SIZE);
  
  //read slot
  const u16 record_offset = read_u16_le(buffer + offset + 2);
  const u16 record_length = read_u16_le(buffer + offset + 4);

  std::vector<u8> record_buffer (record_length, 0);
  memcpy(record_buffer.data(), buffer + record_offset, record_length);

  return record(record_buffer.data(), record_length);
}

Status slotted_page::delete_record(u16 slot_id)
{
  //strictly make slot not alive
  u8* buffer = pg->get_buffer();
  u16 slot_count = pg->get_slot_count();
  
  if (slot_id > slot_count)
  {
    return Status::not_found("slot_id does not exist");
  }

  u16 offset = 24 + (slot_id * SLOT_SIZE);
  write_u16_le(buffer + offset + 6, false); 
  
  return Status::OK();
}

