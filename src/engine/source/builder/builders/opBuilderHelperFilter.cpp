#include "opBuilderHelperFilter.hpp"

#include <algorithm>
#include <optional>
#include <variant>

#include <re2/re2.h>

#include "baseTypes.hpp"
#include "syntax.hpp"
#include <baseHelper.hpp>
#include <utils/ipUtils.hpp>

namespace builder::internals::builders
{

//*************************************************
//*           Comparison filters                  *
//*************************************************

/**
 * @brief Operators supported by the comparison helpers.
 *
 */
enum class Operator
{
    EQ, ///< equal
    NE, ///< not equal
    GT, ///< greater than
    GE, ///< greater than equal
    LT, ///< less than
    LE, ///< less than equal
    ST, ///< start with
    CN  ///< contains
};

/**
 * @brief Type supported by the comparison helpers.
 *
 */
enum class Type
{
    STRING,
    INT
};

/**
 * @brief Get the Int Cmp Function object
 *
 * @param targetField Reference of the field to compare, obtained from the YAML key
 * @param op Operator to use
 * @param rightParameter Right parameter to compare, obtained from the YAML value
 * @param name Formatted name of the helper
 * @return std::function<base::result::Result<base::Event>(base::Event)>
 *
 * @throws std::runtime_error
 *   - if the right parameter is a value and not a valid integer
 *   - if helper::base::Parameter::Type is not supported
 */
std::function<base::result::Result<base::Event>(base::Event)> getIntCmpFunction(
    const std::string& targetField, Operator op, const helper::base::Parameter& rightParameter, const std::string& name)
{
    // Depending on rValue type we store the reference or the integer value
    std::variant<std::string, int> rValue {};
    auto rValueType {rightParameter.m_type};
    switch (rightParameter.m_type)
    {
        case helper::base::Parameter::Type::VALUE:
            try
            {
                rValue = std::stoi(rightParameter.m_value);
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(fmt::format("\"{}\" function: Parameter \"{}\" "
                                                     "could not be converted to int: {}.",
                                                     name,
                                                     rightParameter.m_value,
                                                     e.what()));
            }

            break;

        case helper::base::Parameter::Type::REFERENCE: rValue = rightParameter.m_value; break;

        default:
            throw std::runtime_error(fmt::format("\"{}\" function: Parameter \"{}\" has an invalid type ({}).",
                                                 name,
                                                 rightParameter.m_value,
                                                 static_cast<int>(rightParameter.m_type)));
    }

    // Depending on the operator we return the correct function
    std::function<bool(int l, int r)> cmpFunction;
    switch (op)
    {
        case Operator::EQ:
            cmpFunction = [](int l, int r)
            {
                return l == r;
            };
            break;
        case Operator::NE:
            cmpFunction = [](int l, int r)
            {
                return l != r;
            };
            break;
        case Operator::GT:
            cmpFunction = [](int l, int r)
            {
                return l > r;
            };
            break;
        case Operator::GE:
            cmpFunction = [](int l, int r)
            {
                return l >= r;
            };
            break;
        case Operator::LT:
            cmpFunction = [](int l, int r)
            {
                return l < r;
            };
            break;
        case Operator::LE:
            cmpFunction = [](int l, int r)
            {
                return l <= r;
            };
            break;
        default: break;
    }

    // Tracing messages
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {
        fmt::format("[{}] -> Failure: Parameter \"{}\" not found", name, rightParameter.m_value)};
    const std::string failureTrace3 {fmt::format("[{}] -> Failure: Comparison is false", name)};

    // Function that implements the helper
    return [=](base::Event event) -> base::result::Result<base::Event>
    {
        // We assert that references exists, checking if the optional from Json getter is
        // empty ot not. Then if is a reference we get the value from the event, otherwise
        // we get the value from the parameter

        std::optional<int> lValue {event->getInt(targetField)};
        if (!lValue.has_value())
        {
            return base::result::makeFailure(event, failureTrace1);
        }

        int resolvedValue {0};
        if (helper::base::Parameter::Type::REFERENCE == rValueType)
        {
            std::optional<int> resolvedRValue {event->getInt(std::get<std::string>(rValue))};
            if (!resolvedRValue.has_value())
            {
                return base::result::makeFailure(event, failureTrace2);
            }
            resolvedValue = resolvedRValue.value();
        }
        else
        {
            resolvedValue = std::get<int>(rValue);
        }

        if (cmpFunction(lValue.value(), resolvedValue))
        {
            return base::result::makeSuccess(event, successTrace);
        }
        else
        {
            return base::result::makeFailure(event, failureTrace3);
        }
    };
}

/**
 * @brief Get the String Cmp Function object
 *
 * @param targetField Reference of the field to compare, obtained from the YAML key
 * @param op Operator to use
 * @param rightParameter Right parameter to compare, obtained from the YAML value
 * @param name Formatted name of the helper
 * @return std::function<base::result::Result<base::Event>(base::Event)>
 *
 * @throws std::runtime_error if helper::base::Parameter::Type is not supported
 */
std::function<base::result::Result<base::Event>(base::Event)> getStringCmpFunction(
    const std::string& targetField, Operator op, const helper::base::Parameter& rightParameter, const std::string& name)
{
    // Depending on rValue type we store the reference or the string value, string in both
    // cases
    std::string rValue {};
    const auto rValueType {rightParameter.m_type};
    rValue = rightParameter.m_value;

    // Depending on the operator we return the correct function
    std::function<bool(const std::string& l, const std::string& r)> cmpFunction;
    switch (op)
    {
        case Operator::EQ:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l == r;
            };
            break;
        case Operator::NE:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l != r;
            };
            break;
        case Operator::GT:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l > r;
            };
            break;
        case Operator::GE:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l >= r;
            };
            break;
        case Operator::LT:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l < r;
            };
            break;
        case Operator::LE:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l <= r;
            };
            break;
        case Operator::ST:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                return l.substr(0, r.length()) == r;
            };
            break;
        case Operator::CN:
            cmpFunction = [](const std::string& l, const std::string& r)
            {
                if (!r.empty())
                {
                    return l.find(r) != std::string::npos;
                }
                return false;
            };
            break;

        default: break;
    }

    // Tracing messages
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {
        fmt::format("[{}] -> Failure: Parameter \"{}\" not found", name, rightParameter.m_value)};
    const std::string failureTrace3 {fmt::format("[{}] -> Failure: Comparison is false", name)};

    // Function that implements the helper
    return [=](base::Event event) -> base::result::Result<base::Event>
    {
        // We assert that references exists, checking if the optional from Json getter is
        // empty ot not. Then if is a reference we get the value from the event, otherwise
        // we get the value from the parameter

        const auto lValue {event->getString(targetField)};
        if (!lValue.has_value())
        {
            return base::result::makeFailure(event, failureTrace1);
        }

        std::string resolvedValue {rValue};
        if (helper::base::Parameter::Type::REFERENCE == rValueType)
        {
            const auto resolvedRValue {event->getString(rValue)};
            if (!resolvedRValue.has_value())
            {
                return base::result::makeFailure(event, failureTrace2);
            }
            resolvedValue = resolvedRValue.value();
        }

        if (cmpFunction(lValue.value(), resolvedValue))
        {
            return base::result::makeSuccess(event, successTrace);
        }
        else
        {
            return base::result::makeFailure(event, failureTrace3);
        }
    };
}

/**
 * @brief Builds the Expression for the comparison helper
 *
 * @param definition Helper definition
 * @param op Comparison operator
 * @param type Type of the comparison
 * @return base::Expression
 */
base::Expression opBuilderComparison(const std::string& targetField,
                                     const std::string& rawName,
                                     const std::vector<std::string>& rawParameters,
                                     std::shared_ptr<defs::IDefinitions> definitions,
                                     Operator op,
                                     Type t)
{

    // Identify references and build JSON pointer paths
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    // Assert expected number of parameters
    helper::base::checkParametersSize(rawName, parameters, 1);
    // Format name for the tracer
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);
    // Get the expression depending on the type
    switch (t)
    {
        case Type::INT:
        {
            auto opFn = getIntCmpFunction(targetField, op, parameters[0], name);
            return base::Term<base::EngineOp>::create(name, opFn);
        }
        case Type::STRING:
        {
            auto opFn = getStringCmpFunction(targetField, op, parameters[0], name);
            return base::Term<base::EngineOp>::create(name, opFn);
        }
        default:
            throw std::runtime_error(
                fmt::format("{} function: Unsupported comparison type ({}).", name, static_cast<int>(t)));
    }
}

//*************************************************
//*               Int Cmp filters                 *
//*************************************************

// field: +int_equal/int|$ref/
base::Expression opBuilderHelperIntEqual(const std::string& targetField,
                                         const std::string& rawName,
                                         const std::vector<std::string>& rawParameters,
                                         std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::EQ, Type::INT)};
    return expression;
}

// field: +int_not_equal/int|$ref/
base::Expression opBuilderHelperIntNotEqual(const std::string& targetField,
                                            const std::string& rawName,
                                            const std::vector<std::string>& rawParameters,
                                            std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::NE, Type::INT)};
    return expression;
}

// field: +int_less/int|$ref/
base::Expression opBuilderHelperIntLessThan(const std::string& targetField,
                                            const std::string& rawName,
                                            const std::vector<std::string>& rawParameters,
                                            std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::LT, Type::INT)};
    return expression;
}

// field: +int_less_or_equal/int|$ref/
base::Expression opBuilderHelperIntLessThanEqual(const std::string& targetField,
                                                 const std::string& rawName,
                                                 const std::vector<std::string>& rawParameters,
                                                 std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::LE, Type::INT)};
    return expression;
}

// field: +int_greater/int|$ref/
base::Expression opBuilderHelperIntGreaterThan(const std::string& targetField,
                                               const std::string& rawName,
                                               const std::vector<std::string>& rawParameters,
                                               std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::GT, Type::INT)};
    return expression;
}

// field: +int_greater_or_equal/int|$ref/
base::Expression opBuilderHelperIntGreaterThanEqual(const std::string& targetField,
                                                    const std::string& rawName,
                                                    const std::vector<std::string>& rawParameters,
                                                    std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::GE, Type::INT)};
    return expression;
}

//*************************************************
//*           String Cmp filters                  *
//*************************************************

// field: +string_equal/value|$ref
base::Expression opBuilderHelperStringEqual(const std::string& targetField,
                                            const std::string& rawName,
                                            const std::vector<std::string>& rawParameters,
                                            std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::EQ, Type::STRING)};
    return expression;
}

// field: +string_not_equal/value|$ref
base::Expression opBuilderHelperStringNotEqual(const std::string& targetField,
                                               const std::string& rawName,
                                               const std::vector<std::string>& rawParameters,
                                               std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::NE, Type::STRING)};
    return expression;
}

// field: +string_greater/value|$ref
base::Expression opBuilderHelperStringGreaterThan(const std::string& targetField,
                                                  const std::string& rawName,
                                                  const std::vector<std::string>& rawParameters,
                                                  std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::GT, Type::STRING)};
    return expression;
}

// field: +string_greater_or_equal/value|$ref
base::Expression opBuilderHelperStringGreaterThanEqual(const std::string& targetField,
                                                       const std::string& rawName,
                                                       const std::vector<std::string>& rawParameters,
                                                       std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::GE, Type::STRING)};
    return expression;
}

// field: +string_less/value|$ref
base::Expression opBuilderHelperStringLessThan(const std::string& targetField,
                                               const std::string& rawName,
                                               const std::vector<std::string>& rawParameters,
                                               std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::LT, Type::STRING)};
    return expression;
}

// field: +string_less_or_equal/value|$ref
base::Expression opBuilderHelperStringLessThanEqual(const std::string& targetField,
                                                    const std::string& rawName,
                                                    const std::vector<std::string>& rawParameters,
                                                    std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::LE, Type::STRING)};
    return expression;
}

// field: +starts_with/value|$ref
base::Expression opBuilderHelperStringStarts(const std::string& targetField,
                                             const std::string& rawName,
                                             const std::vector<std::string>& rawParameters,
                                             std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::ST, Type::STRING)};
    return expression;
}

// field: +contains/value|$ref
base::Expression opBuilderHelperStringContains(const std::string& targetField,
                                               const std::string& rawName,
                                               const std::vector<std::string>& rawParameters,
                                               std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression {opBuilderComparison(targetField, rawName, rawParameters, definitions, Operator::CN, Type::STRING)};
    return expression;
}

//*************************************************
//*               Regex filters                   *
//*************************************************

// field: +regex_match/regexp
base::Expression opBuilderHelperRegexMatch(const std::string& targetField,
                                           const std::string& rawName,
                                           const std::vector<std::string>& rawParameters,
                                           std::shared_ptr<defs::IDefinitions> definitions)
{
    // Identify references and build JSON pointer paths
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    // Assert expected number of parameters
    helper::base::checkParametersSize(rawName, parameters, 1);
    // Parameter type check
    helper::base::checkParameterType(rawName, parameters[0], helper::base::Parameter::Type::VALUE);
    // Format name for the tracer
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    auto regex_ptr {std::make_shared<RE2>(parameters[0].m_value, RE2::Quiet)};
    if (!regex_ptr->ok())
    {
        throw std::runtime_error(fmt::format("\"{}\" function: "
                                             "Invalid regex: \"{}\".",
                                             name,
                                             parameters[0].m_value));
    }

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {fmt::format("[{}] -> Failure: Regex did not match", name)};

    // Return Term
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            const auto resolvedField {event->getString(targetField)};
            if (!resolvedField.has_value())
            {
                return base::result::makeFailure(event, failureTrace1);
            }

            if (RE2::PartialMatch(resolvedField.value(), *regex_ptr))
            {
                return base::result::makeSuccess(event, successTrace);
            }
            else
            {
                return base::result::makeFailure(event, failureTrace2);
            }
        });
}

// field: +regex_not_match/regexp
base::Expression opBuilderHelperRegexNotMatch(const std::string& targetField,
                                              const std::string& rawName,
                                              const std::vector<std::string>& rawParameters,
                                              std::shared_ptr<defs::IDefinitions> definitions)
{
    // TODO: Regex parameter fails at operationBuilderSplit

    // Identify references and build JSON pointer paths
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    // Assert expected number of parameters
    helper::base::checkParametersSize(rawName, parameters, 1);
    // Parameter type check
    helper::base::checkParameterType(rawName, parameters[0], helper::base::Parameter::Type::VALUE);
    // Format name for the tracer
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    auto regex_ptr {std::make_shared<RE2>(parameters[0].m_value, RE2::Quiet)};
    if (!regex_ptr->ok())
    {
        throw std::runtime_error(fmt::format("\"{}\" function: "
                                             "Invalid regex: \"{}\".",
                                             name,
                                             parameters[0].m_value));
    }

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {fmt::format("[{}] -> Failure: Regex did match", name)};

    // Return Term
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            const auto resolvedField {event->getString(targetField)};
            if (!resolvedField.has_value())
            {
                return base::result::makeFailure(event, failureTrace1);
            }

            if (!RE2::PartialMatch(resolvedField.value(), *regex_ptr))
            {
                return base::result::makeSuccess(event, successTrace);
            }
            else
            {
                return base::result::makeFailure(event, failureTrace2);
            }
        });
}

// //*************************************************
// //*               IP filters                     *
// //*************************************************

// field: +ip_cidr_match/192.168.0.0/16
// field: +ip_cidr_match/192.168.0.0/255.255.0.0
base::Expression opBuilderHelperIPCIDR(const std::string& targetField,
                                       const std::string& rawName,
                                       const std::vector<std::string>& rawParameters,
                                       std::shared_ptr<defs::IDefinitions> definitions)
{

    // Identify references and build JSON pointer paths
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    // Assert expected number of parameters
    helper::base::checkParametersSize(rawName, parameters, 2);
    // Parameter type check
    for (const auto& parameter : parameters)
    {
        helper::base::checkParameterType(rawName, parameter, helper::base::Parameter::Type::VALUE);
    }
    // Format name for the tracer
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    uint32_t network {};
    try
    {
        network = utils::ip::IPv4ToUInt(parameters[0].m_value);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(fmt::format("\"{}\" function: IPv4 address \"{}\" "
                                             "could not be converted to int: {}",
                                             name,
                                             network,
                                             e.what()));
    }

    uint32_t mask {};
    try
    {
        mask = utils::ip::IPv4MaskUInt(parameters[1].m_value);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(fmt::format("\"{}\" function: IPv4 Mask \"{}\" "
                                             "could not be converted to int: {}",
                                             name,
                                             parameters[1].m_value,
                                             e.what()));
    }

    uint32_t net_lower {network & mask};
    uint32_t net_upper {net_lower | (~mask)};

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {fmt::format("[{}] -> Failure: IPv4 address ", name)};
    const std::string failureTrace3 {fmt::format("[{}] -> Failure: IP address is not in CIDR", name)};

    // Return Term
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            const auto resolvedField {event->getString(targetField)};
            if (!resolvedField.has_value())
            {
                return base::result::makeFailure(event, failureTrace1);
            }

            uint32_t ip {};
            try
            {
                ip = utils::ip::IPv4ToUInt(resolvedField.value());
            }
            catch (std::exception& e)
            {
                return base::result::makeFailure(
                    event,
                    failureTrace2
                        + fmt::format("'{}' could not be converted to int: {}", resolvedField.value(), e.what()));
            }
            if (net_lower <= ip && ip <= net_upper)
            {
                return base::result::makeSuccess(event, successTrace);
            }
            else
            {
                return base::result::makeFailure(event, failureTrace3);
            }
        });
}

//*************************************************
//*               Existance filters               *
//*************************************************

// field: +exists
base::Expression opBuilderHelperExists(const std::string& targetField,
                                       const std::string& rawName,
                                       const std::vector<std::string>& rawParameters,
                                       std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};
    const std::string failureTrace {
        fmt::format("[{}] -> Failure: Target field '{}' does not exist", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            if (event->exists(targetField))
            {
                return base::result::makeSuccess(event, successTrace);
            }
            else
            {
                return base::result::makeFailure(event, failureTrace);
            }
        });
}

// field: +not_exists
base::Expression opBuilderHelperNotExists(const std::string& targetField,
                                          const std::string& rawName,
                                          const std::vector<std::string>& rawParameters,
                                          std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' does exist", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            if (!event->exists(targetField))
            {
                return base::result::makeSuccess(event, successTrace);
            }
            else
            {
                return base::result::makeFailure(event, failureTrace);
            }
        });
}

//*************************************************
//*               Array filters                   *
//*************************************************

base::Expression opBuilderHelperArrayPresence(const std::string& targetField,
                                              const std::string& rawName,
                                              const std::vector<std::string>& rawParameters,
                                              std::shared_ptr<defs::IDefinitions> definitions,
                                              bool checkPresence)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersMinSize(rawName, parameters, 1);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {
        fmt::format("[{}] -> Failure: Target field '{}' is not an array", name, targetField)};
    const std::string failureTrace3 {fmt::format("[{}] -> Failure: Target array '{}' {} of the parameters",
                                                 name,
                                                 targetField,
                                                 checkPresence ? "does not contain any": "contain at least one")};

    // Return Term
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            if (!event->exists(targetField))
            {
                return base::result::makeFailure(event, failureTrace1);
            }

            const auto resolvedArray {event->getArray(targetField)};
            if (!resolvedArray.has_value())
            {
                return base::result::makeFailure(event, failureTrace2);
            }

            json::Json cmpValue {};
            auto result = base::result::makeSuccess(event, successTrace);
            for (const auto& parameter : parameters)
            {
                switch (parameter.m_type)
                {
                    case helper::base::Parameter::Type::REFERENCE:
                    {
                        auto resolvedParameter {event->getJson(parameter.m_value)};
                        if (resolvedParameter.has_value())
                        {
                            cmpValue = std::move(resolvedParameter.value());
                        }
                        else
                        {
                            continue;
                        }
                    }
                    break;
                    case helper::base::Parameter::Type::VALUE:
                    {
                        cmpValue.setString(parameter.m_value);
                    }
                    break;
                }

                // Check if the array contains the value, if so finish
                if (std::find_if(resolvedArray.value().begin(),
                                 resolvedArray.value().end(),
                                 [&cmpValue](const json::Json& value) { return value == cmpValue; })
                    != resolvedArray.value().end())
                {
                    if(!checkPresence)
                    {
                        result = base::result::makeFailure(event, failureTrace3);
                    }
                    return result;
                }
            }

            if(checkPresence)
            {
                result = base::result::makeFailure(event, failureTrace3);
            }
            return result;
        });
}

// field: +array_contains/value1/value2/...valueN
base::Expression opBuilderHelperContainsString(const std::string& targetField,
                                               const std::string& rawName,
                                               const std::vector<std::string>& rawParameters,
                                               std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression = opBuilderHelperArrayPresence(targetField, rawName, rawParameters, definitions, true);
    return expression;
}

// field: +array_not_contains/value1/value2/...valueN
base::Expression opBuilderHelperNotContainsString(const std::string& targetField,
                                               const std::string& rawName,
                                               const std::vector<std::string>& rawParameters,
                                               std::shared_ptr<defs::IDefinitions> definitions)
{
    auto expression = opBuilderHelperArrayPresence(targetField, rawName, rawParameters, definitions, false);
    return expression;
}

//*************************************************
//*                Type filters                   *
//*************************************************

// field: +is_number
base::Expression opBuilderHelperIsNumber(const std::string& targetField,
                                         const std::string& rawName,
                                         const std::vector<std::string>& rawParameters,
                                         std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {
        fmt::format("[{}] -> Failure: Target field '{}' is not a number", name, targetField)};
    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (event->isNumber(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_not_number
base::Expression opBuilderHelperIsNotNumber(const std::string& targetField,
                                            const std::string& rawName,
                                            const std::vector<std::string>& rawParameters,
                                            std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is a number", name, targetField)};
    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (!event->isNumber(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_string
base::Expression opBuilderHelperIsString(const std::string& targetField,
                                         const std::string& rawName,
                                         const std::vector<std::string>& rawParameters,
                                         std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {
        fmt::format("[{}] -> Failure: Target field '{}' is not a string", name, targetField)};
    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (event->isString(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_not_string
base::Expression opBuilderHelperIsNotString(const std::string& targetField,
                                            const std::string& rawName,
                                            const std::vector<std::string>& rawParameters,
                                            std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is a string", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (!event->isString(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_boolean
base::Expression opBuilderHelperIsBool(const std::string& targetField,
                                       const std::string& rawName,
                                       const std::vector<std::string>& rawParameters,
                                       std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {
        fmt::format("[{}] -> Failure: Target field '{}' is not a boolean", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (event->isBool(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_not_boolean
base::Expression opBuilderHelperIsNotBool(const std::string& targetField,
                                          const std::string& rawName,
                                          const std::vector<std::string>& rawParameters,
                                          std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is a boolean", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (!event->isBool(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_array
base::Expression opBuilderHelperIsArray(const std::string& targetField,
                                        const std::string& rawName,
                                        const std::vector<std::string>& rawParameters,
                                        std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {
        fmt::format("[{}] -> Failure: Target field '{}' is not an array", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (event->isArray(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_not_array
base::Expression opBuilderHelperIsNotArray(const std::string& targetField,
                                           const std::string& rawName,
                                           const std::vector<std::string>& rawParameters,
                                           std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is an array", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (!event->isArray(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_object
base::Expression opBuilderHelperIsObject(const std::string& targetField,
                                         const std::string& rawName,
                                         const std::vector<std::string>& rawParameters,
                                         std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {
        fmt::format("[{}] -> Failure: Target field '{}' is not an object", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (event->isObject(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_not_object
base::Expression opBuilderHelperIsNotObject(const std::string& targetField,
                                            const std::string& rawName,
                                            const std::vector<std::string>& rawParameters,
                                            std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is an object", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (!event->isObject(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_null
base::Expression opBuilderHelperIsNull(const std::string& targetField,
                                       const std::string& rawName,
                                       const std::vector<std::string>& rawParameters,
                                       std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is not null", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (event->isNull(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_not_null
base::Expression opBuilderHelperIsNotNull(const std::string& targetField,
                                          const std::string& rawName,
                                          const std::vector<std::string>& rawParameters,
                                          std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is null", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->exists(targetField))
            {
                if (!event->isNull(targetField))
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_true
base::Expression opBuilderHelperIsTrue(const std::string& targetField,
                                       const std::string& rawName,
                                       const std::vector<std::string>& rawParameters,
                                       std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is false", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->getBool(targetField).has_value())
            {
                if (event->getBool(targetField).value())
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

// field: +is_false
base::Expression opBuilderHelperIsFalse(const std::string& targetField,
                                        const std::string& rawName,
                                        const std::vector<std::string>& rawParameters,
                                        std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions)};
    helper::base::checkParametersSize(rawName, parameters, 0);
    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // Tracing
    const auto successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace {fmt::format("[{}] -> Failure: Target field '{}' is true", name, targetField)};

    const std::string failureMissingValueTrace {
        fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};

    // Return result
    return base::Term<base::EngineOp>::create(
        name,
        [=, targetField = std::move(targetField)](base::Event event) -> base::result::Result<base::Event>
        {
            base::result::Result<base::Event> result;

            if (event->getBool(targetField).has_value())
            {
                if (!event->getBool(targetField).value())
                {
                    result = base::result::makeSuccess(event, successTrace);
                }
                else
                {
                    result = base::result::makeFailure(event, failureTrace);
                }
            }
            else
            {
                result = base::result::makeFailure(event, failureMissingValueTrace);
            }

            return result;
        });
}

//*************************************************
//*              Definition filters               *
//*************************************************

// <field>: +match_value/$<definition_array>|$<array_reference>
base::Expression opBuilderHelperMatchValue(const std::string& targetField,
                                           const std::string& rawName,
                                           const std::vector<std::string>& rawParameters,
                                           std::shared_ptr<defs::IDefinitions> definitions)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions, false)};
    helper::base::checkParametersSize(rawName, parameters, 1);

    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    std::optional<std::vector<json::Json>> definitionArray;

    if (helper::base::Parameter::Type::VALUE == parameters[0].m_type)
    {
        // Parameter is a definition
        json::Json definitionValue {};

        try
        {
            definitionValue = json::Json {parameters[0].m_value.c_str()};
        }
        catch (std::runtime_error& e)
        {
            throw std::runtime_error(
                fmt::format("Engine builder: [{}] Definition '{}' has an invalid type", name, parameters[0].m_value));
        }

        definitionArray = definitionValue.getArray();
        if (!definitionArray.has_value())
        {
            throw std::runtime_error(
                fmt::format("Engine builder: [{}] Definition '{}' is not an array", name, parameters[0].m_value));
        }
    }

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {
        fmt::format("[{}] -> Failure: Target field '{}' has an invalid type", name, targetField)};
    const std::string failureTrace3 {
        fmt::format("[{}] -> Failure: Parameter '{}' not found", name, parameters[0].m_value)};
    const std::string failureTrace4 {
        fmt::format("[{}] -> Failure: Parameter '{}' is not an array", name, parameters[0].m_value)};
    const std::string failureTrace5 {
        fmt::format("[{}] -> Failure: Array '{}' does not contain '{}'", name, parameters[0].m_value, targetField)};

    // Return Term
    return base::Term<base::EngineOp>::create(
        name,
        [=, parameter = std::move(parameters[0]), definitionArray = std::move(definitionArray)](
            base::Event event) -> base::result::Result<base::Event>
        {
            if (!event->exists(targetField))
            {
                return base::result::makeFailure(event, failureTrace1);
            }

            // Get value
            json::Json cmpValue {};
            {
                auto resolvedField {event->getJson(targetField)};
                if (resolvedField.has_value())
                {
                    cmpValue = std::move(resolvedField.value());
                }
                else
                {
                    return base::result::makeFailure(event, failureTrace2);
                }
            }

            bool isSuccess {false};
            // TODO Should be a function in json::Json for search in array
            auto searchCmpValue = [&cmpValue](const std::vector<json::Json>& def) -> bool
            {
               return std::find_if(def.begin(), def.end(), [&cmpValue](const json::Json& value) { return value == cmpValue; })
                    != def.end();
            };

            // Get array
            if (helper::base::Parameter::Type::REFERENCE == parameter.m_type)
            {
                // TODO Should be 1 trace, if exist and is array, in all helers, no shearch for existance twice
                // Parameter is a reference
                if (!event->exists(parameter.m_value))
                {
                    return base::result::makeFailure(event, failureTrace3);
                }

                if (!event->isArray(parameter.m_value))
                {
                    return base::result::makeFailure(event, failureTrace4);
                }

                isSuccess = searchCmpValue(event->getArray(parameter.m_value).value());
            }
            else
            {
                // Parameter is a definition
                isSuccess = searchCmpValue(definitionArray.value());
            }

            // Check if the array contains the value
            if (isSuccess)
            {
                return base::result::makeSuccess(event, successTrace);
            }

            // Not found
            return base::result::makeFailure(event, failureTrace5);
        });
}

// <field>: +match_key/$<definition_object>|$<object_reference>
base::Expression opBuilderHelperMatchKey(const std::string& targetField,
                                         const std::string& rawName,
                                         const std::vector<std::string>& rawParameters,
                                         std::shared_ptr<defs::IDefinitions> definitions,
                                         std::shared_ptr<schemf::ISchema> schema)
{
    auto parameters {helper::base::processParameters(rawName, rawParameters, definitions, false)};
    helper::base::checkParametersSize(rawName, parameters, 1);

    const auto name = helper::base::formatHelperName(rawName, targetField, parameters);

    // If target field (key) is a schema field, the value should be a string
    if (schema->hasField(DotPath::fromJsonPath(targetField))
        && (schema->getType(DotPath::fromJsonPath(targetField)) != json::Json::Type::String))
    {
        throw std::runtime_error(
            fmt::format("Engine helper builder: [{}] failed schema validation: Target field '{}' value is not a string",
                        name,
                        targetField));
    }

    std::optional<json::Json> definitionObject;

    if (helper::base::Parameter::Type::VALUE == parameters[0].m_type)
    {
        // Parameter is a definition
        json::Json definitionValue {};

        try
        {
            definitionValue = json::Json {parameters[0].m_value.c_str()};
        }
        catch (std::runtime_error& e)
        {
            throw std::runtime_error(
                fmt::format("Engine builder: [{}] Definition '{}' has an invalid type", name, parameters[0].m_value));
        }

        definitionObject = definitionValue.getJson();
        if (!definitionObject.has_value())
        {
            throw std::runtime_error(
                fmt::format("Engine builder: [{}] Definition '{}' has an invalid type", name, parameters[0].m_value));
        }

        if (!definitionObject->isObject())
        {
            throw std::runtime_error(
                fmt::format("Engine builder: [{}] Definition '{}' is not an object", name, parameters[0].m_value));
        }
    }

    // Tracing
    const std::string successTrace {fmt::format("[{}] -> Success", name)};

    const std::string failureTrace1 {fmt::format("[{}] -> Failure: Target field '{}' not found", name, targetField)};
    const std::string failureTrace2 {
        fmt::format("[{}] -> Failure: Target field '{}' is not a string", name, targetField)};
    const std::string failureTrace3 {
        fmt::format("[{}] -> Failure: Parameter '{}' not found", name, parameters[0].m_value)};
    const std::string failureTrace4 {
        fmt::format("[{}] -> Failure: Parameter '{}' has an invalid type", name, parameters[0].m_value)}; // TODO: ???
    const std::string failureTrace5 {
        fmt::format("[{}] -> Failure: Parameter '{}' is not an object", name, parameters[0].m_value)};

    const std::string failureTrace6 {
        fmt::format("[{}] -> Failure: Object '{}' does not contain '{}'", name, parameters[0].m_value, targetField)};

    // Return Term
    return base::Term<base::EngineOp>::create(
        name,
        [=, parameter = std::move(parameters[0]), definitionObject = std::move(definitionObject)](
            base::Event event) -> base::result::Result<base::Event>
        {
            // Get key
            if (!event->exists(targetField))
            {
                return base::result::makeFailure(event, failureTrace1);
            }

            if (!event->isString(targetField))
            {
                return base::result::makeFailure(event, failureTrace2);
            }

            auto pointerPath = json::Json::formatJsonPath(event->getString(targetField).value());
            bool exists {false};

            if (helper::base::Parameter::Type::REFERENCE == parameter.m_type)
            {
                // Parameter is a reference
                if (!event->exists(parameter.m_value))
                {
                    return base::result::makeFailure(event, failureTrace3);
                }

                if (!event->isObject(parameter.m_value))
                {
                    return base::result::makeFailure(event, failureTrace5);
                }

                exists = event->exists(parameter.m_value + pointerPath);
            }
            else
            {
                // Parameter is a definition
                exists = definitionObject->exists(pointerPath);
            }

            // Check if object contains the key
            if (!exists)
            {
                return base::result::makeFailure(event, failureTrace6);
            }

            return base::result::makeSuccess(event, successTrace);
        });
}

HelperBuilder getOpBuilderHelperMatchKey(std::shared_ptr<schemf::ISchema> schema)
{
    return [schema](const std::string& targetField,
                    const std::string& rawName,
                    const std::vector<std::string>& rawParameters,
                    std::shared_ptr<defs::IDefinitions> definitions)
    {
        return opBuilderHelperMatchKey(targetField, rawName, rawParameters, definitions, schema);
    };
}

} // namespace builder::internals::builders
