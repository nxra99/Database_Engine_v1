#include "slottedpage.h"

slotted_page::slotted_page (page* pg)
{
  this->pg = pg;
  // pg->write_page_type(page_type::SLOTTED_PAGE);
}

StatusOr<u16> slotted_page::add_record (record& rec)
{
  u16 free_space = pg->get_free_space();
  if (!(free_space >= rec.record_length + SLOT_SIZE))
  {
    return Status::corruption("insufficient space to store record on page");
  }

  u8* buffer = pg->mutate_buffer();
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
  write_u8_le(buffer + free_lower + 8, 0);
  write_u8_le(buffer + free_lower + 9, 0);
  write_u8_le(buffer + free_lower + 10, 0);
 

  //update page metadata
  pg->write_free_lower(free_lower + SLOT_SIZE);
  pg->write_free_upper(offset);
  pg->write_free_space(offset - free_lower);
  pg->write_slot_count(slot_id);
  pg->write_page_type(page_type::SLOTTED_PAGE);
  
  return slot_id;

  // assumption here: page_id and next_page_id have already been implemented
}

StatusOr<const record> slotted_page::get_record (u16 slot_id)
{
  u8* buffer = pg->mutate_buffer();
  u16 slot_count = pg->get_slot_count();
  
  if (slot_id > slot_count)
  {
    return Status::not_found("slot_id does not exist");
  }

  ASSIGN_OR_RETURN(slot _slot, get_slot(slot_id));
  
  std::vector<u8> record_buffer (_slot.record_length, 0);
  memcpy(record_buffer.data(), buffer + _slot.offset, _slot.record_length);

  return record(record_buffer.data(), _slot.record_length);
}

Status slotted_page::delete_record(u16 slot_id)
{
  //strictly make slot not alive
  u8* buffer = pg->mutate_buffer();
  
  ASSIGN_OR_RETURN(slot _slot, get_slot(slot_id));
  memset(buffer + _slot.offset, 0, _slot.record_length);
  _slot.live = false;
  write_u16_le(buffer + _slot.offset + 6, _slot.live);

  return Status::OK();
}

StatusOr<slot> slotted_page::get_slot(u16 slot_id)
{
  u16 offset = HEADER_SIZE + (slot_id - 1) * SLOT_SIZE;
  
  slot s;

  const u8* buffer = pg->get_buffer();
  s.slot_id = read_u16_le(buffer + offset);
  if (s.slot_id != slot_id) return Status::not_found("slot_id does not match");
  s.offset = read_u16_le(buffer + offset + 2);
  s.record_length = read_u16_le(buffer + offset + 4);
  s.live = static_cast<u8>(read_u16_le(buffer + offset + 6));
  if (s.live == false) return Status::not_supported("slot is not live");
  
  return s;
}

Status slotted_page::update_record(u16 slot_id, record& rec)
{
  ASSIGN_OR_RETURN(slot s, get_slot(slot_id));
  if (s.record_length < rec.record_length) return Status::corruption("cannot update. updated record length must be <= to old record length", "please delete previous slot_id and add new record");
  
  u8* buffer = pg->mutate_buffer();
  memset(buffer + s.offset, 0, s.record_length);
  memcpy(buffer + s.offset, rec.buffer, rec.record_length);
  
  s.record_length = rec.record_length;
  
  return Status::OK();
}

