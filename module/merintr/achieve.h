// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * achieve.h:  Header file for achievement system UI.
 */

#ifndef _ACHIEVE_H
#define _ACHIEVE_H

#define MAX_ACHIEVEMENTS 64
#define MAX_ACHIEVE_NAME 128
#define MAX_ACHIEVE_DESC 256

#define ACHIEVE_CAT_ALL         0
#define ACHIEVE_CAT_EXPLORATION 1
#define ACHIEVE_CAT_COMBAT      2
#define ACHIEVE_CAT_PROGRESSION 3
#define ACHIEVE_CAT_SOCIAL      4
#define ACHIEVE_CAT_META        5

// Toast display duration in milliseconds.
#define ACHIEVE_TOAST_DURATION   4000
#define ACHIEVE_TOAST_TIMER_ID  9001

typedef struct {
   int  id;
   char name[MAX_ACHIEVE_NAME];
   char desc[MAX_ACHIEVE_DESC];
   BYTE earned;
   BYTE category;
   WORD progress;
   WORD target;
   WORD points;
   WORD rewardGold;
   WORD rewardItemCount;
   BYTE unclaimed;
} AchievementInfo;

typedef struct {
   int total;
   int earned;
   AchievementInfo achievements[MAX_ACHIEVEMENTS];
} AchievementData;

// Tracker overlay constants.
#define ACHIEVE_TRACKER_W       200
#define ACHIEVE_TRACKER_H       36
#define ACHIEVE_TRACKER_TIMER   9002

Bool HandleAchievementEarned(char *ptr, long len);
Bool HandleAchievements(char *ptr, long len);
void CommandAchievements(char *args);
void AchievementDialogShow(void);
void AchievementTrackerUpdate(void);
void AchievementTrackerDestroy(void);

#endif /* #ifndef _ACHIEVE_H */
