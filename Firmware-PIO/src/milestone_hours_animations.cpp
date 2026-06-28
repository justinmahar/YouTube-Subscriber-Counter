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

// Two triangles meeting at the pinch (row 3). Half-width per row, rows 0–7.
static const int8_t HOURS_GLASS_HW[] = {3, 2, 1, 0, 1, 2, 3, 3};

static const int8_t HOURS_TOP_SAND_R[] = {0, 0, 0, 0, 0, 1, 1, 1};
static const int8_t HOURS_TOP_SAND_DX[] = {-2, -1, 0, 1, 2, -1, 0, 1};
static const int8_t HOURS_BOT_SAND_R[] = {7, 7, 7, 7, 7, 6, 6, 6};
static const int8_t HOURS_BOT_SAND_DX[] = {-2, -1, 0, 1, 2, -1, 0, 1};
static const int HOURS_SAND_COUNT = 8;

static void hoursDrawHourglassFrameAt(MilestoneCtx &ctx, int ox, int revealThroughRow) {
  for (int row = 0; row < ctx.height && row <= revealThroughRow; row++) {
    int hw = HOURS_GLASS_HW[row];
    if (hw == 0) {
      if (ox >= 0 && ox < ctx.width) {
        ctx.matrix->setPoint(row, ctx.colStart + ox, true);
      }
      continue;
    }
    for (int c = ox - hw; c <= ox + hw; c++) {
      if (c < 0 || c >= ctx.width) {
        continue;
      }
      bool cap = (row == 0 || row == 7);
      bool edge = (c == ox - hw || c == ox + hw);
      if (cap || edge) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
  }
}

static void hoursDrawHourglassFrame(MilestoneCtx &ctx, int revealThroughRow) {
  hoursDrawHourglassFrameAt(ctx, ctx.cx, revealThroughRow);
}

static void hoursDrawClockTicks(MilestoneCtx &ctx) {
  static const int8_t TICK_DX[] = {-6, 6, -7, 7, -6, 6};
  static const int8_t TICK_ROW[] = {1, 1, 4, 4, 6, 6};
  for (int i = 0; i < 6; i++) {
    int col = ctx.cx + TICK_DX[i];
    int row = TICK_ROW[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

static void hoursDrawSandStream(MilestoneCtx &ctx, int headRow, int tailRow) {
  for (int row = tailRow; row <= headRow; row++) {
    if (row >= 0 && row < ctx.height && (row == headRow || row == tailRow)) {
      ctx.matrix->setPoint(row, ctx.colStart + ctx.cx, true);
    }
  }
}

static void hoursDrawTopSand(MilestoneCtx &ctx, int remaining) {
  int start = HOURS_SAND_COUNT - remaining;
  for (int i = start; i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_TOP_SAND_R[i];
    int col = ctx.cx + HOURS_TOP_SAND_DX[i];
    ctx.matrix->setPoint(row, ctx.colStart + col, true);
  }
}

static void hoursDrawBottomSand(MilestoneCtx &ctx, int count) {
  for (int i = 0; i < count && i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_BOT_SAND_R[i];
    int col = ctx.cx + HOURS_BOT_SAND_DX[i];
    ctx.matrix->setPoint(row, ctx.colStart + col, true);
  }
}

static void hoursDrawFallingGrainAt(MilestoneCtx &ctx, int ox, int row, int trailRow) {
  if (ox < 0 || ox >= ctx.width) {
    return;
  }
  ctx.matrix->setPoint(row, ctx.colStart + ox, true);
  if (trailRow >= 0 && trailRow < ctx.height && trailRow != row) {
    ctx.matrix->setPoint(trailRow, ctx.colStart + ox, true);
  }
}

static void hoursDrawFallingGrain(MilestoneCtx &ctx, int row, int trailRow) {
  hoursDrawFallingGrainAt(ctx, ctx.cx, row, trailRow);
}

static void hoursDrawFilledHourglass(MilestoneCtx &ctx, int topRemaining, int bottomCount) {
  hoursDrawClockTicks(ctx);
  hoursDrawTopSand(ctx, topRemaining);
  hoursDrawBottomSand(ctx, bottomCount);
  hoursDrawHourglassFrame(ctx, ctx.height - 1);
}

static void hoursDrawTopSandAt(MilestoneCtx &ctx, int ox, int remaining) {
  int start = HOURS_SAND_COUNT - remaining;
  for (int i = start; i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_TOP_SAND_R[i];
    int col = ox + HOURS_TOP_SAND_DX[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

static void hoursDrawBottomSandAt(MilestoneCtx &ctx, int ox, int count) {
  for (int i = 0; i < count && i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_BOT_SAND_R[i];
    int col = ox + HOURS_BOT_SAND_DX[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

static void hoursDrawDualHourglasses(MilestoneCtx &ctx, int leftOx, int rightOx,
                                     int topRemaining, int bottomCount) {
  hoursDrawTopSandAt(ctx, leftOx, topRemaining);
  hoursDrawTopSandAt(ctx, rightOx, topRemaining);
  hoursDrawBottomSandAt(ctx, leftOx, bottomCount);
  hoursDrawBottomSandAt(ctx, rightOx, bottomCount);
  hoursDrawHourglassFrameAt(ctx, leftOx, ctx.height - 1);
  hoursDrawHourglassFrameAt(ctx, rightOx, ctx.height - 1);
}

static void hoursAnimDualBuild(MilestoneCtx &ctx, int leftOx, int rightOx) {
  for (int row = 0; row < ctx.height; row++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrameAt(ctx, leftOx, row);
    hoursDrawHourglassFrameAt(ctx, rightOx, row);
    milestoneFrameShow(ctx, 40, min(15, 8 + row));
  }
}

static void hoursAnimDualFill(MilestoneCtx &ctx, int leftOx, int rightOx) {
  for (int top = 1; top <= HOURS_SAND_COUNT; top++) {
    milestoneClear(ctx);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, top, 0);
    milestoneFrameShow(ctx, top == HOURS_SAND_COUNT ? 100 : 38, min(15, 8 + top));
  }
}

static void hoursAnimDualDrain(MilestoneCtx &ctx, int leftOx, int rightOx) {
  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int grain = 0; grain < HOURS_SAND_COUNT; grain++) {
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      hoursDrawTopSandAt(ctx, leftOx, HOURS_SAND_COUNT - grain - 1);
      hoursDrawTopSandAt(ctx, rightOx, HOURS_SAND_COUNT - grain - 1);
      hoursDrawBottomSandAt(ctx, leftOx, grain);
      hoursDrawBottomSandAt(ctx, rightOx, grain);
      hoursDrawFallingGrainAt(ctx, leftOx, fallRows[step],
                              step > 0 ? fallRows[step - 1] : -1);
      hoursDrawFallingGrainAt(ctx, rightOx, fallRows[step],
                              step > 0 ? fallRows[step - 1] : -1);
      hoursDrawHourglassFrameAt(ctx, leftOx, ctx.height - 1);
      hoursDrawHourglassFrameAt(ctx, rightOx, ctx.height - 1);
      milestoneFrameShow(ctx, step < 2 ? 36 : 28, min(15, 10 + step));
    }
    milestoneClear(ctx);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, HOURS_SAND_COUNT - grain - 1,
                             grain + 1);
    milestoneFrameShow(ctx, 22, 12);
  }
}

static void hoursAnimDualFinale(MilestoneCtx &ctx, int leftOx, int rightOx) {
  milestoneClear(ctx);
  hoursDrawDualHourglasses(ctx, leftOx, rightOx, 0, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 120, 14);

  for (int ring = 1; ring <= 4; ring++) {
    milestoneClear(ctx);
    milestoneDrawExplosionRing(ctx, leftOx, ctx.cy, ring);
    milestoneDrawExplosionRing(ctx, rightOx, ctx.cy, ring);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, 0, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, 44, min(15, 9 + ring));
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, 0, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, pulse == 2 ? 90 : 60, pulse % 2 == 0 ? 15 : 11);
    if (pulse < 2) {
      milestoneClear(ctx);
      milestoneFrameShow(ctx, 30, 5);
    }
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 70, 15);
}

static void hoursAnimTickSweep(MilestoneCtx &ctx) {
  static const int8_t TICK_DX[] = {-6, 6, -7, 7, -6, 6};
  static const int8_t TICK_ROW[] = {1, 1, 4, 4, 6, 6};
  for (int i = 0; i < 6; i++) {
    milestoneClear(ctx);
    for (int t = 0; t < 6; t++) {
      int col = ctx.cx + TICK_DX[t];
      int row = TICK_ROW[t];
      if (col >= 0 && col < ctx.width && t <= i) {
        ctx.matrix->setPoint(row, ctx.colStart + col, true);
      }
    }
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 46, min(15, 9 + i));
  }
}

static void hoursAnimTimeRipple(MilestoneCtx &ctx, int bands) {
  for (int band = 0; band < bands; band++) {
    milestoneClear(ctx);
    hoursDrawTopSand(ctx, 0);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    hoursDrawClockTicks(ctx);
    for (int c = 0; c < ctx.width; c++) {
      if (abs(c - ctx.cx) > 6 + band) {
        continue;
      }
      for (int row = ctx.cy - band; row <= ctx.cy + band; row++) {
        if (row < 0 || row >= ctx.height) {
          continue;
        }
        bool edge = (row == ctx.cy - band || row == ctx.cy + band);
        if (edge || band == 0) {
          ctx.matrix->setPoint(row, ctx.colStart + c, true);
        }
      }
    }
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 50, min(15, 10 + band * 2));
  }
}

static void hoursAnimBottomSettle(MilestoneCtx &ctx) {
  for (int g = 0; g < HOURS_SAND_COUNT; g++) {
    milestoneClear(ctx);
    hoursDrawClockTicks(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    int row = HOURS_BOT_SAND_R[g];
    int col = ctx.cx + HOURS_BOT_SAND_DX[g];
    ctx.matrix->setPoint(row, ctx.colStart + col, true);
    ctx.matrix->setPoint(g % 2 == 0 ? 3 : 4, ctx.colStart + ctx.cx, true);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 42, min(15, 9 + g / 2));
  }
}

// +100 hours — hourglass builds, full top drains into bottom, ping, flash.
static void hoursAnimTier100(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 12);

  for (int row = 0; row < ctx.height; row++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, row);
    milestoneFrameShow(ctx, 45, min(15, 8 + row));
  }

  milestoneClear(ctx);
  hoursDrawHourglassFrame(ctx, ctx.height - 1);
  hoursDrawTopSand(ctx, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 120, 14);

  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int grain = 0; grain < HOURS_SAND_COUNT; grain++) {
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      hoursDrawTopSand(ctx, HOURS_SAND_COUNT - grain - 1);
      hoursDrawBottomSand(ctx, grain);
      hoursDrawFallingGrain(ctx, fallRows[step], step > 0 ? fallRows[step - 1] : -1);
      hoursDrawHourglassFrame(ctx, ctx.height - 1);
      milestoneFrameShow(ctx, step < 2 ? 42 : 34, min(15, 10 + step));
    }
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    hoursDrawTopSand(ctx, HOURS_SAND_COUNT - grain - 1);
    hoursDrawBottomSand(ctx, grain + 1);
    milestoneFrameShow(ctx, 26, 12);
  }

  milestoneClear(ctx);
  hoursDrawHourglassFrame(ctx, ctx.height - 1);
  hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 140, 14);

  for (int ring = 1; ring <= 3; ring++) {
    milestoneClear(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    milestoneDrawDiamondRing(ctx, ring + 4);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 48, min(15, 9 + ring * 2));
  }

  for (int pulse = 0; pulse < 2; pulse++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, pulse == 0 ? 90 : 55, pulse == 0 ? 14 : 15);
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 65, 15);
  milestoneEffectEnd(ctx);
}

// +1K hours — clock ticks, sand stream pour, ripple, spark rain, finale.
static void hoursAnimTier1K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 13);

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    if (pulse > 0) {
      hoursDrawClockTicks(ctx);
    }
    milestoneFrameShow(ctx, pulse == 2 ? 70 : 50, min(15, 10 + pulse * 2));
  }

  hoursAnimTickSweep(ctx);

  for (int top = 2; top <= HOURS_SAND_COUNT; top += 2) {
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, top, 0);
    milestoneFrameShow(ctx, 36, min(15, 9 + top / 2));
  }

  milestoneClear(ctx);
  hoursDrawFilledHourglass(ctx, HOURS_SAND_COUNT, 0);
  milestoneFrameShow(ctx, 100, 14);

  for (int tick = 0; tick < 4; tick++) {
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, HOURS_SAND_COUNT, 0);
    hoursDrawClockTicks(ctx);
    milestoneFrameShow(ctx, tick == 3 ? 80 : 55, tick % 2 == 0 ? 14 : 11);
  }

  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int batch = 0; batch < HOURS_SAND_COUNT / 2; batch++) {
    int grainsDone = batch * 2;
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      hoursDrawClockTicks(ctx);
      hoursDrawTopSand(ctx, HOURS_SAND_COUNT - grainsDone - 2);
      hoursDrawBottomSand(ctx, grainsDone);
      hoursDrawSandStream(ctx, fallRows[step], max(1, fallRows[step] - 2));
      if (step == 2) {
        ctx.matrix->setPoint(3, ctx.colStart + ctx.cx - 1, true);
        ctx.matrix->setPoint(4, ctx.colStart + ctx.cx, true);
      }
      if (step == 3) {
        ctx.matrix->setPoint(3, ctx.colStart + ctx.cx + 1, true);
        ctx.matrix->setPoint(4, ctx.colStart + ctx.cx, true);
      }
      hoursDrawHourglassFrame(ctx, ctx.height - 1);
      milestoneFrameShow(ctx, 30, min(15, 11 + step));
    }
    grainsDone += 2;
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, HOURS_SAND_COUNT - grainsDone, grainsDone);
    milestoneFrameShow(ctx, 26, 12);
  }

  milestoneClear(ctx);
  hoursDrawFilledHourglass(ctx, 0, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 110, 14);

  hoursAnimTimeRipple(ctx, 4);
  hoursAnimBottomSettle(ctx);

  milestoneClear(ctx);
  hoursDrawFilledHourglass(ctx, 0, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 90, 14);

  for (int ring = 1; ring <= 5; ring++) {
    milestoneClear(ctx);
    hoursDrawClockTicks(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    milestoneDrawDiamondRing(ctx, ring + 3);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 46, min(15, 9 + ring));
  }

  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    hoursDrawClockTicks(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    for (int c = 0; c < ctx.width; c++) {
      if (abs(c - ctx.cx) <= 4) {
        continue;
      }
      int sparkRow = (frame * 2 + c * 5) % ctx.height;
      if (frame % 2 == 0) {
        ctx.matrix->setPoint(sparkRow, ctx.colStart + c, true);
      }
      if (frame % 7 == 0 && c == 1) {
        for (int trail = 0; trail < 3; trail++) {
          int row = (sparkRow + trail) % ctx.height;
          if (trail < 2) {
            ctx.matrix->setPoint(row, ctx.colStart + c, true);
          }
        }
      }
    }
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 32, min(15, 7 + frame / 4));
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, 0, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, pulse == 2 ? 90 : 65, pulse % 2 == 0 ? 15 : 12);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, 35, 5);
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 75, 15);
  milestoneEffectEnd(ctx);
}

// +10K hours — twin hourglasses build, fill, drain in sync, celebration.
static void hoursAnimTier10K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 14);

  const int leftOx = ctx.cx - 9;
  const int rightOx = ctx.cx + 9;

  hoursAnimDualBuild(ctx, leftOx, rightOx);
  hoursAnimDualFill(ctx, leftOx, rightOx);
  hoursAnimDualDrain(ctx, leftOx, rightOx);
  hoursAnimDualFinale(ctx, leftOx, rightOx);
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
    runHoursTier(display, hoursAnimTier1K, tier, 850, 1000);
    break;
  case MilestoneTier::Tier10K:
    runHoursTier(display, hoursAnimTier10K, tier, 1100, 1300);
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
