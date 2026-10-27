#pragma once
#include "buffer_pool.h"

struct RID
{
  
  u16 slot_id;
  PGID page_id;
};

class heap_file
{
public:
//create method to replace a constructor.

static StatusOr<std::unique_ptr<heap_file>> create_heap_file(buffer_pool* pool);

~heap_file();

  StatusOr<RID> insert(record& rec);
  StatusOr<record> get(RID& rid);
  Status remove(RID& rid); //change live to false.
  Status update(RID& rid, record& rec);
  Status add_page();

  PGID get_first_pg_id();
  PGID get_last_pg_id();

private:

  heap_file(PGID first_pg_id, buffer_pool* pool);
  buffer_pool* pool;
  PGID first_pg_id{0xffffffff};
  PGID last_pg_id{0xffffffff};

};

