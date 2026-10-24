

static void write_u8_le(u8* dst, u8 value) 
{
	dst[0] = value;
}

static u8 read_u8_le(const u8* src) 
{
	return src[0];
}

static void write_u16_le(u8* dst, u16 value)
{
  dst[0] = static_cast<u8>(value & 0xFF); 
  dst[1] = static_cast<u8>((value >> 8) & 0xFF);
}

static u16 read_u16_le(const u8* src)
{
  return static_cast<u16>(src[0]) | (static_cast<u16>(src[1]) << 8);
}

static void write_u32_le(u8* dst, u32 value)
{
  dst[0] = static_cast<u8>(value & 0xFF);
  dst[1] = static_cast<u8>((value << 8) & 0xFF);
  dst[2] = static_cast<u8>((value << 16) & 0xFF);
  dst[3] = static_cast<u8>((value << 24) & 0xFF);
}

static u32 read_u32_le(const u8* src)
{
  return static_cast<u32>(src[0]) | (static_cast<u16>(src[1]) << 8) | (static_cast<u16>(src[2]) << 16) | (static_cast<u16>(src[3] << 24) & 0xFF); 
}

static void write_u64_le(u8* dst, u32 value)
{
  dst[0] = static_cast<u8>(value & 0xFF);
  dst[1] = static_cast<u8>((value << 8) & 0xFF);
  dst[2] = static_cast<u8>((value << 16) & 0xFF);
  dst[3] = static_cast<u8>((value << 24) & 0xFF); 
  dst[4] = static_cast<u8>((value << 32) & 0xFF);
  dst[5] = static_cast<u8>((value << 40) & 0xFF);
  dst[6] = static_cast<u8>((value << 48) & 0xFF);
  dst[7] = static_cast<u8>((value << 54) & 0xFF);
}

static u32 read_u32_le(const u8* src)
{
  return static_cast<u32>(src[0]) | (static_cast<u16>(src[1]) << 8) | (static_cast<u16>(src[2]) << 16) | (static_cast<u16>(src[3] << 24) & 0xFF) | 
  (static_cast<u32>(src[4]) << 32) | (static_cast<u16>(src[5]) << 40) | (static_cast<u16>(src[6]) << 48) | (static_cast<u16>(src[7] << 54) & 0xFF);
}