#include "milestone_animations.h"

#include "milestone_hours_animations.h"
#include "milestone_subs_animations.h"
#include "milestone_views_animations.h"

#include <MD_MAX72xx.h>
#include <stdio.h>

void milestoneShowCenteredText(MD_Parola &display, const char *text,
                               uint16_t holdMs) {
  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setTextAlignment(PA_CENTER);
  display.print(text);
  delay(holdMs);
}

void milestoneRevealLabels(MD_Parola &display, const char *statLabel,
                           const char *increaseLabel) {
  milestoneShowCenteredText(display, statLabel, 900);
  milestoneShowCenteredText(display, increaseLabel, 1200);
}

void milestoneCleanupDisplay(MD_Parola &display) {
  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(0);
}

void milestoneGetIncreaseLabel(MilestoneTier tier, char *buffer, size_t size) {
  switch (tier) {
  case MilestoneTier::Tier100:
    snprintf(buffer, size, "+100!");
    break;
  case MilestoneTier::Tier1K:
    snprintf(buffer, size, "+1K!");
    break;
  case MilestoneTier::Tier10K:
    snprintf(buffer, size, "+10K!");
    break;
  case MilestoneTier::Tier100K:
    snprintf(buffer, size, "+100K!");
    break;
  case MilestoneTier::Tier1M:
    snprintf(buffer, size, "+1M!");
    break;
  case MilestoneTier::Tier10M:
    snprintf(buffer, size, "+10M!");
    break;
  }
}

static MilestoneTier tierFromAnimation(MilestoneAnimation animation) {
  return static_cast<MilestoneTier>(static_cast<uint8_t>(animation) % 6);
}

void runMilestoneAnimation(MD_Parola &display, MilestoneAnimation animation) {
  MilestoneTier tier = tierFromAnimation(animation);

  switch (animation) {
  case MilestoneAnimation::Subs100:
  case MilestoneAnimation::Subs1K:
  case MilestoneAnimation::Subs10K:
  case MilestoneAnimation::Subs100K:
  case MilestoneAnimation::Subs1M:
  case MilestoneAnimation::Subs10M:
    runSubsMilestoneAnimation(display, tier);
    break;

  case MilestoneAnimation::Views100:
  case MilestoneAnimation::Views1K:
  case MilestoneAnimation::Views10K:
  case MilestoneAnimation::Views100K:
  case MilestoneAnimation::Views1M:
  case MilestoneAnimation::Views10M:
    runViewsMilestoneAnimation(display, tier);
    break;

  case MilestoneAnimation::Hours100:
  case MilestoneAnimation::Hours1K:
  case MilestoneAnimation::Hours10K:
  case MilestoneAnimation::Hours100K:
  case MilestoneAnimation::Hours1M:
  case MilestoneAnimation::Hours10M:
    runHoursMilestoneAnimation(display, tier);
    break;
  }
}
