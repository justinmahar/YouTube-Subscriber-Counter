#include "holiday_easter_eggs.h"

#include "holiday_april_fools.h"
#include "holiday_birthdays.h"
#include "holiday_christmas.h"
#include "holiday_cinco_de_mayo.h"
#include "holiday_coffee_day.h"
#include "holiday_creator_cat_day.h"
#include "holiday_easter.h"
#include "holiday_earth_day.h"
#include "holiday_fathers_day.h"
#include "holiday_groundhog.h"
#include "holiday_halloween.h"
#include "holiday_july_fourth.h"
#include "holiday_labor_day.h"
#include "holiday_leap_day.h"
#include "holiday_memorial_day.h"
#include "holiday_new_years_eve.h"
#include "holiday_pi_day.h"
#include "holiday_presidents_day.h"
#include "holiday_pirate_day.h"
#include "holiday_st_patricks.h"
#include "holiday_thanksgiving.h"
#include "holiday_valentines.h"
#include "holiday_veterans_day.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>
#include <time.h>

static const unsigned long HOLIDAY_EGG_INTERVAL_MS = 5UL * 60UL * 1000UL;
static const char *TZ_EST = "EST5EDT,M3.2.0/2,M11.1.0/2";

static bool timeSynced = false;
static time_t holidayServerTimeUnix = 0;
static unsigned long holidayServerTimeMillis = 0;
static unsigned long lastHolidayEggMs = 0;

static void drawHolidayStar(MilestoneCtx &ctx, int cx, int cy, int radius) {
  ctx.matrix->setPoint(cy, ctx.colStart + cx, true);
  for (int step = 1; step <= radius; step++) {
    if (cx - step >= 0) {
      ctx.matrix->setPoint(cy, ctx.colStart + cx - step, true);
    }
    if (cx + step < ctx.width) {
      ctx.matrix->setPoint(cy, ctx.colStart + cx + step, true);
    }
    if (cy - step >= 0) {
      ctx.matrix->setPoint(cy - step, ctx.colStart + cx, true);
    }
    if (cy + step < ctx.height) {
      ctx.matrix->setPoint(cy + step, ctx.colStart + cx, true);
    }
    if (step <= 2) {
      if (cx - step >= 0 && cy - step >= 0) {
        ctx.matrix->setPoint(cy - step, ctx.colStart + cx - step, true);
      }
      if (cx + step < ctx.width && cy - step >= 0) {
        ctx.matrix->setPoint(cy - step, ctx.colStart + cx + step, true);
      }
      if (cx - step >= 0 && cy + step < ctx.height) {
        ctx.matrix->setPoint(cy + step, ctx.colStart + cx - step, true);
      }
      if (cx + step < ctx.width && cy + step < ctx.height) {
        ctx.matrix->setPoint(cy + step, ctx.colStart + cx + step, true);
      }
    }
  }
}

static void runHolidayIntroAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 9);

  for (int frame = 0; frame < ctx.width / 2 + 8; frame++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      int edgeDistance = min(c, ctx.width - 1 - c);
      if (edgeDistance <= frame) {
        ctx.matrix->setPoint(0, ctx.colStart + c, true);
        ctx.matrix->setPoint(ctx.height - 1, ctx.colStart + c, true);
      }
      if ((c + frame) % 5 == 0) {
        int row = (frame + c * 3) % ctx.height;
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }

    int radius = min(4, max(1, frame / 2));
    drawHolidayStar(ctx, ctx.cx, ctx.cy, radius);
    milestoneFrameShow(ctx, 36, min(15, 7 + frame / 3));
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    drawHolidayStar(ctx, ctx.cx, ctx.cy, 4);
    if (pulse % 2 == 0) {
      for (int c = 0; c < ctx.width; c += 3) {
        ctx.matrix->setPoint((c + pulse) % ctx.height, ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, pulse == 2 ? 120 : 70, pulse % 2 == 0 ? 15 : 10);
  }

  milestoneEffectEnd(ctx);

  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(11);
  display.setTextAlignment(PA_LEFT);
  display.displayScroll("Holiday!", PA_LEFT, PA_SCROLL_LEFT, 60);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(200);
}

static bool isThanksgiving(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 10 && timeinfo.tm_wday == 4 &&
         timeinfo.tm_mday >= 22 && timeinfo.tm_mday <= 28;
}

static bool isMemorialDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 4 && timeinfo.tm_wday == 1 &&
         timeinfo.tm_mday >= 25 && timeinfo.tm_mday <= 31;
}

static bool isLaborDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 8 && timeinfo.tm_wday == 1 &&
         timeinfo.tm_mday >= 1 && timeinfo.tm_mday <= 7;
}

static bool isFathersDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 5 && timeinfo.tm_wday == 0 &&
         timeinfo.tm_mday >= 15 && timeinfo.tm_mday <= 21;
}

static bool isPresidentsDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 1 && timeinfo.tm_wday == 1 &&
         timeinfo.tm_mday >= 15 && timeinfo.tm_mday <= 21;
}

static bool isEaster(const struct tm &timeinfo) {
  int year = timeinfo.tm_year + 1900;
  int goldenYear = year % 19;
  int century = year / 100;
  int yearOfCentury = year % 100;
  int leapCentury = century / 4;
  int centuryRemainder = century % 4;
  int correction = (century + 8) / 25;
  int moonCorrection = (century - correction + 1) / 3;
  int epact = (19 * goldenYear + century - leapCentury - moonCorrection + 15) % 30;
  int yearLeap = yearOfCentury / 4;
  int yearRemainder = yearOfCentury % 4;
  int weekdayCorrection =
      (32 + 2 * centuryRemainder + 2 * yearLeap - epact - yearRemainder) % 7;
  int monthOffset =
      (goldenYear + 11 * epact + 22 * weekdayCorrection) / 451;
  int easterMonth =
      (epact + weekdayCorrection - 7 * monthOffset + 114) / 31;
  int easterDay =
      ((epact + weekdayCorrection - 7 * monthOffset + 114) % 31) + 1;

  return timeinfo.tm_mon == easterMonth - 1 && timeinfo.tm_mday == easterDay;
}

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
  Serial.println("Holiday timezone configured (EST).");
  return true;
}

bool holidayEasterEggsSetServerTime(double serverTimeUnix) {
  if (!isfinite(serverTimeUnix) || serverTimeUnix <= 0) {
    Serial.println("Holiday clock missing valid server time.");
    return false;
  }

  unsigned long now = millis();
  bool wasTimeSynced = timeSynced;
  holidayServerTimeUnix = (time_t)serverTimeUnix;
  holidayServerTimeMillis = now;
  timeSynced = true;
  if (!wasTimeSynced) {
    lastHolidayEggMs = now;
  }
  Serial.println("Holiday clock synced from stats API.");
  return true;
}

HolidayId getCurrentHoliday() {
  if (!timeSynced) {
    return HolidayId::None;
  }

  time_t currentUnixTime =
      holidayServerTimeUnix + ((millis() - holidayServerTimeMillis) / 1000);
  struct tm timeinfo;
  if (localtime_r(&currentUnixTime, &timeinfo) == nullptr) {
    return HolidayId::None;
  }

  if (isEaster(timeinfo)) {
    return HolidayId::Easter;
  }
  if (timeinfo.tm_mon == 2 && timeinfo.tm_mday == 17) {
    return HolidayId::StPatricksDay;
  }
  if (timeinfo.tm_mon == 2 && timeinfo.tm_mday == 14) {
    return HolidayId::PiDay;
  }
  if (timeinfo.tm_mon == 3 && timeinfo.tm_mday == 1) {
    return HolidayId::AprilFools;
  }
  if (timeinfo.tm_mon == 3 && timeinfo.tm_mday == 22) {
    return HolidayId::EarthDay;
  }
  if (timeinfo.tm_mon == 4 && timeinfo.tm_mday == 5) {
    return HolidayId::CincoDeMayo;
  }
  if (isMemorialDay(timeinfo)) {
    return HolidayId::MemorialDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 2) {
    return HolidayId::GroundhogDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 14) {
    return HolidayId::ValentinesDay;
  }
  if (isPresidentsDay(timeinfo)) {
    return HolidayId::PresidentsDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 29) {
    return HolidayId::LeapDay;
  }
  if (isFathersDay(timeinfo)) {
    return HolidayId::FathersDay;
  }
  if (timeinfo.tm_mon == 6 && timeinfo.tm_mday == 4) {
    return HolidayId::JulyFourth;
  }
  if (timeinfo.tm_mon == 7 && timeinfo.tm_mday == 8) {
    return HolidayId::CreatorCatDay;
  }
  if (timeinfo.tm_mon == 7 && timeinfo.tm_mday == 12) {
    return HolidayId::JustinBirthday;
  }
  if (timeinfo.tm_mon == 8 && timeinfo.tm_mday == 12) {
    return HolidayId::DadBirthday;
  }
  if (isLaborDay(timeinfo)) {
    return HolidayId::LaborDay;
  }
  if (timeinfo.tm_mon == 8 && timeinfo.tm_mday == 19) {
    return HolidayId::PirateDay;
  }
  if (timeinfo.tm_mon == 8 && timeinfo.tm_mday == 29) {
    return HolidayId::CoffeeDay;
  }
  if (timeinfo.tm_mon == 9 && timeinfo.tm_mday == 31) {
    return HolidayId::Halloween;
  }
  if (timeinfo.tm_mon == 10 && timeinfo.tm_mday == 11) {
    return HolidayId::VeteransDay;
  }
  if (isThanksgiving(timeinfo)) {
    return HolidayId::Thanksgiving;
  }
  if (timeinfo.tm_mon == 11 && timeinfo.tm_mday == 25) {
    return HolidayId::Christmas;
  }
  if ((timeinfo.tm_mon == 11 && timeinfo.tm_mday == 31) ||
      (timeinfo.tm_mon == 0 && timeinfo.tm_mday == 1)) {
    return HolidayId::NewYearsEve;
  }

  return HolidayId::None;
}

void runHolidayPreviewCycle(MD_Parola &display) {
  static const HolidayId previewOrder[] = {
      HolidayId::NewYearsEve,      HolidayId::GroundhogDay,
      HolidayId::ValentinesDay,    HolidayId::PresidentsDay,
      HolidayId::LeapDay,          HolidayId::PiDay,
      HolidayId::StPatricksDay,    HolidayId::Easter,
      HolidayId::AprilFools,       HolidayId::EarthDay,
      HolidayId::CincoDeMayo,      HolidayId::MemorialDay,
      HolidayId::FathersDay,       HolidayId::CreatorCatDay,
      HolidayId::LaborDay,         HolidayId::PirateDay,
      HolidayId::CoffeeDay,
      HolidayId::Halloween,        HolidayId::VeteransDay,
      HolidayId::Thanksgiving,
  };

  for (HolidayId holiday : previewOrder) {
    Serial.print("Holiday preview: ");
    Serial.println(static_cast<uint8_t>(holiday));
    runHolidayEasterEgg(display, holiday);
  }
}

void runHolidayEasterEgg(MD_Parola &display, HolidayId holiday) {
  if (holiday != HolidayId::None) {
    runHolidayIntroAnimation(display);
  }

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
  case HolidayId::GroundhogDay:
    runGroundhogHolidayAnimation(display);
    break;
  case HolidayId::NewYearsEve:
    runNewYearsEveHolidayAnimation(display);
    break;
  case HolidayId::PiDay:
    runPiDayHolidayAnimation(display);
    break;
  case HolidayId::AprilFools:
    runAprilFoolsHolidayAnimation(display);
    break;
  case HolidayId::CincoDeMayo:
    runCincoDeMayoHolidayAnimation(display);
    break;
  case HolidayId::JustinBirthday:
    runBirthdayHolidayAnimation(display, "Justin's birthday!");
    break;
  case HolidayId::DadBirthday:
    runBirthdayHolidayAnimation(display, "Happy birthday Dad!");
    break;
  case HolidayId::PirateDay:
    runPirateDayHolidayAnimation(display);
    break;
  case HolidayId::VeteransDay:
    runVeteransDayHolidayAnimation(display);
    break;
  case HolidayId::Thanksgiving:
    runThanksgivingHolidayAnimation(display);
    break;
  case HolidayId::EarthDay:
    runEarthDayHolidayAnimation(display);
    break;
  case HolidayId::MemorialDay:
    runMemorialDayHolidayAnimation(display);
    break;
  case HolidayId::CreatorCatDay:
    runCreatorCatDayHolidayAnimation(display);
    break;
  case HolidayId::LaborDay:
    runLaborDayHolidayAnimation(display);
    break;
  case HolidayId::LeapDay:
    runLeapDayHolidayAnimation(display);
    break;
  case HolidayId::Easter:
    runEasterHolidayAnimation(display);
    break;
  case HolidayId::FathersDay:
    runFathersDayHolidayAnimation(display);
    break;
  case HolidayId::PresidentsDay:
    runPresidentsDayHolidayAnimation(display);
    break;
  case HolidayId::CoffeeDay:
    runCoffeeDayHolidayAnimation(display);
    break;
  case HolidayId::None:
    break;
  }
}

bool checkAndRunHolidayEasterEgg(MD_Parola &display) {
  if (PREVIEW_HOLIDAYS) {
    return false;
  }

  HolidayId holiday = getCurrentHoliday();
  if (holiday == HolidayId::None) {
    return false;
  }

  unsigned long now = millis();
  if (lastHolidayEggMs != 0 &&
      now - lastHolidayEggMs < HOLIDAY_EGG_INTERVAL_MS) {
    return false;
  }

  Serial.print("Holiday easter egg: ");
  Serial.println(static_cast<uint8_t>(holiday));

  runHolidayEasterEgg(display, holiday);
  lastHolidayEggMs = now;
  return true;
}
