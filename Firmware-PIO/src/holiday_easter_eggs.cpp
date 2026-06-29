#include "holiday_easter_eggs.h"

#include "holiday_christmas.h"
#include "holiday_halloween.h"
#include "holiday_july_fourth.h"
#include "holiday_st_patricks.h"
#include "holiday_valentines.h"

#include <MD_MAX72xx.h>
#include <time.h>

static const unsigned long HOLIDAY_EGG_INTERVAL_MS = 5UL * 60UL * 1000UL;
static const char *NTP_SERVER_1 = "pool.ntp.org";
static const char *NTP_SERVER_2 = "time.nist.gov";
static const char *TZ_EST = "EST5EDT,M3.2.0/2,M11.1.0/2";

static bool timeSynced = false;
static unsigned long lastHolidayEggMs = 0;

void holidayScrollMessage(MD_Parola &display, const char *message) {
  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setTextAlignment(PA_LEFT);
  display.displayScroll(message, PA_LEFT, PA_SCROLL_LEFT, 80);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(400);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(0);
}

bool holidayEasterEggsInit() {
  setenv("TZ", TZ_EST, 1);
  tzset();
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);

  struct tm timeinfo;
  for (int attempt = 0; attempt < 20; attempt++) {
    if (getLocalTime(&timeinfo, 1000)) {
      timeSynced = true;
      lastHolidayEggMs = millis();
      Serial.println("Holiday clock synced (EST).");
      return true;
    }
    delay(100);
  }

  Serial.println("Holiday clock sync failed.");
  return false;
}

HolidayId getCurrentHoliday() {
  if (!timeSynced) {
    return HolidayId::None;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return HolidayId::None;
  }

  if (timeinfo.tm_mon == 2 && timeinfo.tm_mday == 17) {
    return HolidayId::StPatricksDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 14) {
    return HolidayId::ValentinesDay;
  }
  if (timeinfo.tm_mon == 6 && timeinfo.tm_mday == 4) {
    return HolidayId::JulyFourth;
  }
  if (timeinfo.tm_mon == 9 && timeinfo.tm_mday == 31) {
    return HolidayId::Halloween;
  }
  if (timeinfo.tm_mon == 11 && timeinfo.tm_mday == 25) {
    return HolidayId::Christmas;
  }

  return HolidayId::None;
}

void runHolidayEasterEgg(MD_Parola &display, HolidayId holiday) {
  switch (holiday) {
  case HolidayId::StPatricksDay:
    runStPatricksHolidayAnimation(display);
    break;
  case HolidayId::Halloween:
    runHalloweenHolidayAnimation(display);
    break;
  case HolidayId::Christmas:
    runChristmasHolidayAnimation(display);
    break;
  case HolidayId::ValentinesDay:
    runValentinesHolidayAnimation(display);
    break;
  case HolidayId::JulyFourth:
    runJulyFourthHolidayAnimation(display);
    break;
  case HolidayId::None:
    break;
  }
}

bool checkAndRunHolidayEasterEgg(MD_Parola &display) {
  HolidayId holiday = getCurrentHoliday();
  if (holiday == HolidayId::None) {
    return false;
  }

  unsigned long now = millis();
  if (lastHolidayEggMs != 0 && now - lastHolidayEggMs < HOLIDAY_EGG_INTERVAL_MS) {
    return false;
  }

  Serial.print("Holiday easter egg: ");
  Serial.println(static_cast<uint8_t>(holiday));

  runHolidayEasterEgg(display, holiday);
  lastHolidayEggMs = now;
  return true;
}
