#pragma once
#include "page.h"

struct slot
{
  slot(){}
  slot(u16 sid, u16 os, u16 rl, bool l) : slot_id(sid), offset(os), record_length(rl), live(l) {}
  u16 slot_id; 
  u16 offset;
  u16 record_length;
  bool live;
  u8 reserved[3] = {0, 0, 0};
};

struct record
{
  u8* buffer;
  u16 record_length;
};

class slotted_page
{
public:
  
  slotted_page(page* pg);
  
  StatusOr<u16> add_record(record& rec); //returns slot_id
  StatusOr<const record> get_record(u16 slot_id);
  Status delete_record(u16 slot_id);
  Status update_record(u16 slot_id, record& rec);
  
  
private:
  page* pg;
  StatusOr<slot> get_slot(u16 slot_id);
};

