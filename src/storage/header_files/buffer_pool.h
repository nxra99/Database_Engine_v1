
#pragma once
//frame, bufferpool
//TODO:: rebuild logical architecture here...
#include "disk_manager.h"
#include "slottedpage.h"

constexpr u8 FRAME_SIZE = 10;

//frame: 
struct frame
{
  page pg; //page has a default constructor
  u32 page_id = 0xffffffff;
  u32 pin_count = 0;
  bool ref_bit = 0;
  bool is_dirty = false;
  bool is_live = false;
};

class page_guard;

class buffer_pool
{
public:
  buffer_pool(disk_manager* disk);
  StatusOr<u32> new_page();
  StatusOr<page_guard> fetch_page(u32 page_id);
  StatusOr<FIDX> get_frame_idx(u32 page_id); //lookup
  bool is_live(FIDX frame_id);
  
  //clock replacement
  StatusOr<FIDX> victim(); 
  StatusOr<FIDX> evict();
  StatusOr<FIDX> get_free_frame();
  
  //frame/page modifications:
  Status mark_dirty(u32 page_id, bool dirty);
  Status pin_frame(u32 page_id);
  Status unpin_frame(u32 page_id);
  Status flush_page(u32 page_id);
  Status flush_dirty_pages();

private:
  std::array<frame, FRAME_SIZE> frames;
  disk_manager* disk;
  std::unordered_map<u32, FIDX> map; //page_id to frame_idx
  
  u8 get_live_frames(); //count live frames
  void null_frame(FIDX frame_idx); //nullify a frame
  
  void advance_hand(); //advance hand  
  FIDX hand = 0; //clock hand
  friend class page_guard;
};

class page_guard
{
public:
  page_guard(page* pg, u32 page_id, buffer_pool* pool, FIDX frame_idx)
  {
    this->pg = pg;
    this->page_id = page_id;
    this->pool = pool;
    this->frame_idx = frame_idx;
  }
  page_guard (page_guard&& other) noexcept;

  ~page_guard(); //auto unpin_page on destruction
  page* get();

  page_guard(const page_guard& OTHER) = delete;
  page_guard& operator= (const page_guard& OTHER) = delete;
  page_guard& operator= (page_guard&& other) noexcept = delete;
private:
  page* pg;
  u32 page_id;
  buffer_pool* pool;
  FIDX frame_idx;
};



