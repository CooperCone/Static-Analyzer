#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>

static Severity g_severity;

void setSeverity(Severity severity) {
    g_severity = severity;
}

char *strFromSeverity(Severity severity) {
    switch (severity) {
        case Severity_Debug: {
            return "Debug";
        }
        case Severity_Info: {
            return "Info";
        }
        case Severity_Warn: {
            return "Warn";
        }
        case Severity_Error: {
            return "Error";
        }
        case Severity_Fatal: {
            return "Fatal";
        }
        default: {
            assert(false);
        }
    }

    return "";
}

void vlog(char *file, int line, Severity severity, const char *format, ...) {
    va_list args = {0};
    va_start(args, format);

    if (severity >= g_severity) {
        printf("(%s) %s:%d - ", strFromSeverity(severity), file, line);
        vprintf(format, args);
    }

    va_end(args);
}
