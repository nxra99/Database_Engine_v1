#include "heap_file.h"

heap_file::~heap_file() = default;

StatusOr<std::unique_ptr<heap_file>> heap_file::create_heap_file(buffer_pool* pool)
{
  ASSIGN_OR_RETURN(PGID first_pg_id, pool->new_page());
  
  std::unique_ptr<heap_file> hf (new heap_file(first_pg_id, pool));
  hf->last_pg_id = first_pg_id;

  return hf;
}

heap_file::heap_file(PGID first_pg_id, buffer_pool* pool)
{
  this->pool = pool;
  this->first_pg_id = first_pg_id;
}

Status heap_file::add_page()
{
  ASSIGN_OR_RETURN(PGID page_id, pool->new_page());
  last_pg_id = page_id;
  
  return Status::OK();
}

Status heap_file::insert(record& rec)
{
  //Strictly append to last page for now. If no space, run add_page() and append there.
  //TODO:: in v2, must implement a free space map here to add records to slots that are not live.
  ASSIGN_OR_RETURN(page_guard pguard, pool->fetch_page(last_pg_id));
  page* pg = pguard.get();
  u16 free_space = pg->get_free_space();
  if (free_space > rec.record_length)
  {
    slotted_page spg (pg);
    spg.add_record(rec); 
    return Status::OK();
  }

  RETURN_IF_ERROR(add_page());
  ASSIGN_OR_RETURN(page_guard _pguard, pool->fetch_page(last_pg_id));  
  page* _pg = _pguard.get();
  slotted_page _spg(_pg);
  _spg.add_record(rec);
  return Status::OK();
}










PGID heap_file::get_first_pg_id()
{
  return first_pg_id;
}

