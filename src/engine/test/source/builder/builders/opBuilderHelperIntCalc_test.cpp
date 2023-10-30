
#include <any>
#include <gtest/gtest.h>
#include <vector>

#include <baseTypes.hpp>
#include <defs/mocks/failDef.hpp>

#include "opBuilderHelperMap.hpp"

using namespace base;
namespace bld = builder::internals::builders;

// INT64_MAX = 
constexpr auto almostMaxNum = INT64_MAX - 1;
// INT64_MIN =-
constexpr auto almostMinNum = INT64_MIN + 1;

TEST(opBuilderHelperIntCalc, Builds)
{
    auto tuple = std::make_tuple(std::string {"/field"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    ASSERT_NO_THROW(std::apply(bld::opBuilderHelperIntCalc, tuple));
}

TEST(opBuilderHelperIntCalc, Builds_error_bad_operator)
{
    auto tuple = std::make_tuple(std::string {"/field"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"test", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    ASSERT_THROW(std::apply(bld::opBuilderHelperIntCalc, tuple), std::runtime_error);
}

TEST(opBuilderHelperIntCalc, Builds_error_zero_division)
{
    auto tuple = std::make_tuple(std::string {"/field"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "0"},
                                 std::make_shared<defs::mocks::FailDef>());

    ASSERT_THROW(std::apply(bld::opBuilderHelperIntCalc, tuple), std::runtime_error);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_field_not_exist)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"fieldcheck": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_sum)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(20, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_sub)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sub", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(0, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_mul)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(100, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_div)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(1, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_ref_field_not_exist)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield2": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_ref_sum)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(20, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_ref_sub)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sub", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(0, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_ref_mul)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(100, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_ref_division_by_zero)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield2": 0})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_ref_div)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(1, result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_field_not_exist)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 15,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "fieldcheck": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_sum)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 15,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(20, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_sub)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sub", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 15,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(0, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_mul)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 15,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(100, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_div)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "10"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 15,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(1, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_ref_field_not_exist)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "$otherfield"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 10,
                                                   "otherfield2": 10})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_ref_sum)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "$parentObjt_2.field2check"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 10,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(20, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_ref_sub)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sub", "$parentObjt_2.field2check"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 10,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(0, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_ref_mul)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "$parentObjt_2.field2check"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 10,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");
    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(100, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_ref_division_by_zero)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "$parentObjt_2.field2check"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 0,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_ref_div)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "$parentObjt_2.field2check"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "field2check": 10,
                        "ref_key": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ(1, result.payload()->getInt64("/parentObjt_1/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_sum_multiple_parameters)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10", "20", "30"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 1})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ((1 + 10 + 20 + 30), result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_sub_multiple_parameters)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sub", "10", "20", "30"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 1})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ((1 - 10 - 20 - 30), result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_mul_multiple_parameters)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "10", "20", "30"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 1})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ((1 * 10 * 20 * 30), result.payload()->getInt64("/field2check").value());
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_div_multiple_parameters)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "10", "20", "30"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({"field2check": 1})");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ((1 / 10 / 20 / 30), result.payload()->getInt64("/field2check").value());
}

// Division by zero by value and reference with multiple arguments
TEST(opBuilderHelperIntCalc, Exec_int_calc_div_by_zero_multiple_parameters)
{
    // by value
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"div", "10", "0", "30"},
                                 std::make_shared<defs::mocks::FailDef>());

    ASSERT_THROW(std::apply(bld::opBuilderHelperIntCalc, tuple), std::runtime_error);

    // by reference
    tuple = std::make_tuple(std::string {"/field2check"},
                            std::string {"int_calculate"},
                            std::vector<std::string> {"div", "$Object.A", "$Object.B", "$Object.C"},
                            std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                        "field2check": 10,
                        "Object":
                        {
                            "A": 10,
                            "B": 11,
                            "C": 0
                        }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    auto result = op(event1);

    ASSERT_FALSE(result);

    // both
    tuple = std::make_tuple(std::string {"/field2check"},
                            std::string {"int_calculate"},
                            std::vector<std::string> {"div", "$Object.A", "0", "$Object.C"},
                            std::make_shared<defs::mocks::FailDef>());

    auto event2 = std::make_shared<json::Json>(R"({
                    "field2check": 10,
                    "Object": {
                        "A": 10,
                        "B": 11,
                        "C": 0
                    }
                    })");

    ASSERT_THROW(std::apply(bld::opBuilderHelperIntCalc, tuple), std::runtime_error);
}

// Division by zero by reference with multiple arguments and multilevel fields
TEST(opBuilderHelperIntCalc, Exec_int_calc_multilevel_division_by_zero_several_params)
{
    auto tuple = std::make_tuple(
        std::string {"/parentObjt_1/field2check"},
        std::string {"int_calculate"},
        std::vector<std::string> {"div", "$parentObjt_2.firstReference", "$parentObjt_2.seccondReference"},
        std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_2": {
                        "seccondReference": 0,
                        "firstReference": 10
                    },
                    "parentObjt_1": {
                        "field2check": 10,
                        "ref_key": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

// Succesfully using both values and references
TEST(opBuilderHelperIntCalc, Exec_int_calc_sum_multiple_parameters_values_and_references)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "10", "$Object.A", "30"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "field2check": 10,
                    "Object": {
                        "A": 10,
                        "B": 11,
                        "C": 0
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_TRUE(result);

    ASSERT_EQ((10 + 10 + 10 + 30), result.payload()->getInt64("/field2check").value());
}

// Failing on several non existing references
TEST(opBuilderHelperIntCalc, Exec_int_calc_mul_several_non_existing_references)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "$Object.C", "$Object.Z"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt": {
                        "field2check": 15
                    },
                    "Object": {
                        "A": 10,
                        "B": 11
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

// Failing on several wrong type references
TEST(opBuilderHelperIntCalc, Exec_int_calc_mul_several_different_types_references)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "$Object.A", "$Object.B", "$Object.C", "$Object.D"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt": {
                        "field2check": 1
                    },
                    "Object": {
                        "A": null,
                        "B": "string",
                        "C": { "field":"value"},
                        "D": ["fieldA","fieldB"]
                    }
                    })");

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
}

// Failing on several empty values
TEST(opBuilderHelperIntCalc, Exec_int_calc_several_empty_params)
{
    auto tuple = std::make_tuple(std::string {"/parentObjt_1/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "10", "", ""},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 = std::make_shared<json::Json>(R"({
                    "parentObjt_1": {
                        "field2check": 15
                    }})");

    ASSERT_THROW(std::apply(bld::opBuilderHelperIntCalc, tuple), std::runtime_error);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_sum_value_error)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sum", "2"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 =
        std::make_shared<json::Json>(fmt::format(R"({{"field2check": {}}})", std::to_string(almostMaxNum)).c_str());

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);
    // TODO: does it make any sense to check the whole message or should the message be
    // shorter? same for all the next results
    ASSERT_EQ(result.trace(),
              "[helper.int_calculate[/field2check, sum, 2]] -> Failure: operation result in "
              "integer Overflown");

    auto tuple2 = std::make_tuple(std::string {"/field2check"},
                                  std::string {"int_calculate"},
                                  std::vector<std::string> {"sum", "-3"},
                                  std::make_shared<defs::mocks::FailDef>());

    auto event2 =
        std::make_shared<json::Json>(fmt::format(R"({{"field2check": {}}})", std::to_string(-almostMaxNum)).c_str());

    auto op2 = std::apply(bld::opBuilderHelperIntCalc, tuple2)->getPtr<Term<EngineOp>>()->getFn();

    result = op2(event2);

    ASSERT_FALSE(result);
    ASSERT_EQ(result.trace(),
              "[helper.int_calculate[/field2check, sum, -3]] -> Failure: operation result in "
              "integer Underflown");
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_sub_value_error)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"sub", "2"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 =
        std::make_shared<json::Json>(fmt::format(R"({{"field2check": {}}})", std::to_string(almostMinNum)).c_str());

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);

    auto tuple2 = std::make_tuple(std::string {"/field2check"},
                                  std::string {"int_calculate"},
                                  std::vector<std::string> {"sub", "-2"},
                                  std::make_shared<defs::mocks::FailDef>());

    auto event2 =
        std::make_shared<json::Json>(fmt::format(R"({{"field2check": {}}})", std::to_string(almostMaxNum)).c_str());

    auto op2 = std::apply(bld::opBuilderHelperIntCalc, tuple2)->getPtr<Term<EngineOp>>()->getFn();

    result = op2(event2);

    ASSERT_FALSE(result);
}

TEST(opBuilderHelperIntCalc, Exec_int_calc_mul_value_error)
{
    auto tuple = std::make_tuple(std::string {"/field2check"},
                                 std::string {"int_calculate"},
                                 std::vector<std::string> {"mul", "2"},
                                 std::make_shared<defs::mocks::FailDef>());

    auto event1 =
        std::make_shared<json::Json>(fmt::format(R"({{"field2check": {}}})", std::to_string(almostMaxNum)).c_str());

    auto op = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result::Result<Event> result = op(event1);

    ASSERT_FALSE(result);

    auto event2 =
        std::make_shared<json::Json>(fmt::format(R"({{"field2check": {}}})", std::to_string(almostMinNum)).c_str());

    auto op2 = std::apply(bld::opBuilderHelperIntCalc, tuple)->getPtr<Term<EngineOp>>()->getFn();

    result = op2(event2);

    ASSERT_FALSE(result);
}
