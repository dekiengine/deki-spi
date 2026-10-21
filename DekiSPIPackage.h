#pragma once

#ifdef _WIN32
#ifdef DEKI_SPI_EXPORTS
#define DEKI_SPI_API __declspec(dllexport)
#else
#define DEKI_SPI_API __declspec(dllimport)
#endif
#else
#define DEKI_SPI_API __attribute__((visibility("default")))
#endif
