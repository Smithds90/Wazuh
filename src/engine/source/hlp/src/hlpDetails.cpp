#include "hlpDetails.hpp"
#include "specificParsers.hpp"

const parserConfigFuncPtr kParsersConfig[] = {
    nullptr,
    configureAnyParser,
    nullptr,
    nullptr,
    configureTsParser,
    nullptr,
    configureJsonParser,
    configureMapParser,
    configureDomainParser,
    configureFilepathParser,
    nullptr,
    nullptr,
    configureQuotedString,
    configureBooleanParser,
    nullptr,
    configureIgnoreParser,
};

const parserFuncPtr kAvailableParsers[] = {
    parseAny,
    parseAny,
    matchLiteral,
    parseIPaddress,
    parseTimeStamp,
    parseURL,
    parseJson,
    parseMap,
    parseDomain,
    parseFilePath,
    parseUserAgent,
    parseNumber,
    parseQuotedString,
    parseBoolean,
    nullptr,
    parseIgnore,
};
