#include "buffer_pool.h"

buffer_pool::buffer_pool(disk_manager* disk)
{
  this->disk = disk;  
}

StatusOr<FIDX> buffer_pool::new_page()
{
  //FYI:: first test, frames are currently free
  FIDX idx = UINT8_MAX;
  
  StatusOr<FIDX> lookup = get_free_frame();
  if (lookup.ok())
  {
    idx = *lookup;
    null_frame(idx);
  }
  else
  {
    ASSIGN_OR_RETURN(idx, evict());
  }

  ASSIGN_OR_RETURN(u32 page_id, disk->allocate());

  frame& frme = frames[idx];
  frme.is_live = true;

  page_header header = {page_id, 0xffffffff, page_type::NONE, 0, PAGE_SIZE, HEADER_SIZE, PAGE_SIZE - HEADER_SIZE};
  page pg(header);
  
  
  frme.is_dirty = false;
  frme.page_id = page_id;
  frme.pg = pg;

  map[page_id] = idx;
  
  return idx;
}

StatusOr<page_guard> buffer_pool::fetch_page(u32 page_id)
{
 StatusOr<FIDX> lookup = get_frame_idx(page_id);
 if (lookup.ok())
 {
    FIDX idx = *lookup;
    frames[idx].pin_count++;
    frames[idx].ref_bit = true;
    
    page_guard pguard (&frames[idx].pg, page_id, this, idx);
    return pguard;  
  }
  
  FIDX idx = UINT8_MAX;
  StatusOr<FIDX> free_idx = get_free_frame();
  if (free_idx.ok())
  {
    idx = *free_idx;
    null_frame(idx);
  }
  else
  {
    ASSIGN_OR_RETURN(idx, evict());
  }

  std::array<u8, PAGE_SIZE> page_buffer{}; 
  ssize_t bytes = disk->read_page(page_buffer.data(), page_id);
  if (bytes != PAGE_SIZE) return Status::not_found("page does not exist in disk");
  
  frame& _frame = frames[idx];
  _frame.is_dirty = true;
  _frame.is_live = true;
  _frame.page_id = page_id;
  _frame.pin_count += 1;
  _frame.ref_bit = true;
  _frame.pg.load_from(page_buffer.data());  //page implementation to frame
  
  map[page_id] = idx;
  
  page_guard pguard (&_frame.pg, page_id, this, idx);
  return pguard; 
}

StatusOr<FIDX> buffer_pool::get_frame_idx(u32 page_id)
{
  auto it = map.find(page_id);
  if (it != map.end())
  {
    return it->second;
  }

  return Status::not_found("page is not in frames, please call fetch page");
}

Status buffer_pool::mark_dirty(u32 page_id, bool dirty)
{
  ASSIGN_OR_RETURN(u8 idx, get_frame_idx(page_id));
  frames[idx].is_dirty = dirty;

  return Status::OK();
}

Status buffer_pool::pin_frame(u32 page_id)
{
  ASSIGN_OR_RETURN(u8 idx, get_frame_idx(page_id));
  frames[idx].pin_count++;

  return Status::OK();
}

Status buffer_pool::unpin_frame(u32 page_id)
{
  ASSIGN_OR_RETURN(u8 idx, get_frame_idx(page_id));
  if (frames[idx].pin_count == 0) return Status::IO_error("cannot unpin a page that has pin_count of 0");
  
  frames[idx].pin_count--;
  return Status::OK();
}

Status buffer_pool::flush_page(u32 page_id)
{
  ASSIGN_OR_RETURN(FIDX idx, get_frame_idx(page_id));
  
  if (!frames[idx].is_live) return Status::not_supported("frame is not live");
  
  if (frames[idx].is_dirty)
  {
    const u8* page_buffer = frames[idx].pg.get_buffer();
    ssize_t bytes_written = disk->write_page(page_buffer, page_id);
    if (bytes_written != PAGE_SIZE) return Status::IO_error("Must only write to disk 4096 bytes at a time");
    frames[idx].is_dirty = false;
    return Status::OK();
  }


  //flush page does not mean evict the page. no need to null frame here  
  return Status::not_supported("frame is not dirty");
}



Status buffer_pool::flush_dirty_pages()
{
  for (size_t idx = 0; idx < frames.size(); idx++)
  {
    if (!frames[idx].is_live) return Status::not_supported("frame is not live", "idx: " + idx);
    
    if (frames[idx].is_dirty)
    {
      const u8* page_buffer = frames[idx].pg.get_buffer();
      ssize_t bytes_written = disk->write_page(page_buffer, frames[idx].page_id);
      if (bytes_written != PAGE_SIZE) return Status::IO_error("Must only write to disk 4096 bytes at a time");
      frames[idx].is_dirty = false;
    }
  }
  return Status::OK();
}

bool buffer_pool::is_live(FIDX frame_id)
{
  return frames[frame_id].is_live;
}

StatusOr<FIDX> buffer_pool::get_free_frame()
{
  for (FIDX i = 0; i < FRAME_SIZE; i++)
  {
    if (!frames[i].is_live) 
    {
      return i; 
    }
  }
  return Status::not_found("no frames are available, please evict");
}


void buffer_pool::null_frame(FIDX frame_idx)
{
  frame& current = frames[frame_idx];
  current.page_id = 0xffffffff;
  current.is_live = false;
  current.pin_count = 0;
  current.ref_bit = false;
  current.is_dirty = false;
  // current.pg = page{};
}

StatusOr<FIDX> buffer_pool::victim()
{ 
  size_t i = 0;
  do
  {
    frame& current = frames[hand];
    if (!current.ref_bit && current.pin_count == 0)
    {
      return hand;
    }
    current.ref_bit = false;
    advance_hand();
    i++;
  } while ((3 * frames.size()) > i);
  
  return Status::not_found("victim not found");
}

StatusOr<FIDX> buffer_pool::evict()
{
  u8 live_frames = get_live_frames();
  if (live_frames < frames.size()) return Status::invalid_argument("free frames are available");
  
  ASSIGN_OR_RETURN(FIDX idx, victim());
  frame& current = frames[idx];
  // current.is_dirty = true;
  u32 page_id = current.page_id;
  if (current.is_dirty) 
  {
    RETURN_IF_ERROR(flush_page(page_id)); 
  }
  null_frame(idx);
  map.erase(page_id);
  
  return idx;
}

void buffer_pool::advance_hand()
{
  (hand == frames.size() - 1) ? hand = 0 : hand++;
}


u8 buffer_pool::get_live_frames()
{
  size_t i = 0, live_frames = 0;
  while (i < frames.size())
  {
    if (frames[i].is_live != false) live_frames++;
    i++;  
  }

  return live_frames;
}

page_guard::~page_guard()
{
  if (pool != nullptr && pool->frames[frame_idx].pin_count > 0)
    pool->frames[frame_idx].pin_count--;
}

page* page_guard::get()
{
  return pg;
}

page_guard::page_guard(page_guard&& other) noexcept
{
  this->pg = std::move(other.pg);
  this->page_id = other.page_id;
  this->pool = other.pool;
  this->frame_idx = other.frame_idx;
  
  other.pg = nullptr;
  other.pool = nullptr;
  other.page_id = 0xffffffff;
  other.frame_idx = 0xff;
}