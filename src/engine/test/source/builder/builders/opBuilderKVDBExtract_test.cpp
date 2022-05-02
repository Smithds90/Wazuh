/* Copyright (C) 2015-2022, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <vector>

#include <gtest/gtest.h>

#include "testUtils.hpp"
#include <kvdb/kvdbManager.hpp>
#include <opBuilderKVDB.hpp>

namespace
{
using namespace base;
namespace bld = builder::internals::builders;

using FakeTrFn = std::function<void(std::string)>;
static FakeTrFn tr = [](std::string msg){};

class opBuilderKVDBExtractTest : public ::testing::Test
{

protected:
    KVDBManager& kvdbManager = KVDBManager::get();

    virtual void SetUp()
    {
        kvdbManager.addDb("TEST_DB");
    }

    virtual void TearDown()
    {
        kvdbManager.deleteDB("TEST_DB");
    }
};

// Build ok
TEST_F(opBuilderKVDBExtractTest, Builds)
{
    Document doc {R"({
        "map":
            {"field2extract": "+kvdb_extract/TEST_DB/ref_key"}
    })"};
    ASSERT_NO_THROW(bld::opBuilderKVDBExtract(doc.get("/map"), tr));
}

// Build incorrect number of arguments
TEST_F(opBuilderKVDBExtractTest, Builds_incorrect_number_of_arguments)
{
    Document doc {R"({
        "check":
            {"field2match": "+kvdb_extract/TEST_DB"}
    })"};
    ASSERT_THROW(bld::opBuilderKVDBExtract(doc.get("/check"), tr),
                 std::runtime_error);
}

// Build invalid DB
TEST_F(opBuilderKVDBExtractTest, Builds_incorrect_invalid_db)
{
    Document doc {R"({
        "check":
            {"field2match": "+kvdb_extract/INVALID_DB/ref_key"}
    })"};
    ASSERT_THROW(bld::opBuilderKVDBExtract(doc.get("/check"), tr),
                 std::runtime_error);
}

// Static key
TEST_F(opBuilderKVDBExtractTest, Static_key)
{
    auto kvdb = kvdbManager.getDB("TEST_DB");
    kvdb->write("KEY", "VALUE");

    Document doc {R"({
        "map":
            {"field2extract": "+kvdb_extract/TEST_DB/KEY"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createSharedEvent(R"(
                {"dummy_field": "qwe"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"dummy_field": "ASD123asd"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"dummy_field": "ASD"}
            )"));
            s.on_completed();
        });

    Lifter lift = bld::opBuilderKVDBExtract(doc.get("/map"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/field2extract").GetString(), "VALUE");
    ASSERT_STREQ(expected[1]->getEvent()->get("/field2extract").GetString(), "VALUE");
    ASSERT_STREQ(expected[2]->getEvent()->get("/field2extract").GetString(), "VALUE");
}

// Dynamic key
TEST_F(opBuilderKVDBExtractTest, Dynamic)
{
    auto kvdb = kvdbManager.getDB("TEST_DB");
    kvdb->write("KEY", "VALUE");

    Document doc {R"({
        "map":
            {"field2extract": "+kvdb_extract/TEST_DB/$key"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createSharedEvent(R"(
                {"key": "KEY"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"inexistent_key": "KEY"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"invalid_string": 123}
            )"));
            s.on_next(createSharedEvent(R"(
                {"invalid_key": "INVALID_KEY"}
            )"));
            s.on_completed();
        });

    Lifter lift = bld::opBuilderKVDBExtract(doc.get("/map"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 4);
    ASSERT_STREQ(expected[0]->getEvent()->get("/field2extract").GetString(), "VALUE");
    ASSERT_FALSE(expected[1]->getEvent()->exists("/field2extract"));
    ASSERT_FALSE(expected[2]->getEvent()->exists("/field2extract"));
    ASSERT_FALSE(expected[3]->getEvent()->exists("/field2extract"));
}

// Multi level key
TEST_F(opBuilderKVDBExtractTest, Multi_level_key)
{
    auto kvdb = kvdbManager.getDB("TEST_DB");
    kvdb->write("KEY", "VALUE");

    Document doc {R"({
        "map":
            {"field2extract": "+kvdb_extract/TEST_DB/$a.b.key"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createSharedEvent(R"(
                 {"a":{"b":{"key":"KEY"}}}
            )"));
            s.on_next(createSharedEvent(R"(
                 {"a":{"b":{"inexistent_key":"KEY"}}}
            )"));
            s.on_next(createSharedEvent(R"(
                 {"a":{"b":{"invalid_key":"INVALID_KEY"}}}
            )"));
            s.on_completed();
        });

    Lifter lift = bld::opBuilderKVDBExtract(doc.get("/map"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/field2extract").GetString(), "VALUE");
    ASSERT_FALSE(expected[1]->getEvent()->exists("/field2extract"));
    ASSERT_FALSE(expected[2]->getEvent()->exists("/field2extract"));
}

// Multi level target
TEST_F(opBuilderKVDBExtractTest, Multi_level_target)
{
    auto kvdb = kvdbManager.getDB("TEST_DB");
    kvdb->write("KEY", "VALUE");

    Document doc {R"({
        "map":
            {"a.b.field2extract": "+kvdb_extract/TEST_DB/KEY"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createSharedEvent(R"(
                {"not_fieldToCreate": "qwe"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"not_fieldToCreate": "ASD123asd"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"not_fieldToCreate": "ASD"}
            )"));
            s.on_completed();
        });

    Lifter lift = bld::opBuilderKVDBExtract(doc.get("/map"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/a/b/field2extract").GetString(), "VALUE");
    ASSERT_STREQ(expected[1]->getEvent()->get("/a/b/field2extract").GetString(), "VALUE");
    ASSERT_STREQ(expected[2]->getEvent()->get("/a/b/field2extract").GetString(), "VALUE");
}

// Existent target
TEST_F(opBuilderKVDBExtractTest, Existent_target)
{
    auto kvdb = kvdbManager.getDB("TEST_DB");
    kvdb->write("KEY", "VALUE");

    Document doc {R"({
        "map":
            {"field2extract": "+kvdb_extract/TEST_DB/KEY"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createSharedEvent(R"(
                {"dummy_data": "dummy_value"}
            )"));
            s.on_next(createSharedEvent(R"(
                {"field2extract": "PRE_VALUE"}
            )"));
            s.on_completed();
        });

    Lifter lift = bld::opBuilderKVDBExtract(doc.get("/map"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 2);
    ASSERT_STREQ(expected[0]->getEvent()->get("/field2extract").GetString(), "VALUE");
    ASSERT_STREQ(expected[0]->getEvent()->get("/field2extract").GetString(), "VALUE");
}
} // namespace
