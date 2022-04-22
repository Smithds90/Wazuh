/* Copyright (C) 2015-2022, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <gtest/gtest.h>
#include "testUtils.hpp"
#include <vector>

#include "opBuilderHelperMap.hpp"

using namespace builder::internals::builders;

using FakeTrFn = std::function<void(std::string)>;
static FakeTrFn tr = [](std::string msg){};

auto createEvent = [](const char * json){
    return std::make_shared<Base::EventHandler>(std::make_shared<json::Document>(json));
};

// Build ok
TEST(opBuilderHelperStringTrim, Builds)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/both/t"}
    })"};
    ASSERT_NO_THROW(opBuilderHelperStringTrim(doc.get("/normalize"), tr));
}

// Build incorrect number of arguments
TEST(opBuilderHelperStringTrim, Builds_incorrect_number_of_arguments)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/both/t/t"}
    })"};
    ASSERT_THROW(opBuilderHelperStringTrim(doc.get("/normalize"), tr), std::runtime_error);
}

// Test ok: both trim
TEST(opBuilderHelperStringTrim, BothOk)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/both/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"fieldToTranf": "---hi---"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "hi---"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "---hi"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "hi"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 4);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieldToTranf").GetString(), "hi");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieldToTranf").GetString(), "hi");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieldToTranf").GetString(), "hi");
}

TEST(opBuilderHelperStringTrim, Start_ok)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/begin/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"fieldToTranf": "---hi---"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "hi---"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "---hi"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "hi"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 4);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieldToTranf").GetString(), "hi---");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieldToTranf").GetString(), "hi---");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieldToTranf").GetString(), "hi");
    ASSERT_STREQ(expected[3]->getEvent()->get("/fieldToTranf").GetString(), "hi");
}

// Test ok: dynamic values (string)
TEST(opBuilderHelperStringTrim, End_ok)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/end/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"fieldToTranf": "---hi---"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "hi---"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "---hi"}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": "hi"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 4);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieldToTranf").GetString(), "---hi");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieldToTranf").GetString(), "hi");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieldToTranf").GetString(), "---hi");
    ASSERT_STREQ(expected[3]->getEvent()->get("/fieldToTranf").GetString(), "hi");
}

TEST(opBuilderHelperStringTrim, Multilevel_src)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf.a.b": "+s_trim/end/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "---hi---"}}}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "hi---"}}}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "---hi"}}}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "hi"}}}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 4);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieldToTranf/a/b").GetString(), "---hi");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieldToTranf/a/b").GetString(), "hi");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieldToTranf/a/b").GetString(), "---hi");
    ASSERT_STREQ(expected[3]->getEvent()->get("/fieldToTranf/a/b").GetString(), "hi");
}

TEST(opBuilderHelperStringTrim, Not_exist_src)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/end/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"not_ext": "---hi---"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 1);
    ASSERT_FALSE(expected[0]->getEvent()->exists("/fieldToTranf"));
}

TEST(opBuilderHelperStringTrim, Src_not_string)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf": "+s_trim/end/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"fieldToTranf": 15}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 1);
    ASSERT_TRUE(expected[0]->getEvent()->exists("/fieldToTranf"));
    ASSERT_EQ(expected[0]->getEvent()->get("/fieldToTranf").GetInt(), 15);
}

TEST(opBuilderHelperStringTrim, Multilevel)
{
    Document doc{R"({
        "normalize":
            {"fieldToTranf.a.b": "+s_trim/end/-"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "---hi---"}}}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "hi---"}}}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "---hi"}}}
            )"));
            s.on_next(createEvent(R"(
                {"fieldToTranf": {"a": {"b": "hi"}}}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringTrim(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 4);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieldToTranf/a/b").GetString(), "---hi");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieldToTranf/a/b").GetString(), "hi");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieldToTranf/a/b").GetString(), "---hi");
    ASSERT_STREQ(expected[3]->getEvent()->get("/fieldToTranf/a/b").GetString(), "hi");
}
