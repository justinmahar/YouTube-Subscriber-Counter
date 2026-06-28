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

// Hourglass half-width per row (8-row display).
static const int8_t HOURS_GLASS_HW[] = {1, 2, 3, 1, 1, 3, 2, 1};

static void hoursDrawHourglassOutline(MilestoneCtx &ctx) {
  for (int row = 0; row < ctx.height; row++) {
    int hw = HOURS_GLASS_HW[row];
    for (int col = ctx.cx - hw; col <= ctx.cx + hw; col++) {
      if (col < 0 || col >= ctx.width) {
        continue;
      }
      bool edge = (col == ctx.cx - hw || col == ctx.cx + hw);
      if (row == 0 || row == ctx.height - 1) {
        edge = true;
      }
      if (row == 3 || row == 4) {
        edge = (col == ctx.cx);
      }
      ctx.matrix->setPoint(row, ctx.colStart + col, edge);
    }
  }
}

static void hoursDrawSandFill(MilestoneCtx &ctx, int fillLevel) {
  for (int grain = 0; grain < fillLevel; grain++) {
    int row = 7 - grain / 3;
    int offset = grain % 3;
    int col = ctx.cx + offset - 1;
    if (row >= 5 && row < ctx.height && col >= ctx.cx - 2 && col <= ctx.cx + 2) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

// +100 hours — hourglass outline, sand grains fall, gentle ping, flash.
static void hoursAnimTier100(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 12);

  for (int frame = 0; frame < 6; frame++) {
    milestoneClear(ctx);
    hoursDrawHourglassOutline(ctx);
    if (frame >= 2) {
      hoursDrawSandFill(ctx, min(3, frame - 1));
    }
    milestoneFrameShow(ctx, 55, min(15, 8 + frame));
  }

  const int sandCols[] = {ctx.cx - 1, ctx.cx, ctx.cx + 1, ctx.cx, ctx.cx - 1, ctx.cx};
  int settled = 0;
  for (int grain = 0; grain < 6; grain++) {
    for (int row = 1; row <= 7; row++) {
      milestoneClear(ctx);
      hoursDrawHourglassOutline(ctx);
      hoursDrawSandFill(ctx, settled);
      int col = sandCols[grain];
      if (row <= 4) {
        ctx.matrix->setPoint(row, ctx.colStart + col, true);
      } else {
        ctx.matrix->setPoint(row, ctx.colStart + ctx.cx, row >= 6);
        if (row >= 5) {
          ctx.matrix->setPoint(row, ctx.colStart + col, true);
        }
      }
      milestoneFrameShow(ctx, row < 4 ? 38 : 48, 12);
    }
    settled++;
  }

  milestoneClear(ctx);
  hoursDrawHourglassOutline(ctx);
  hoursDrawSandFill(ctx, 6);
  milestoneFrameShow(ctx, 120, 13);

  for (int ring = 1; ring <= 3; ring++) {
    milestoneClear(ctx);
    hoursDrawHourglassOutline(ctx);
    hoursDrawSandFill(ctx, 6);
    milestoneDrawDiamondRing(ctx, ring + 3);
    milestoneFrameShow(ctx, 50, min(15, 9 + ring * 2));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 75, 15);
  milestoneEffectEnd(ctx);
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
    runHoursTier(display, hoursAnimTier100, tier, 750, 900);
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
