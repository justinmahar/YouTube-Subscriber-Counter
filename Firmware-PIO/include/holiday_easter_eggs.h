#ifndef HOLIDAY_EASTER_EGGS_H
#define HOLIDAY_EASTER_EGGS_H

#include <MD_Parola.h>
#include <stdint.h>

enum class HolidayId : uint8_t {
  None = 0,
  StPatricksDay = 1,
  Halloween = 2,
  Christmas = 3,
  ValentinesDay = 4,
  JulyFourth = 5,
};

// Set true to preview a holiday animation on boot (before intro/WiFi).
const bool RUN_HOLIDAY_PREVIEW_ON_BOOT = true;
const HolidayId HOLIDAY_PREVIEW_BOOT = HolidayId::JulyFourth;

// Sync NTP and configure America/Eastern. Call after WiFi connects.
bool holidayEasterEggsInit();

// Returns today's holiday in EST, or None.
HolidayId getCurrentHoliday();

// Play animation + scrolling greeting for the given holiday.
void runHolidayEasterEgg(MD_Parola &display, HolidayId holiday);

// Shared ending for holiday animations.
void holidayScrollMessage(MD_Parola &display, const char *message);

// On holidays, runs the easter egg every 5 minutes. Returns true if one played.
bool checkAndRunHolidayEasterEgg(MD_Parola &display);

#endif
