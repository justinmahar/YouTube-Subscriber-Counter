#ifndef MILESTONE_HELPERS_H
#define MILESTONE_HELPERS_H

#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <stdint.h>

struct MilestoneCtx {
  MD_Parola *display;
  MD_MAX72XX *matrix;
  uint16_t colStart;
  uint16_t colEnd;
  int width;
  int height;
  int cx;
  int cy;
};

void milestoneCtxInit(MD_Parola &display, MilestoneCtx &ctx);
void milestoneEffectBegin(MilestoneCtx &ctx, uint8_t intensity = 13);
void milestoneEffectEnd(MilestoneCtx &ctx);
void milestoneFrameShow(MilestoneCtx &ctx, uint16_t delayMs, uint8_t intensity = 15);

void milestoneClear(MilestoneCtx &ctx);
void milestoneFillAll(MilestoneCtx &ctx);
void milestoneDrawDiamondRing(MilestoneCtx &ctx, int ring);
void milestoneDrawExplosionRing(MilestoneCtx &ctx, int originX, int originY,
                              int ring);
void milestoneDrawFireworkBurst(MilestoneCtx &ctx, int originX, int originY,
                                int burstFrame, int particleCount, float speed);

void milestoneAnimInwardCollapse(MilestoneCtx &ctx, uint16_t frameDelayMs = 45);
void milestoneAnimVictoryPulses(MilestoneCtx &ctx, int pulseCount,
                                uint16_t onMs = 140, uint16_t offMs = 90);
void milestoneAnimSparkRain(MilestoneCtx &ctx, int frames,
                            uint16_t frameDelayMs = 38);
void milestoneAnimDualComets(MilestoneCtx &ctx, int passes,
                             uint16_t frameDelayMs = 28);
void milestoneAnimRockets(MilestoneCtx &ctx, int frames,
                          uint16_t frameDelayMs = 34);
void milestoneAnimMegablast(MilestoneCtx &ctx, int maxRing, uint16_t fastDelayMs,
                            uint16_t slowDelayMs, bool flashAfter = true);
void milestoneAnimFireworkBarrage(MilestoneCtx &ctx, const int *originX,
                                  const int *originY, int burstCount,
                                  int frames, int staggerFrames,
                                  int particleCount, float speed,
                                  uint16_t frameDelayMs);
void milestoneAnimChainExplosions(MilestoneCtx &ctx, const int *originX,
                                  int siteCount, int maxRing,
                                  uint16_t frameDelayMs, uint16_t sitePauseMs);
void milestoneAnimRippleBars(MilestoneCtx &ctx, int frames,
                             uint16_t frameDelayMs = 36);
void milestoneAnimDoubleDiamondPulse(MilestoneCtx &ctx, uint16_t frameDelayMs = 48);
void milestoneAnimTripleTsunami(MilestoneCtx &ctx, int framesPerWave,
                                uint16_t frameDelayMs = 42,
                                uint16_t wavePauseMs = 90);
void milestoneAnimCounterClimb(MilestoneCtx &ctx, uint16_t frameDelayMs = 32);
void milestoneAnimCounterShimmer(MilestoneCtx &ctx, int frames = 8,
                                 uint16_t frameDelayMs = 35);
void milestoneAnimSlowRingBuild(MilestoneCtx &ctx, int maxRing,
                                uint16_t frameDelayMs = 58);
void milestoneAnimShockwaveFlashes(MilestoneCtx &ctx, int flashCount,
                                   uint16_t flashMs = 55,
                                   uint16_t emberMs = 45);
void milestoneAnimEmberFallout(MilestoneCtx &ctx, int frames,
                               uint16_t frameDelayMs = 34);
void milestoneAnimFinalFlashes(MilestoneCtx &ctx, int flashCount,
                                uint16_t flashMs = 70, uint16_t gapMs = 45,
                                uint16_t lastFlashMs = 120);

#endif
