// src/config.h.in
// cmake replaces $<IF:$<CONFIG:Debug>,$<TIMESTAMP:%Y-%m-%d %H:%M:%S>,$<DATE:%Y-%m-%d %H:%M:%S>>> with the actual build date string
#pragma once

#define BUILD_DATE "$<IF:$<CONFIG:Debug>,$<TIMESTAMP:%Y-%m-%d %H:%M:%S>,$<DATE:%Y-%m-%d %H:%M:%S>>>"
