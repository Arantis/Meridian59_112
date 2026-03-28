// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * achieve.c:  Achievement system client UI.
 */

#include "client.h"
#include "merintr.h"
#include "achieve.h"
#include <mmsystem.h>

// Colors for achievement states.
#define ACHIEVE_COLOR_EARNED     RGB(34, 139, 34)
#define ACHIEVE_COLOR_PROGRESS   RGB(180, 140, 20)
#define ACHIEVE_COLOR_LOCKED     RGB(120, 120, 120)
#define ACHIEVE_COLOR_EARNED_BG  RGB(232, 245, 232)
#define ACHIEVE_COLOR_UNCLAIMED  RGB(255, 215, 0)    // Gold for claimable
#define ACHIEVE_COLOR_UNCLAIM_BG RGB(255, 250, 220)  // Light gold tint
#define ACHIEVE_COLOR_SELECT_BG  RGB(51, 102, 153)
#define ACHIEVE_COLOR_SELECT_FG  RGB(255, 255, 255)

// Toast colors.
#define TOAST_BG                 RGB(30, 30, 30)
#define TOAST_BORDER             RGB(34, 139, 34)
#define TOAST_TEXT_TITLE          RGB(255, 215, 0)
#define TOAST_TEXT_NAME           RGB(255, 255, 255)
#define TOAST_TEXT_POINTS         RGB(34, 139, 34)

// Tracker colors.
#define TRACKER_BG               RGB(20, 20, 20)
#define TRACKER_BORDER           RGB(80, 80, 80)
#define TRACKER_TEXT             RGB(200, 200, 200)
#define TRACKER_BAR_BG           RGB(50, 50, 50)
#define TRACKER_BAR_FG           RGB(34, 139, 34)

static AchievementData achieveData;
static HWND hAchieveDialog = NULL;
static HWND hToastWnd = NULL;
static HWND hTrackerWnd = NULL;
static BYTE currentFilter = ACHIEVE_CAT_ALL;
static char searchFilter[64] = "";

// Maps listbox index to achieveData.achievements[] index.
static int listToAchieve[MAX_ACHIEVEMENTS];
static int listCount = 0;

// Toast data.
static char toastName[MAX_ACHIEVE_NAME];
static WORD toastPoints = 0;

// Tracker data: ID of tracked achievement (-1 = none).
static int trackedAchievement = -1;

static INT_PTR CALLBACK AchievementDialogProc(HWND hDlg, UINT message,
                                               WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK ToastWndProc(HWND hWnd, UINT message,
                                      WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK TrackerWndProc(HWND hWnd, UINT message,
                                        WPARAM wParam, LPARAM lParam);

static const char *GetCategoryName(BYTE category);
static void PopulateAchievementList(HWND hDlg, BYTE filterCat);
static void FormatItemText(int ai, char *buf, int bufSize);
static void ShowAchievementToast(const char *name, WORD points);
static int  GetTotalEarnedPoints(void);
static int  GetTotalMaxPoints(void);
static BOOL MatchesSearch(int ai, const char *search);
static void FormatDescText(int ai, char *buf, int bufSize);

/************************************************************************/
static const char *GetCategoryName(BYTE category)
{
   switch (category)
   {
   case ACHIEVE_CAT_EXPLORATION: return "Exploration";
   case ACHIEVE_CAT_COMBAT:      return "Combat";
   case ACHIEVE_CAT_PROGRESSION: return "Progression";
   case ACHIEVE_CAT_SOCIAL:      return "Social";
   case ACHIEVE_CAT_META:        return "Meta";
   default:                      return "Other";
   }
}

/************************************************************************/
static int GetTotalEarnedPoints(void)
{
   int i, total = 0;
   for (i = 0; i < achieveData.total; i++)
   {
      if (achieveData.achievements[i].earned)
         total += achieveData.achievements[i].points;
   }
   return total;
}

/************************************************************************/
static int GetTotalMaxPoints(void)
{
   int i, total = 0;
   for (i = 0; i < achieveData.total; i++)
      total += achieveData.achievements[i].points;
   return total;
}

/************************************************************************/
static BOOL MatchesSearch(int ai, const char *search)
{
   char nameLower[MAX_ACHIEVE_NAME];
   char descLower[MAX_ACHIEVE_DESC];
   char searchLower[64];
   int i;

   if (search[0] == '\0')
      return TRUE;

   // Case-insensitive search in name and description.
   for (i = 0; achieveData.achievements[ai].name[i] && i < MAX_ACHIEVE_NAME - 1; i++)
      nameLower[i] = (char)tolower(achieveData.achievements[ai].name[i]);
   nameLower[i] = '\0';

   for (i = 0; achieveData.achievements[ai].desc[i] && i < MAX_ACHIEVE_DESC - 1; i++)
      descLower[i] = (char)tolower(achieveData.achievements[ai].desc[i]);
   descLower[i] = '\0';

   for (i = 0; search[i] && i < 63; i++)
      searchLower[i] = (char)tolower(search[i]);
   searchLower[i] = '\0';

   return (strstr(nameLower, searchLower) != NULL
           || strstr(descLower, searchLower) != NULL);
}

/************************************************************************/
static void FormatItemText(int ai, char *buf, int bufSize)
{
   if (achieveData.achievements[ai].unclaimed)
      sprintf(buf, "  %s  [CLAIM!]  +%dpt",
              achieveData.achievements[ai].name,
              achieveData.achievements[ai].points);
   else if (achieveData.achievements[ai].earned)
      sprintf(buf, "  %s  (+%dpt)",
              achieveData.achievements[ai].name,
              achieveData.achievements[ai].points);
   else if (achieveData.achievements[ai].target > 0)
      sprintf(buf, "  %s  [%d/%d]  %dpt",
              achieveData.achievements[ai].name,
              achieveData.achievements[ai].progress,
              achieveData.achievements[ai].target,
              achieveData.achievements[ai].points);
   else
      sprintf(buf, "  %s  %dpt",
              achieveData.achievements[ai].name,
              achieveData.achievements[ai].points);
}

/************************************************************************/
static void FormatDescText(int ai, char *buf, int bufSize)
{
   int len;

   // Base description.
   if (achieveData.achievements[ai].target > 0
       && !achieveData.achievements[ai].earned)
      len = sprintf(buf, "%s (%d/%d)",
                    achieveData.achievements[ai].desc,
                    achieveData.achievements[ai].progress,
                    achieveData.achievements[ai].target);
   else
      len = sprintf(buf, "%s", achieveData.achievements[ai].desc);

   // Append reward info.
   if (achieveData.achievements[ai].rewardGold > 0
       && achieveData.achievements[ai].rewardItemCount > 0)
      sprintf(buf + len, "  |  Reward: %d gold, %d items",
              achieveData.achievements[ai].rewardGold,
              achieveData.achievements[ai].rewardItemCount);
   else if (achieveData.achievements[ai].rewardGold > 0)
      sprintf(buf + len, "  |  Reward: %d gold",
              achieveData.achievements[ai].rewardGold);
   else if (achieveData.achievements[ai].rewardItemCount > 0)
      sprintf(buf + len, "  |  Reward: %d items",
              achieveData.achievements[ai].rewardItemCount);
}

/************************************************************************/
static void PopulateAchievementList(HWND hDlg, BYTE filterCat)
{
   HWND hList = GetDlgItem(hDlg, IDC_ACHIEVELIST);
   int i;
   char itemText[512];
   char header[128];
   int filteredEarned = 0;
   int filteredTotal = 0;

   SendMessage(hList, LB_RESETCONTENT, 0, 0);
   listCount = 0;

   for (i = 0; i < achieveData.total; i++)
   {
      int idx;

      if (filterCat != ACHIEVE_CAT_ALL
          && achieveData.achievements[i].category != filterCat)
         continue;

      if (!MatchesSearch(i, searchFilter))
         continue;

      filteredTotal++;
      if (achieveData.achievements[i].earned)
         filteredEarned++;

      FormatItemText(i, itemText, sizeof(itemText));

      idx = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)itemText);
      SendMessage(hList, LB_SETITEMDATA, idx, i);
      listToAchieve[listCount] = i;
      listCount++;
   }

   // Header with score.
   {
      int earnedPoints = GetTotalEarnedPoints();
      int maxPoints = GetTotalMaxPoints();
      if (filterCat == ACHIEVE_CAT_ALL)
         sprintf(header, "Achievements: %d / %d    Score: %d / %d",
                 achieveData.earned, achieveData.total,
                 earnedPoints, maxPoints);
      else
         sprintf(header, "%s: %d / %d    Score: %d / %d",
                 GetCategoryName(filterCat), filteredEarned, filteredTotal,
                 earnedPoints, maxPoints);
   }

   SetDlgItemText(hDlg, IDC_ACHIEVEHEADER, header);

   // Select first item.
   if (listCount > 0)
   {
      char descText[512];
      int ai = listToAchieve[0];

      SendMessage(hList, LB_SETCURSEL, 0, 0);

      FormatDescText(ai, descText, sizeof(descText));
      SetDlgItemText(hDlg, IDC_ACHIEVEDESC, descText);
   }
   else
   {
      SetDlgItemText(hDlg, IDC_ACHIEVEDESC, "");
   }
}

/************************************************************************/
/*
 * Toast notification.
 */
static void ShowAchievementToast(const char *name, WORD points)
{
   WNDCLASS wc;
   HWND hParent;
   RECT rcParent;
   int toastW = 280, toastH = 60;
   int x, y;

   if (hToastWnd != NULL)
   {
      DestroyWindow(hToastWnd);
      hToastWnd = NULL;
   }

   strncpy(toastName, name, MAX_ACHIEVE_NAME - 1);
   toastName[MAX_ACHIEVE_NAME - 1] = '\0';
   toastPoints = points;

   wc.style = 0;
   wc.lpfnWndProc = ToastWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = hInst;
   wc.hIcon = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "AchieveToast";
   RegisterClass(&wc);

   hParent = cinfo->hMain;
   GetWindowRect(hParent, &rcParent);
   x = rcParent.left + (rcParent.right - rcParent.left - toastW) / 2;
   y = rcParent.top + 20;

   hToastWnd = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
      "AchieveToast", NULL, WS_POPUP,
      x, y, toastW, toastH,
      hParent, NULL, hInst, NULL);

   if (hToastWnd != NULL)
   {
      ShowWindow(hToastWnd, SW_SHOWNOACTIVATE);
      UpdateWindow(hToastWnd);
      SetTimer(hToastWnd, ACHIEVE_TOAST_TIMER_ID, ACHIEVE_TOAST_DURATION, NULL);
   }

   PlaySound("SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
}

/************************************************************************/
static LRESULT CALLBACK ToastWndProc(HWND hWnd, UINT message,
                                      WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_PAINT:
   {
      PAINTSTRUCT ps;
      HDC hdc;
      RECT rc, rcText;
      HBRUSH hBgBrush, hBorderBrush;
      HFONT hOldFont;
      char pointsText[64];

      hdc = BeginPaint(hWnd, &ps);
      GetClientRect(hWnd, &rc);

      hBgBrush = CreateSolidBrush(TOAST_BG);
      FillRect(hdc, &rc, hBgBrush);
      DeleteObject(hBgBrush);

      hBorderBrush = CreateSolidBrush(TOAST_BORDER);
      FrameRect(hdc, &rc, hBorderBrush);
      rc.left++; rc.top++; rc.right--; rc.bottom--;
      FrameRect(hdc, &rc, hBorderBrush);
      DeleteObject(hBorderBrush);

      SetBkMode(hdc, TRANSPARENT);
      hOldFont = (HFONT)SelectObject(hdc, GetFont(FONT_LIST));

      rcText = rc;
      rcText.left += 10;
      rcText.top += 6;
      SetTextColor(hdc, TOAST_TEXT_TITLE);
      DrawText(hdc, "Achievement Unlocked!", -1, &rcText,
               DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

      rcText.top += 20;
      SetTextColor(hdc, TOAST_TEXT_NAME);
      DrawText(hdc, toastName, -1, &rcText,
               DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

      sprintf(pointsText, "+%d pt", toastPoints);
      rcText.right -= 10;
      SetTextColor(hdc, TOAST_TEXT_POINTS);
      DrawText(hdc, pointsText, -1, &rcText,
               DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);

      SelectObject(hdc, hOldFont);
      EndPaint(hWnd, &ps);
      return 0;
   }

   case WM_TIMER:
      if (wParam == ACHIEVE_TOAST_TIMER_ID)
      {
         KillTimer(hWnd, ACHIEVE_TOAST_TIMER_ID);
         DestroyWindow(hWnd);
         hToastWnd = NULL;
         return 0;
      }
      break;

   case WM_LBUTTONDOWN:
      KillTimer(hWnd, ACHIEVE_TOAST_TIMER_ID);
      DestroyWindow(hWnd);
      hToastWnd = NULL;
      return 0;
   }

   return DefWindowProc(hWnd, message, wParam, lParam);
}

/************************************************************************/
/*
 * Tracker overlay - persistent small window showing tracked achievement.
 */
static void ShowTracker(void)
{
   WNDCLASS wc;
   HWND hParent;
   RECT rcParent;
   int x, y;

   if (hTrackerWnd != NULL)
   {
      InvalidateRect(hTrackerWnd, NULL, TRUE);
      return;
   }

   wc.style = 0;
   wc.lpfnWndProc = TrackerWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = hInst;
   wc.hIcon = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "AchieveTracker";
   RegisterClass(&wc);

   hParent = cinfo->hMain;
   GetWindowRect(hParent, &rcParent);
   x = rcParent.right - ACHIEVE_TRACKER_W - 10;
   y = rcParent.bottom - ACHIEVE_TRACKER_H - 30;

   hTrackerWnd = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
      "AchieveTracker", NULL, WS_POPUP,
      x, y, ACHIEVE_TRACKER_W, ACHIEVE_TRACKER_H,
      hParent, NULL, hInst, NULL);

   if (hTrackerWnd != NULL)
   {
      ShowWindow(hTrackerWnd, SW_SHOWNOACTIVATE);
      UpdateWindow(hTrackerWnd);
   }
}

void AchievementTrackerUpdate(void)
{
   if (hTrackerWnd != NULL && trackedAchievement >= 0)
      InvalidateRect(hTrackerWnd, NULL, TRUE);
}

void AchievementTrackerDestroy(void)
{
   if (hTrackerWnd != NULL)
   {
      DestroyWindow(hTrackerWnd);
      hTrackerWnd = NULL;
   }
   trackedAchievement = -1;
}

static LRESULT CALLBACK TrackerWndProc(HWND hWnd, UINT message,
                                        WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_PAINT:
   {
      PAINTSTRUCT ps;
      HDC hdc;
      RECT rc, rcBar;
      HBRUSH hBrush;
      HFONT hOldFont;
      int ai = -1;
      int i;
      char text[256];

      hdc = BeginPaint(hWnd, &ps);
      GetClientRect(hWnd, &rc);

      // Background.
      hBrush = CreateSolidBrush(TRACKER_BG);
      FillRect(hdc, &rc, hBrush);
      DeleteObject(hBrush);

      // Border.
      hBrush = CreateSolidBrush(TRACKER_BORDER);
      FrameRect(hdc, &rc, hBrush);
      DeleteObject(hBrush);

      // Find tracked achievement in data.
      for (i = 0; i < achieveData.total; i++)
      {
         if (achieveData.achievements[i].id == trackedAchievement)
         {
            ai = i;
            break;
         }
      }

      if (ai >= 0)
      {
         SetBkMode(hdc, TRANSPARENT);
         hOldFont = (HFONT)SelectObject(hdc, GetFont(FONT_LIST));

         // Achievement name.
         SetTextColor(hdc, TRACKER_TEXT);

         if (achieveData.achievements[ai].earned)
            sprintf(text, "%s  [Complete]", achieveData.achievements[ai].name);
         else if (achieveData.achievements[ai].target > 0)
            sprintf(text, "%s  %d/%d",
                    achieveData.achievements[ai].name,
                    achieveData.achievements[ai].progress,
                    achieveData.achievements[ai].target);
         else
            sprintf(text, "%s", achieveData.achievements[ai].name);

         rc.left += 6;
         rc.top += 4;
         DrawText(hdc, text, -1, &rc,
                  DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

         // Progress bar for progress-based achievements.
         if (achieveData.achievements[ai].target > 0
             && !achieveData.achievements[ai].earned)
         {
            int barW, fillW;

            rcBar.left = 6;
            rcBar.top = 22;
            rcBar.right = ACHIEVE_TRACKER_W - 6;
            rcBar.bottom = 30;
            barW = rcBar.right - rcBar.left;

            // Bar background.
            hBrush = CreateSolidBrush(TRACKER_BAR_BG);
            FillRect(hdc, &rcBar, hBrush);
            DeleteObject(hBrush);

            // Bar fill.
            fillW = (int)((long)barW
                    * achieveData.achievements[ai].progress
                    / achieveData.achievements[ai].target);
            if (fillW > 0)
            {
               rcBar.right = rcBar.left + fillW;
               hBrush = CreateSolidBrush(TRACKER_BAR_FG);
               FillRect(hdc, &rcBar, hBrush);
               DeleteObject(hBrush);
            }
         }

         SelectObject(hdc, hOldFont);
      }

      EndPaint(hWnd, &ps);
      return 0;
   }

   case WM_LBUTTONDOWN:
      // Click to dismiss tracker.
      DestroyWindow(hWnd);
      hTrackerWnd = NULL;
      trackedAchievement = -1;
      return 0;
   }

   return DefWindowProc(hWnd, message, wParam, lParam);
}

/************************************************************************/
Bool HandleAchievementEarned(char *ptr, long len)
{
   int achievement_id;
   char name[MAX_ACHIEVE_NAME];
   char desc[MAX_ACHIEVE_DESC];

   if (len < SIZE_ID)
      return False;

   Extract(&ptr, &achievement_id, SIZE_ID);
   len -= SIZE_ID;

   len = ExtractString(&ptr, len, name, MAX_ACHIEVE_NAME);
   if (len == (WORD)-1)
      return False;

   len = ExtractString(&ptr, len, desc, MAX_ACHIEVE_DESC);
   if (len == (WORD)-1)
      return False;

   // Update local cache.
   {
      int i;
      WORD points = 0;

      for (i = 0; i < achieveData.total; i++)
      {
         if (achieveData.achievements[i].id == achievement_id)
         {
            achieveData.achievements[i].earned = 1;
            achieveData.earned++;
            points = achieveData.achievements[i].points;
            break;
         }
      }

      ShowAchievementToast(name, points);
   }

   // Refresh dialog if open.
   if (hAchieveDialog != NULL)
      PopulateAchievementList(hAchieveDialog, currentFilter);

   // Update tracker if tracking this or any related achievement.
   AchievementTrackerUpdate();

   return True;
}

/************************************************************************/
Bool HandleAchievements(char *ptr, long len)
{
   WORD total, earned;
   int i;

   if (len < 4)
      return False;

   Extract(&ptr, &total, SIZE_LIST_LEN);
   len -= SIZE_LIST_LEN;
   Extract(&ptr, &earned, SIZE_LIST_LEN);
   len -= SIZE_LIST_LEN;

   if (total > MAX_ACHIEVEMENTS)
      total = MAX_ACHIEVEMENTS;

   achieveData.total = total;
   achieveData.earned = earned;

   for (i = 0; i < total; i++)
   {
      Extract(&ptr, &achieveData.achievements[i].id, SIZE_ID);
      len -= SIZE_ID;

      len = ExtractString(&ptr, len, achieveData.achievements[i].name,
                          MAX_ACHIEVE_NAME);
      if (len == (WORD)-1)
         return False;

      len = ExtractString(&ptr, len, achieveData.achievements[i].desc,
                          MAX_ACHIEVE_DESC);
      if (len == (WORD)-1)
         return False;

      Extract(&ptr, &achieveData.achievements[i].earned, 1);
      len -= 1;

      Extract(&ptr, &achieveData.achievements[i].category, 1);
      len -= 1;

      Extract(&ptr, &achieveData.achievements[i].progress, SIZE_LIST_LEN);
      len -= SIZE_LIST_LEN;
      Extract(&ptr, &achieveData.achievements[i].target, SIZE_LIST_LEN);
      len -= SIZE_LIST_LEN;

      Extract(&ptr, &achieveData.achievements[i].points, SIZE_LIST_LEN);
      len -= SIZE_LIST_LEN;

      Extract(&ptr, &achieveData.achievements[i].rewardGold, SIZE_LIST_LEN);
      len -= SIZE_LIST_LEN;
      Extract(&ptr, &achieveData.achievements[i].rewardItemCount, SIZE_LIST_LEN);
      len -= SIZE_LIST_LEN;

      Extract(&ptr, &achieveData.achievements[i].unclaimed, 1);
      len -= 1;
   }

   currentFilter = ACHIEVE_CAT_ALL;
   searchFilter[0] = '\0';

   AchievementDialogShow();

   return True;
}

/************************************************************************/
void CommandAchievements(char *args)
{
   RequestAchievements();
}

/************************************************************************/
void AchievementDialogShow(void)
{
   HWND hDlg;

   if (hAchieveDialog != NULL)
   {
      SetForegroundWindow(hAchieveDialog);
      return;
   }

   hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ACHIEVEMENTS),
                            cinfo->hMain, AchievementDialogProc, 0);
   if (hDlg != NULL)
      ShowWindow(hDlg, SW_SHOW);
}

/************************************************************************/
static void DrawAchievementItem(DRAWITEMSTRUCT *dis)
{
   int ai;
   COLORREF textColor, bgColor;
   HBRUSH hBrush;
   RECT rcText, rcIcon;
   char text[512];
   BOOL selected;

   if (dis->itemID == (UINT)-1)
      return;

   ai = (int)dis->itemData;
   if (ai < 0 || ai >= achieveData.total)
      return;

   selected = (dis->itemState & ODS_SELECTED) != 0;

   if (selected)
   {
      textColor = ACHIEVE_COLOR_SELECT_FG;
      bgColor = ACHIEVE_COLOR_SELECT_BG;
   }
   else if (achieveData.achievements[ai].unclaimed)
   {
      textColor = ACHIEVE_COLOR_UNCLAIMED;
      bgColor = ACHIEVE_COLOR_UNCLAIM_BG;
   }
   else if (achieveData.achievements[ai].earned)
   {
      textColor = ACHIEVE_COLOR_EARNED;
      bgColor = ACHIEVE_COLOR_EARNED_BG;
   }
   else if (achieveData.achievements[ai].target > 0
            && achieveData.achievements[ai].progress > 0)
   {
      textColor = ACHIEVE_COLOR_PROGRESS;
      bgColor = GetSysColor(COLOR_WINDOW);
   }
   else
   {
      textColor = ACHIEVE_COLOR_LOCKED;
      bgColor = GetSysColor(COLOR_WINDOW);
   }

   hBrush = CreateSolidBrush(bgColor);
   FillRect(dis->hDC, &dis->rcItem, hBrush);
   DeleteObject(hBrush);

   rcIcon = dis->rcItem;
   rcIcon.right = rcIcon.left + 18;
   rcText = dis->rcItem;
   rcText.left += 20;

   SetBkMode(dis->hDC, TRANSPARENT);
   SetTextColor(dis->hDC, textColor);

   if (achieveData.achievements[ai].earned)
   {
      RECT rcCheck;
      HBRUSH hCheckBrush;
      HPEN hPen, hOldPen;
      int cy = (rcIcon.top + rcIcon.bottom) / 2;

      rcCheck.left = rcIcon.left + 4;
      rcCheck.top = cy - 5;
      rcCheck.right = rcCheck.left + 10;
      rcCheck.bottom = cy + 5;

      hCheckBrush = CreateSolidBrush(selected ?
                       ACHIEVE_COLOR_SELECT_FG : ACHIEVE_COLOR_EARNED);
      FillRect(dis->hDC, &rcCheck, hCheckBrush);
      DeleteObject(hCheckBrush);

      hPen = CreatePen(PS_SOLID, 2, selected ?
                 ACHIEVE_COLOR_SELECT_BG : RGB(255, 255, 255));
      hOldPen = (HPEN)SelectObject(dis->hDC, hPen);
      MoveToEx(dis->hDC, rcCheck.left + 2, cy - 1, NULL);
      LineTo(dis->hDC, rcCheck.left + 4, cy + 2);
      LineTo(dis->hDC, rcCheck.right - 2, cy - 3);
      SelectObject(dis->hDC, hOldPen);
      DeleteObject(hPen);
   }
   else
   {
      HPEN hPen, hOldPen;
      HBRUSH hOldBrush;
      RECT rcCheck;
      int cy = (rcIcon.top + rcIcon.bottom) / 2;

      rcCheck.left = rcIcon.left + 4;
      rcCheck.top = cy - 5;
      rcCheck.right = rcCheck.left + 10;
      rcCheck.bottom = cy + 5;

      hPen = CreatePen(PS_SOLID, 1, textColor);
      hOldPen = (HPEN)SelectObject(dis->hDC, hPen);
      hOldBrush = (HBRUSH)SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
      Rectangle(dis->hDC, rcCheck.left, rcCheck.top,
                rcCheck.right, rcCheck.bottom);
      SelectObject(dis->hDC, hOldPen);
      SelectObject(dis->hDC, hOldBrush);
      DeleteObject(hPen);

      if (achieveData.achievements[ai].target > 0
          && achieveData.achievements[ai].progress > 0)
      {
         RECT rcFill;
         HBRUSH hFillBrush;
         int fillWidth;

         fillWidth = (int)((rcCheck.right - rcCheck.left - 2)
                     * achieveData.achievements[ai].progress
                     / achieveData.achievements[ai].target);

         rcFill.left = rcCheck.left + 1;
         rcFill.top = rcCheck.top + 1;
         rcFill.right = rcFill.left + fillWidth;
         rcFill.bottom = rcCheck.bottom - 1;

         hFillBrush = CreateSolidBrush(selected ?
                         ACHIEVE_COLOR_SELECT_FG : ACHIEVE_COLOR_PROGRESS);
         FillRect(dis->hDC, &rcFill, hFillBrush);
         DeleteObject(hFillBrush);
      }
   }

   // Highlight tracked achievement with a small marker.
   if (achieveData.achievements[ai].id == trackedAchievement)
   {
      RECT rcMark;
      HBRUSH hMarkBrush;
      rcMark.left = dis->rcItem.right - 6;
      rcMark.top = dis->rcItem.top + 2;
      rcMark.right = dis->rcItem.right - 2;
      rcMark.bottom = dis->rcItem.bottom - 2;
      hMarkBrush = CreateSolidBrush(selected ?
                      ACHIEVE_COLOR_SELECT_FG : TOAST_TEXT_TITLE);
      FillRect(dis->hDC, &rcMark, hMarkBrush);
      DeleteObject(hMarkBrush);
   }

   SendMessage(dis->hwndItem, LB_GETTEXT, dis->itemID, (LPARAM)text);
   rcText.top += 1;
   DrawText(dis->hDC, text, -1, &rcText,
            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

   if (dis->itemState & ODS_FOCUS)
      DrawFocusRect(dis->hDC, &dis->rcItem);
}

/************************************************************************/
static INT_PTR CALLBACK AchievementDialogProc(HWND hDlg, UINT message,
                                               WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_INITDIALOG:
   {
      HWND hList, hCat, hSearch;
      BYTE categories[16];
      int numCats = 0;
      int i, j;
      BOOL found;

      hAchieveDialog = hDlg;
      hList = GetDlgItem(hDlg, IDC_ACHIEVELIST);
      hCat = GetDlgItem(hDlg, IDC_ACHIEVECAT);
      hSearch = GetDlgItem(hDlg, IDC_ACHIEVESEARCH);

      SetWindowFont(hList, GetFont(FONT_LIST), FALSE);

      // Search placeholder hint via cue banner.
      SendMessage(hSearch, EM_SETCUEBANNER, 0, (LPARAM)L"Search...");

      // Build category combobox.
      SendMessage(hCat, CB_ADDSTRING, 0, (LPARAM)"All");
      SendMessage(hCat, CB_SETITEMDATA, 0, ACHIEVE_CAT_ALL);

      for (i = 0; i < achieveData.total; i++)
      {
         found = FALSE;
         for (j = 0; j < numCats; j++)
         {
            if (categories[j] == achieveData.achievements[i].category)
            {
               found = TRUE;
               break;
            }
         }
         if (!found && numCats < 16)
         {
            int idx;
            categories[numCats++] = achieveData.achievements[i].category;
            idx = (int)SendMessage(hCat, CB_ADDSTRING, 0,
                     (LPARAM)GetCategoryName(achieveData.achievements[i].category));
            SendMessage(hCat, CB_SETITEMDATA, idx,
                        achieveData.achievements[i].category);
         }
      }

      SendMessage(hCat, CB_SETCURSEL, 0, 0);

      // Update track button text based on current selection.
      if (trackedAchievement >= 0)
         SetDlgItemText(hDlg, IDC_ACHIEVETRACK, "Untrack");
      else
         SetDlgItemText(hDlg, IDC_ACHIEVETRACK, "Track");

      PopulateAchievementList(hDlg, ACHIEVE_CAT_ALL);

      CenterWindow(hDlg, GetParent(hDlg));
      return TRUE;
   }

   case WM_MEASUREITEM:
   {
      MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
      if (mis->CtlID == IDC_ACHIEVELIST)
      {
         mis->itemHeight = 18;
         return TRUE;
      }
      break;
   }

   case WM_DRAWITEM:
   {
      DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
      if (dis->CtlID == IDC_ACHIEVELIST)
      {
         DrawAchievementItem(dis);
         return TRUE;
      }
      break;
   }

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
      case IDCANCEL:
         DestroyWindow(hDlg);
         hAchieveDialog = NULL;
         return TRUE;

      case IDC_ACHIEVESEARCH:
         if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
         {
            GetDlgItemText(hDlg, IDC_ACHIEVESEARCH,
                           searchFilter, sizeof(searchFilter));
            PopulateAchievementList(hDlg, currentFilter);
         }
         return TRUE;

      case IDC_ACHIEVECAT:
         if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
         {
            HWND hCat = GetDlgItem(hDlg, IDC_ACHIEVECAT);
            int sel = (int)SendMessage(hCat, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR)
            {
               currentFilter = (BYTE)SendMessage(hCat, CB_GETITEMDATA, sel, 0);
               PopulateAchievementList(hDlg, currentFilter);
            }
         }
         return TRUE;

      case IDC_ACHIEVETRACK:
      {
         HWND hList = GetDlgItem(hDlg, IDC_ACHIEVELIST);
         int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);

         if (trackedAchievement >= 0)
         {
            // Untrack.
            AchievementTrackerDestroy();
            SetDlgItemText(hDlg, IDC_ACHIEVETRACK, "Track");
            InvalidateRect(hList, NULL, TRUE);
         }
         else if (sel != LB_ERR && sel < listCount)
         {
            int ai = listToAchieve[sel];

            // Only track unfinished achievements with progress.
            if (!achieveData.achievements[ai].earned)
            {
               trackedAchievement = achieveData.achievements[ai].id;
               SetDlgItemText(hDlg, IDC_ACHIEVETRACK, "Untrack");
               ShowTracker();
               InvalidateRect(hList, NULL, TRUE);
            }
         }
         return TRUE;
      }

      case IDC_ACHIEVELIST:
         if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
         {
            HWND hList = GetDlgItem(hDlg, IDC_ACHIEVELIST);
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR && sel < listCount)
            {
               int ai = listToAchieve[sel];
               char descText[512];

               FormatDescText(ai, descText, sizeof(descText));
               SetDlgItemText(hDlg, IDC_ACHIEVEDESC, descText);

               // Update track button based on selection.
               if (trackedAchievement == achieveData.achievements[ai].id)
                  SetDlgItemText(hDlg, IDC_ACHIEVETRACK, "Untrack");
               else
                  SetDlgItemText(hDlg, IDC_ACHIEVETRACK, "Track");

               // Enable/disable claim button.
               EnableWindow(GetDlgItem(hDlg, IDC_ACHIEVECLAIM),
                            achieveData.achievements[ai].unclaimed);
            }
         }
         return TRUE;

      case IDC_ACHIEVECLAIM:
      {
         HWND hList = GetDlgItem(hDlg, IDC_ACHIEVELIST);
         int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);

         if (sel != LB_ERR && sel < listCount)
         {
            int ai = listToAchieve[sel];

            if (achieveData.achievements[ai].unclaimed)
            {
               ClaimAchievement(achieveData.achievements[ai].id);
               // Server will send updated list, dialog refreshes.
            }
         }
         return TRUE;
      }
      }
      break;

   case WM_DESTROY:
      hAchieveDialog = NULL;
      return TRUE;
   }

   return FALSE;
}
