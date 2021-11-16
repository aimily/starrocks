// This file is licensed under the Elastic License 2.0. Copyright 2021 StarRocks Limited.

#include "storage/vectorized/row_source_mask.h"

#include "common/config.h"
#include "gtest/gtest.h"
#include "util/file_utils.h"

namespace starrocks::vectorized {

class RowSourceMaskTest : public testing::Test {
protected:
    void SetUp() override {
        _max_row_source_mask_memory_bytes = config::max_row_source_mask_memory_bytes;

        // create tmp dir
        std::stringstream tmp_dir_s;
        tmp_dir_s << config::storage_root_path << TMP_PREFIX;
        _tmp_dir = tmp_dir_s.str();
        FileUtils::create_dir(_tmp_dir);
    }

    void TearDown() override {
        config::max_row_source_mask_memory_bytes = _max_row_source_mask_memory_bytes;

        // remove tmp dir
        if (!_tmp_dir.empty()) {
            FileUtils::remove(_tmp_dir);
        }
    }

    int64_t _max_row_source_mask_memory_bytes;
    std::string _tmp_dir;
};

// NOLINTNEXTLINE
TEST_F(RowSourceMaskTest, mask) {
    RowSourceMask mask(0);
    ASSERT_EQ(0, mask.get_source_num());
    ASSERT_EQ(false, mask.get_agg_flag());

    mask.data = 0x8000;
    ASSERT_EQ(0, mask.get_source_num());
    ASSERT_EQ(true, mask.get_agg_flag());

    mask.set_source_num(0x7FFF);
    mask.set_agg_flag(false);
    ASSERT_EQ(0x7FFF, mask.data);

    mask.set_source_num(0x7FFF);
    mask.set_agg_flag(true);
    ASSERT_EQ(0xFFFF, mask.data);
}

// NOLINTNEXTLINE
TEST_F(RowSourceMaskTest, memory_masks) {
    RowSourceMaskBuffer buffer(0, config::storage_root_path);
    std::vector<RowSourceMask> source_masks;
    config::max_row_source_mask_memory_bytes = 1024;

    source_masks.emplace_back(RowSourceMask(0, false));
    source_masks.emplace_back(RowSourceMask(1, true));
    source_masks.emplace_back(RowSourceMask(1, false));
    buffer.write(source_masks);
    source_masks.clear();
    source_masks.emplace_back(RowSourceMask(1, true));
    source_masks.emplace_back(RowSourceMask(3, true));
    source_masks.emplace_back(RowSourceMask(2, true));
    buffer.write(source_masks);
    buffer.flush();

    // --- read ---
    buffer.flip();
    auto st = buffer.has_remaining();
    ASSERT_TRUE(st.ok());
    ASSERT_TRUE(st.value());
    RowSourceMask mask = buffer.current();
    ASSERT_EQ(0, mask.get_source_num());
    ASSERT_FALSE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_FALSE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(3, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(2, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    // end
    st = buffer.has_remaining();
    ASSERT_TRUE(st.ok());
    ASSERT_FALSE(st.value());

    // --- read again and check has same source ---
    buffer.flip();
    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(0, mask.get_source_num());
    ASSERT_FALSE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());

    // check has same source
    ASSERT_TRUE(buffer.has_same_source(mask.get_source_num(), 2));
    ASSERT_TRUE(buffer.has_same_source(mask.get_source_num(), 3));
    ASSERT_FALSE(buffer.has_same_source(mask.get_source_num(), 4));
}

// NOLINTNEXTLINE
TEST_F(RowSourceMaskTest, memory_masks_with_persistence) {
    RowSourceMaskBuffer buffer(1, config::storage_root_path);
    std::vector<RowSourceMask> source_masks;
    config::max_row_source_mask_memory_bytes = 1;

    source_masks.emplace_back(RowSourceMask(0, false));
    source_masks.emplace_back(RowSourceMask(1, true));
    source_masks.emplace_back(RowSourceMask(1, false));
    buffer.write(source_masks);
    source_masks.clear();
    source_masks.emplace_back(RowSourceMask(1, true));
    source_masks.emplace_back(RowSourceMask(3, true));
    source_masks.emplace_back(RowSourceMask(2, true));
    buffer.write(source_masks);
    buffer.flush();

    // --- read ---
    buffer.flip();
    auto st = buffer.has_remaining();
    ASSERT_TRUE(st.ok());
    ASSERT_TRUE(st.value());
    RowSourceMask mask = buffer.current();
    ASSERT_EQ(0, mask.get_source_num());
    ASSERT_FALSE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_FALSE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(3, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(2, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());
    buffer.advance();

    // end
    st = buffer.has_remaining();
    ASSERT_TRUE(st.ok());
    ASSERT_FALSE(st.value());

    // --- read again and check has same source ---
    buffer.flip();
    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(0, mask.get_source_num());
    ASSERT_FALSE(mask.get_agg_flag());
    buffer.advance();

    ASSERT_TRUE(buffer.has_remaining().value());
    mask = buffer.current();
    ASSERT_EQ(1, mask.get_source_num());
    ASSERT_TRUE(mask.get_agg_flag());

    // check has same source
    ASSERT_TRUE(buffer.has_same_source(mask.get_source_num(), 2));
    // new masks are not deserialized, so there is only 2 same sources
    ASSERT_FALSE(buffer.has_same_source(mask.get_source_num(), 3));
    ASSERT_FALSE(buffer.has_same_source(mask.get_source_num(), 4));
}

} // namespace starrocks::vectorized