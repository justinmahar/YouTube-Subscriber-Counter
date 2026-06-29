#include "holiday_christmas.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setChristmasPixel(MilestoneCtx &ctx, int col, int row,
                              bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + col, on);
  }
}

static void drawSnowflake(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setChristmasPixel(ctx, cx, cy);
  setChristmasPixel(ctx, cx - 1, cy);
  setChristmasPixel(ctx, cx + 1, cy);
  setChristmasPixel(ctx, cx, cy - 1);
  setChristmasPixel(ctx, cx, cy + 1);
  if (wide) {
    setChristmasPixel(ctx, cx - 1, cy - 1);
    setChristmasPixel(ctx, cx + 1, cy - 1);
    setChristmasPixel(ctx, cx - 1, cy + 1);
    setChristmasPixel(ctx, cx + 1, cy + 1);
  }
}

static void drawTree(MilestoneCtx &ctx, int centerX, uint8_t revealRows,
                     uint8_t twinkleFrame) {
  static const uint8_t TREE_WIDTH = 17;
  static const uint8_t TREE_HEIGHT = 8;
  static const uint32_t TREE_ROWS[] = {
      0b00000000100000000, 0b00000001110000000, 0b00000011111000000,
      0b00000111111100000, 0b00001111111110000, 0b00011111111111000,
      0b00000001110000000, 0b00000011111000000,
  };

  int leftCol = centerX - TREE_WIDTH / 2;
  for (int row = 0; row < TREE_HEIGHT; row++) {
    if (row >= revealRows) {
      continue;
    }

    for (int col = 0; col < TREE_WIDTH; col++) {
      uint32_t mask = 1UL << (TREE_WIDTH - 1 - col);
      if ((TREE_ROWS[row] & mask) == 0) {
        continue;
      }

      bool ornament = row >= 2 && row <= 5 &&
                      ((row * 5 + col + twinkleFrame) % 9 == 0);
      if (!ornament || twinkleFrame % 2 == 0) {
        setChristmasPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawStar(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setChristmasPixel(ctx, cx, cy - 1);
  setChristmasPixel(ctx, cx - 1, cy);
  setChristmasPixel(ctx, cx, cy);
  setChristmasPixel(ctx, cx + 1, cy);
  setChristmasPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setChristmasPixel(ctx, cx - 2, cy);
    setChristmasPixel(ctx, cx + 2, cy);
    setChristmasPixel(ctx, cx, cy + 2);
  }
}

void runChristmasHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 7);

  struct Snowflake {
    int x;
    int y;
    bool wide;
  };

  Snowflake flakes[8];
  for (int i = 0; i < 8; i++) {
    flakes[i].x = (i * ctx.width) / 8 + (i % 2);
    flakes[i].y = -i;
    flakes[i].wide = i % 3 == 0;
  }

  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    for (int i = 0; i < 8; i++) {
      flakes[i].y++;
      flakes[i].x += (frame + i) % 4 == 0 ? 1 : 0;
      if (flakes[i].y > ctx.height + 1) {
        flakes[i].y = -2;
        flakes[i].x = random(ctx.width);
      }
      drawSnowflake(ctx, flakes[i].x, flakes[i].y, flakes[i].wide);
    }
    milestoneFrameShow(ctx, 55, 7 + frame % 4);
  }

  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawTree(ctx, ctx.cx, reveal, reveal);
    if (reveal >= 2) {
      drawStar(ctx, ctx.cx, 1, reveal);
    }
    milestoneFrameShow(ctx, 120, min(15, 7 + reveal));
  }

  for (int twinkle = 0; twinkle < 12; twinkle++) {
    milestoneClear(ctx);
    drawTree(ctx, ctx.cx, 8, twinkle);
    drawStar(ctx, ctx.cx, 1, twinkle);
    if (twinkle % 3 == 0) {
      drawSnowflake(ctx, 2 + twinkle, 1 + (twinkle % 5), twinkle % 2 == 0);
      drawSnowflake(ctx, ctx.width - 3 - twinkle / 2, 6 - (twinkle % 4),
                    twinkle % 2 == 1);
    }
    milestoneFrameShow(ctx, 105, twinkle % 2 == 0 ? 15 : 9);
  }

  for (int glow = 0; glow < 8; glow++) {
    milestoneClear(ctx);
    drawTree(ctx, ctx.cx, 8, glow);
    drawStar(ctx, ctx.cx, 1, glow);
    for (int row = 0; row < ctx.height; row++) {
      if ((row + glow) % 2 == 0) {
        setChristmasPixel(ctx, max(0, ctx.cx - 11 + row), row);
        setChristmasPixel(ctx, min(ctx.width - 1, ctx.cx + 11 - row), row);
      }
    }
    milestoneFrameShow(ctx, 55, min(15, 8 + glow));
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Merry Christmas!");
}
