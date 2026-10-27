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

Status heap_file::update(RID& rid, record& rec)
{
  //TODO:: update must move bytes to be able to update.
  //FYI:: For now I will only update if bytes are equal or less than than previous record length.

  ASSIGN_OR_RETURN(page_guard pguard, pool->fetch_page(rid.page_id));
  slotted_page spg(pguard.get());
  
  
  RETURN_IF_ERROR(spg.update_record(rid.slot_id, rec));
  
  return Status::OK();
}

Status heap_file::remove(RID& rid)
{
  ASSIGN_OR_RETURN(page_guard pg, pool->fetch_page(rid.page_id));
  slotted_page spg (pg.get());
  RETURN_IF_ERROR(spg.delete_record(rid.slot_id));

  return Status::OK();
}

StatusOr<RID> heap_file::insert(record& rec)
{
  //Strictly append to last page for now. If no space, run add_page() and append there.
  //TODO:: in v2, must implement a free space map here to add records to slots that are not live.
  ASSIGN_OR_RETURN(page_guard pguard, pool->fetch_page(last_pg_id));
  page* pg = pguard.get();
  u16 free_space = pg->get_free_space();
  if (free_space > rec.record_length)
  {
    slotted_page spg (pg);
    ASSIGN_OR_RETURN(u16 slot_id, spg.add_record(rec));
    RID rid{};
    rid.page_id = last_pg_id;
    rid.slot_id = slot_id;
    RETURN_IF_ERROR(pool->mark_dirty(last_pg_id, true));
    return rid;   
  }

  RETURN_IF_ERROR(add_page());
  ASSIGN_OR_RETURN(page_guard _pguard, pool->fetch_page(last_pg_id));  
  page* _pg = _pguard.get();
  slotted_page _spg(_pg);
  ASSIGN_OR_RETURN(u16 slot_id, _spg.add_record(rec));
  
  RID rid;
  rid.page_id = last_pg_id;
  rid.slot_id = slot_id;
  RETURN_IF_ERROR(pool->mark_dirty(last_pg_id, true));
  
  return rid; 
}



StatusOr<record> heap_file::get (RID& rid)
{ 
  ASSIGN_OR_RETURN(page_guard pguard, pool->fetch_page(rid.page_id));
  slotted_page spg(pguard.get());
  ASSIGN_OR_RETURN(record rec, spg.get_record(rid.slot_id));
  
  return rec;
}

PGID heap_file::get_first_pg_id()
{
  return first_pg_id;
}

PGID heap_file::get_last_pg_id()
{
  return last_pg_id;
}

