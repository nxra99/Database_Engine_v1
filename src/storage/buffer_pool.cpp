#include "buffer_pool.h"

buffer_pool::buffer_pool(disk_manager* disk)
{
  this->disk = disk;  
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
  ASSIGN_OR_RETURN(u8 idx, get_frame_idx(page_id));
  
  if (!frames[idx].is_live) return Status::not_supported("frame is not live");
  
  if (frames[idx].is_dirty)
  {
    u8* page_buffer = frames[idx]._page.get_buffer();
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
      u8* page_buffer = frames[idx]._page.get_buffer();
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


void buffer_pool::null_frame(FIDX frame_idx)
{
  frame& current = frames[frame_idx];
  current.page_id = 0xffffffff;
  current.is_live = false;
  current.pin_count = 0;
  current.ref_bit = false;
  current.is_dirty = false;
  // current._page = page{};
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
  u32 page_id = current._page.get_page_id();
  
  if (current.is_dirty) RETURN_IF_ERROR(flush_page(page_id));
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
  pool->unpin_frame(page_id);  
}

page* page_guard::get()
{
  return pg;
}

