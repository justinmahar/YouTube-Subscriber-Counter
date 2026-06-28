#include "milestone_hours_animations.h"

#include "milestone_helpers.h"

static void runHoursTier(MD_Parola &display, void (*animation)(MD_Parola &),
                         MilestoneTier tier, uint16_t statHoldMs,
                         uint16_t labelHoldMs) {
  animation(display);
  char increaseLabel[12];
  milestoneGetIncreaseLabel(tier, increaseLabel, sizeof(increaseLabel));
  milestoneShowCenteredText(display, "HOURS", statHoldMs);
  milestoneShowCenteredText(display, increaseLabel, labelHoldMs);
  milestoneCleanupDisplay(display);
}

// Placeholder — replace with a dedicated animation per tier.
static void hoursAnimPlaceholder(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx);
  milestoneAnimVictoryPulses(ctx, 2, 100, 70);
  milestoneEffectEnd(ctx);
}

void runHoursMilestoneAnimation(MD_Parola &display, MilestoneTier tier) {
  switch (tier) {
  case MilestoneTier::Tier100:
    runHoursTier(display, hoursAnimPlaceholder, tier, 750, 900);
    break;
  case MilestoneTier::Tier1K:
    runHoursTier(display, hoursAnimPlaceholder, tier, 850, 1000);
    break;
  case MilestoneTier::Tier10K:
    runHoursTier(display, hoursAnimPlaceholder, tier, 1100, 1300);
    break;
  case MilestoneTier::Tier100K:
    runHoursTier(display, hoursAnimPlaceholder, tier, 1200, 1500);
    break;
  case MilestoneTier::Tier1M:
    runHoursTier(display, hoursAnimPlaceholder, tier, 1300, 1600);
    break;
  case MilestoneTier::Tier10M:
    runHoursTier(display, hoursAnimPlaceholder, tier, 1400, 1800);
    break;
  default:
    break;
  }
}
