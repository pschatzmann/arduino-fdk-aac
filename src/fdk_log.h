#pragma once

// User Settings: Activate/Deactivate logging
#ifndef HELIX_LOGGING_ACTIVE
#define HELIX_LOGGING_ACTIVE true
#endif
#ifndef HELIX_LOG_LEVEL
#define HELIX_LOG_LEVEL Warning
#endif

// Logging Implementation
#if HELIX_LOGGING_ACTIVE == true

#ifndef ARDUINO
#include <stdio.h>
#endif

static char log_buffer[512];
enum LogLevel {Debug, Info, Warning, Error};
static LogLevel minLogLevel = Debug;

static const char* levelName(LogLevel level) {
    switch(level){
        case Debug:
            return "D";
        case Info:
            return "I";
        case Warning:
            return "W";
        case Error:
            return "E";
    }
    return "";
}

static void printLog(const char*msg){
#ifdef ARDUINO
    Serial.print(msg);
#else
    printf("%s",msg);
#endif
}

static void printLog(const char* file, int line, LogLevel current_level) {
    const char* file_name = strrchr(file, '/') ? strrchr(file, '/') + 1 : file;
    const char* level_code = levelName(current_level);
    printLog("[");
    printLog(level_code);
    printLog("] ");
    printLog(file_name);
    printLog(" : ");
    char line_str[20];
    snprintf(line_str,20,"%d",line);
    printLog(line_str);
    printLog(" - ");
    printLog(log_buffer);
    printLog("\n");
}

// We print the log based on the log level
#define LOG(level,...) { if(level>=minLogLevel) {  snprintf(log_buffer,512, __VA_ARGS__);  printLog(__FILE__,__LINE__, level); } }
#else
// Remove all log statments from the code
#define LOG(Debug, ...) 
#endif
