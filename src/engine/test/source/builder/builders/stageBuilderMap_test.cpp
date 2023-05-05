#include <gtest/gtest.h>

#include <memory>

#include "baseTypes.hpp"
#include "builder/builders/operationBuilder.hpp"
#include "builder/builders/stageBuilderMap.hpp"
#include "builder/registry.hpp"
#include <json/json.hpp>

#include <defs/mocks/failDef.hpp>

using namespace builder::internals;
using namespace builder::internals::builders;
using namespace json;
using namespace base;

#define GTEST_COUT std::cout << "[          ] [ INFO ] "

TEST(StageBuilderMapTest, Builds)
{
    auto registry = std::make_shared<Registry<builder::internals::Builder>>();
    auto helperRegistry = std::make_shared<Registry<builder::internals::HelperBuilder>>();
    registry->registerBuilder(getOperationMapBuilder(helperRegistry), "operation.map");
    auto mapJson = Json {R"([
        {"string": "value"},
        {"int": 1},
        {"double": 1.0},
        {"boolT": true},
        {"boolF": false},
        {"null": null},
        {"array": [1, 2, 3]},
        {"object": {"a": 1, "b": 2}}
])"};

    ASSERT_NO_THROW(getStageMapBuilder(registry)(mapJson, std::make_shared<defs::mocks::FailDef>()));
}

TEST(StageBuilderMapTest, UnexpectedDefinition)
{
    auto registry = std::make_shared<Registry<builder::internals::Builder>>();
    auto helperRegistry = std::make_shared<Registry<builder::internals::HelperBuilder>>();
    registry->registerBuilder(getOperationMapBuilder(helperRegistry), "operation.map");
    auto mapJson = Json {R"({})"};

    ASSERT_THROW(getStageMapBuilder(registry)(mapJson, std::make_shared<defs::mocks::FailDef>()), std::runtime_error);
}

TEST(StageBuilderMapTest, BuildsCorrectExpression)
{
    auto registry = std::make_shared<Registry<builder::internals::Builder>>();
    auto helperRegistry = std::make_shared<Registry<builder::internals::HelperBuilder>>();
    registry->registerBuilder(getOperationMapBuilder(helperRegistry), "operation.map");
    auto mapJson = Json {R"([
        {"string": "value"},
        {"int": 1},
        {"double": 1.0},
        {"boolT": true},
        {"boolF": false},
        {"null": null},
        {"array": [1, 2, 3]},
        {"object": {"a": 1, "b": 2}}
])"};

    auto expression = getStageMapBuilder(registry)(mapJson, std::make_shared<defs::mocks::FailDef>());

    ASSERT_TRUE(expression->isOperation());
    ASSERT_TRUE(expression->isChain());
    for (auto term : expression->getPtr<Chain>()->getOperands())
    {
        ASSERT_TRUE(term->isTerm() || term->isChain());
    }
}
