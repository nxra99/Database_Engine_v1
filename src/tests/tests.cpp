#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "disk_manager.h"
#include "page.h"
#include "slottedpage.h"
#include "buffer_pool.h"

// Test versions of RETURN_IF_ERROR / ASSIGN_OR_RETURN.
//
// Those two pass an error UP to whoever called the function (`return status;`).
// A TEST_CASE has nobody to return a Status to (it returns void), so these do
// the test equivalent instead: FAIL right here, printing which call failed and why.

// For calls that return a plain Status:
//   REQUIRE_OK(disk->write_header());
#define REQUIRE_OK(expr) \
    do { \
        Status _s = (expr); \
        if (!_s.ok()) FAIL(#expr << " failed: " << _s.to_string()); \
    } while (0)

// For calls that return StatusOr<T>:
//   ASSIGN_OR_REQUIRE(u32 page, disk->allocate());
//     runs disk->allocate() exactly once
//     error   -> test stops on this line, prints "disk->allocate() failed: <message>"
//     success -> u32 page = the value inside the StatusOr
// #expr turns the call's text into a string, so the message says which call failed.
// Same shape as ASSIGN_OR_RETURN in StatusOr.h; only the `return` line is different.
#define ASSIGN_OR_REQUIRE(lhs, expr) \
    ASSIGN_OR_REQUIRE_IMPL(STATUSOR_CONCAT(_status_or_, __LINE__), lhs, expr)

#define ASSIGN_OR_REQUIRE_IMPL(tmp, lhs, expr) \
    auto tmp = (expr); \
    if (!tmp.ok()) FAIL(#expr << " failed: " << tmp.status().to_string()); \
    lhs = std::move(tmp).value()


// fresh file per test, so a run never depends on what the last run left behind
static std::string fresh_db(const std::string& name)
{
    std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    return path.string();
}

// writes raw bytes straight to a file, to fake a damaged database
static void write_raw(const std::string& path, const std::vector<u8>& bytes)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}


TEST_CASE("new file starts with just the header page", "[disk]") {
    std::string path = fresh_db("tdb_new.db");
    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));

    auto result = disk->read_header();
    REQUIRE(result.ok());
    u32 value = *result;
    REQUIRE(value == 1);

    std::array<u8, PAGE_SIZE> page0{};
    REQUIRE(disk->read_page(page0.data(), 0) == PAGE_SIZE);
    REQUIRE(read_u32_le(page0.data()) == 1);
}

TEST_CASE("allocate hands out page ids in order", "[disk]") {
    std::string path = fresh_db("tdb_alloc.db");
    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));

    ASSIGN_OR_REQUIRE(u32 first, disk->allocate());
    ASSIGN_OR_REQUIRE(u32 second, disk->allocate());
    REQUIRE(first == 1);  // page 0 is the header
    REQUIRE(second == 2);

    ASSIGN_OR_REQUIRE(u32 count, disk->read_header());
    REQUIRE(count == 3);
    REQUIRE(std::filesystem::file_size(path) == 3 * PAGE_SIZE);
}

TEST_CASE("page count survives reopening", "[disk]") {
    std::string path = fresh_db("tdb_reopen.db");
    {
    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));
    ASSIGN_OR_REQUIRE(u32 page, disk->allocate());
    REQUIRE(page == 1);
    }

    {
    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));
    ASSIGN_OR_REQUIRE(u32 count, disk->read_header());
    REQUIRE(count == 2);  // header page + page 1

    std::array<u8, PAGE_SIZE> page0{};
    REQUIRE(disk->read_page(page0.data(), 0) == PAGE_SIZE);
    REQUIRE(read_u32_le(page0.data()) == 2);

    ASSIGN_OR_REQUIRE(u32 page, disk->allocate());
    REQUIRE(page == 2);
    }

    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk_, disk_manager::open_path(path));
    ASSIGN_OR_REQUIRE(u32 count, disk_->read_header());
    REQUIRE(count == 3);
    REQUIRE(std::filesystem::file_size(path) == 3 * PAGE_SIZE);
}

TEST_CASE("page data survives reopening", "[disk]") {
    std::string path = fresh_db("tdb_data.db");

    std::array<u8, PAGE_SIZE> written{};
    for (u32 i = 0; i < PAGE_SIZE; i++) written[i] = static_cast<u8>(i * 7);
    u32 page{};

    {
    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));
    ASSIGN_OR_REQUIRE(page, disk->allocate());  // assigning to an existing variable works too
    REQUIRE(disk->write_page(written.data(), page) == PAGE_SIZE);
    }

    ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));
    std::array<u8, PAGE_SIZE> read_back{};
    REQUIRE(disk->read_page(read_back.data(), page) == PAGE_SIZE);
    REQUIRE((read_back == written));  // extra brackets: on failure Catch2 prints "false" instead of all 4096 bytes
}


// --- error paths: a broken file should come back as a non-OK Status, not crash ---

TEST_CASE("a file too short to hold a header is reported", "[disk][errors]") {
    std::string path = fresh_db("tdb_short.db");
    write_raw(path, std::vector<u8>(10, 0xAB));

    // here we WANT an error, so keep the StatusOr and look inside it.
    // open_path reads the header, so a broken file is reported right here
    StatusOr<std::unique_ptr<disk_manager>> result = disk_manager::open_path(path);
    INFO("status was: " << result.status().to_string());  // printed only if a check below fails
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().is_not_found());
}

TEST_CASE("a header saying 0 pages is rejected", "[disk][errors]") {
    std::string path = fresh_db("tdb_zero.db");
    write_raw(path, std::vector<u8>(PAGE_SIZE, 0));

    StatusOr<std::unique_ptr<disk_manager>> result = disk_manager::open_path(path);
    INFO("status was: " << result.status().to_string());
    REQUIRE_FALSE(result.ok());
}

TEST_CASE("page error rec", "[Page]")
{
	page_header header = {0, 0xFFFF, page_type::SLOTTED_PAGE, 0, PAGE_SIZE, HEADER_SIZE, PAGE_SIZE - HEADER_SIZE};
	std::unique_ptr<page> pg = std::make_unique<page>(header);
	{
	slotted_page spg(pg.get());
	record rec;
	std::array<u8, 50> record_buffer{};
	rec = {record_buffer.data(),  50};
	REQUIRE_OK(spg.add_record(rec));	
	}

	REQUIRE(pg->get_free_lower() == HEADER_SIZE + SLOT_SIZE);
	REQUIRE(pg->get_slot_count() == 1);
}

TEST_CASE("buffer pool new page", "[buffer_pool]")
{
	// setup: an empty database file, and a disk_manager that owns it
	std::string path = fresh_db("tdb_buffer_pool.db");
	ASSIGN_OR_REQUIRE(std::unique_ptr<disk_manager> disk, disk_manager::open_path(path));

	// disk.get() is the plain disk_manager* your buffer_pool constructor takes
	buffer_pool pool(disk.get());
	
	for (int i = 0; i < 10; i++)
	{
		ASSIGN_OR_REQUIRE(FIDX idx, pool.new_page());
	}

	REQUIRE(pool.is_live(9));

	// ASSIGN_OR_REQUIRE(FIDX idx, pool->evict());
	// REQUIRE(pool->is_live(1));

	ASSIGN_OR_REQUIRE(page_guard pguard, pool.fetch_page(4));
	page* pg = pguard.get();
	REQUIRE(pg->get_page_id() == 4);
	
}	


