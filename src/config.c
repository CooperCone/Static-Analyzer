#include "config.h"
#include "config_internal.h"

#include "buffer.h"
#include "debug.h"
#include "logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void appendConfigTok(ConfigTokenList *list, ConfigToken tok) {
    list->numTokens++;
    list->tokens = realloc(list->tokens, list->numTokens * sizeof(ConfigToken));
    list->tokens[list->numTokens - 1] = tok;
}

#define ConfigSymbolTok(tok) else if (peek(buff) == tok) {\
    appendConfigTok(outList, (ConfigToken){ tok });\
    consume(buff);\
}

bool tokenizeConfig(Buffer *buff, ConfigTokenList *outList) {
    buff->pos = 0;

    while (buff->pos < buff->size) {
        if (isspace(peek(buff))) {
            consume(buff);
        }

        ConfigSymbolTok(':')
        ConfigSymbolTok(',')
        ConfigSymbolTok('[')
        ConfigSymbolTok(']')

        // FIXME: We currently assume everything else is a string
        // TODO: Will we need any numbers?
        else {
            uint64_t stringLength = 0;

            while (peekAhead(buff, stringLength) != ':' &&
                   peekAhead(buff, stringLength) != ',' &&
                   !isspace(peekAhead(buff, stringLength)))
            {
                stringLength++;
            }

            char *tokenStr = NULL;
            consumeAndCopyOut(buff, stringLength, &tokenStr);

            ConfigToken token = { ConfigToken_String, .string = tokenStr };
            appendConfigTok(outList, token);
        }
    }

    return true;
}

void printConfigTokens(ConfigTokenList tokens) {
    printDebug("Config Token List:\n");
    for (size_t i = 0; i < tokens.numTokens; i++) {
        if (tokens.tokens[i].type == ConfigToken_String) {
            printDebug("Str: %s\n", tokens.tokens[i].string);
        }
        // FIXME: Is 127 really the valid upper bound?
        else if (tokens.tokens[i].type < 127) {
            printDebug("%c\n", tokens.tokens[i].type);
        }
        else {
            logFatal("Config: Found invalid config token when printing: %d\n", tokens.tokens[i].type);
        }
    }
}

bool parseConfigValue(ConfigTokenList *tokens, ConfigValue *outValue) {
    if (tokens->tokens[tokens->curToken].type == '[') {
        // Parse a list
        tokens->curToken++;

        ConfigValue listValue = { ConfigValue_List };
        while (tokens->tokens[tokens->curToken].type != ']') {
            ConfigValue listElem = {0};

            if (!parseConfigValue(tokens, &listElem))
                return false;

            listValue.listSize++;
            listValue.listValues = realloc(listValue.listValues,
                listValue.listSize * sizeof(ConfigValue));
            listValue.listValues[listValue.listSize - 1] = listElem;
        
            if (tokens->tokens[tokens->curToken].type == ',') {
                tokens->curToken++;
            }
            else if (tokens->tokens[tokens->curToken].type == ']') {
                break;
            }
            else {
                logError("Config: Invalid token between list elems: %c\n", tokens->tokens[tokens->curToken].type);
                return false;
            }
        }
        tokens->curToken++;

        *outValue = listValue;
        return true;
    }
    else if (tokens->tokens[tokens->curToken].type == ConfigToken_String) {
        char *stringValue = tokens->tokens[tokens->curToken].string;

        tokens->curToken++;

        if (tokens->curToken < tokens->numTokens &&
            tokens->tokens[tokens->curToken].type != ':')
        {
            // Just a string
            *outValue = (ConfigValue) {
                .type = ConfigValue_String,
                .string = stringValue
            };
            return true;
        }

        tokens->curToken++;
        
        // If we're here, then we have a map
        ConfigValue mapValue = {0};
        if (!parseConfigValue(tokens, &mapValue))
            return false;

        ConfigValue *valueMem = malloc(sizeof(ConfigValue));
        memcpy(valueMem, &mapValue, sizeof(ConfigValue));

        *outValue = (ConfigValue) {
            .type = ConfigValue_Map,
            .mapKey = stringValue,
            .mapValue = valueMem
        };
        return true;
    }
    else {
        logError("Config: Invalid config token when parsing a value. Expected a string or '[': %d\n",
            tokens->tokens[tokens->curToken].type);
        return false;
    }
}

bool readConfigFile(char *configFileName, Config *outConfig) {
    Buffer buffer = {0};
    if (!openAndReadFileToBuffer(configFileName, &buffer)) {
        char *fileError = strerror(errno);
        logError("Config: Couldn't read config file with name: %s. \nError msg: %s\n",
            configFileName, fileError);
        return false;
    }

    ConfigTokenList tokens = {0};
    tokenizeConfig(&buffer, &tokens);

    printConfigTokens(tokens);

    printDebug("\n\n");

    tokens.curToken = 0;

    while (tokens.curToken < tokens.numTokens) {
        ConfigValue value = {0};
        if (!parseConfigValue(&tokens, &value))
            return false;
        
        outConfig->numConfigValues++;
        outConfig->configValues = realloc(outConfig->configValues,
            outConfig->numConfigValues * sizeof(ConfigValue));
        outConfig->configValues[outConfig->numConfigValues - 1] =
            value;
    }

    printConfig(*outConfig);

    return true;
}

void printConfigValueIndent(ConfigValue value, uint64_t indent) {
    for (uint64_t i = 0; i < indent; i++)
        printDebug(" ");
    
    if (value.type == ConfigValue_String) {
        printDebug("%s\n", value.string);
    }
    else if (value.type == ConfigValue_List) {
        printDebug("[\n");

        for (uint64_t i = 0; i < value.listSize; i++) {
            printConfigValueIndent(value.listValues[i], indent + 2);
        }

        for (uint64_t i = 0; i < indent; i++)
            printDebug(" ");
        printDebug("]\n");
    }
    else if (value.type == ConfigValue_Map) {
        printDebug("%s:\n", value.mapKey);
        printConfigValueIndent(*(value.mapValue), indent + 2);
    }
}

void printConfig(Config config) {
    printDebug("Config:\n");
    for (uint64_t i = 0; i < config.numConfigValues; i++) {
        printConfigValueIndent(config.configValues[i], 2);
    }
}
