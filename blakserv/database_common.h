#ifndef DATABASE_COMMON_H
#define DATABASE_COMMON_H

#include <cstring>
#include <string>

#ifndef BLAK_PLATFORM_WINDOWS
#include <pthread.h>
#endif

#define DATABASE_MAX_RECORD_QUEUE 15000
#define DATABASE_MAX_SQL_PARAMS 45
#define DATABASE_MAX_ITEMS_PER_LOOP 10

enum sql_recordtype
{
   STAT_NONE = 0,
   STAT_TOTALMONEY = 1,
   STAT_MONEYCREATED = 2,
   STAT_PLAYERLOGIN = 3,
   STAT_ASSESS_DAM = 4,
   STAT_PLAYERDEATH = 5,
   STAT_PLAYER = 6,
   STAT_PLAYERSUICIDE = 7,
   STAT_GUILD = 8,
   STAT_GUILDDISBAND = 9,
   STAT_SPELLS = 10,
   STAT_SPELL_REAGENT = 11,
   STAT_TREASURE_GEN = 12,
   STAT_MONSTER = 13,
   STAT_MONSTER_ZONE = 14,
   STAT_NPCS = 15,
   STAT_WEAPONS = 16,
   STAT_ROOMS = 17,
   STAT_NPC_ZONE = 18,
   STAT_NPC_SELLITEM = 19,
   STAT_NPC_SELLSKILL = 20,
   STAT_NPC_SELLSPELL = 21,
   STAT_REAGENTS = 22,
   STAT_FOOD = 23,
   STAT_AMMO = 24,
   STAT_ARMOR = 25,
   STAT_MISCITEMS = 26,
   STAT_RINGS = 27,
   STAT_RODS = 28,
   STAT_POTIONS = 29,
   STAT_SCROLLS = 30,
   STAT_WANDS = 31,
   STAT_QUESTITEMS = 32,
   STAT_SKILLS = 33,
   STAT_NECKLACE = 34,
   STAT_INSTRUMENTS = 35,
   STAT_GEMS = 36,
   STAT_OFFERINGS = 37,
   STAT_QUESTS = 38,
   STAT_TREASURE_EXTRA = 39,
   STAT_TREASURE_MAGIC = 40,
   STAT_NPC_SELLCOND = 41,
   STAT_LOGPEN = 42,
   STAT_LOGPEN_ITEM = 43,
   STAT_QUESTGIVER = 44,
   STAT_ACCOUNTS = 45,
   STAT_ACCOUNT_CHARS = 46,
   STAT_COMPL_QUESTS = 47,
   STAT_COMPL_QUEST_ITEMS = 48,
   STAT_MAXSTAT = 48 // Equal to highest defined stat.
};

typedef enum sql_recordtype sql_recordtype;

typedef struct sql_queue_node sql_queue_node;
typedef struct sql_queue sql_queue;
typedef struct sql_data_node sql_data_node;

struct sql_data_node
{
   int type;
   union {
      int num;
      char *str;
   } value;
};

struct sql_queue_node
{
   sql_recordtype type;
   int num_fields;
   void *data;
   sql_queue_node *next;
};

struct sql_queue
{
#ifdef BLAK_PLATFORM_WINDOWS
   HANDLE mutex;
#else
   pthread_mutex_t mutex;
#endif
   int count;
   sql_queue_node *first;
   sql_queue_node *last;
};

static inline bool DatabaseQueueInit(sql_queue *queue)
{
   if (!queue)
      return false;

#ifdef BLAK_PLATFORM_WINDOWS
   queue->mutex = CreateMutex(NULL, FALSE, NULL);
   if (!queue->mutex)
      return false;
#else
   if (pthread_mutex_init(&queue->mutex, NULL) != 0)
      return false;
#endif

   queue->count = 0;
   queue->first = NULL;
   queue->last = NULL;
   return true;
}

static inline void DatabaseQueueShutdown(sql_queue *queue)
{
   if (!queue)
      return;

#ifdef BLAK_PLATFORM_WINDOWS
   if (queue->mutex)
   {
      CloseHandle(queue->mutex);
      queue->mutex = NULL;
   }
#else
   pthread_mutex_destroy(&queue->mutex);
#endif
}

static inline bool DatabaseQueueLock(sql_queue *queue)
{
#ifdef BLAK_PLATFORM_WINDOWS
   DWORD result = WaitForSingleObject(queue->mutex, INFINITE);
   return result == WAIT_OBJECT_0 || result == WAIT_ABANDONED_0;
#else
   return pthread_mutex_lock(&queue->mutex) == 0;
#endif
}

static inline void DatabaseQueueUnlock(sql_queue *queue)
{
#ifdef BLAK_PLATFORM_WINDOWS
   ReleaseMutex(queue->mutex);
#else
   pthread_mutex_unlock(&queue->mutex);
#endif
}

static inline bool DatabaseQueueEnqueue(sql_queue *queue,
   sql_queue_node *node, int max_items)
{
   if (!queue || !node || !DatabaseQueueLock(queue))
      return false;

   if (queue->count >= max_items)
   {
      DatabaseQueueUnlock(queue);
      return false;
   }

   node->next = NULL;
   if (queue->last)
      queue->last->next = node;
   else
      queue->first = node;
   queue->last = node;
   queue->count++;

   DatabaseQueueUnlock(queue);
   return true;
}

static inline sql_queue_node *DatabaseQueueDequeue(sql_queue *queue)
{
   if (!queue || !DatabaseQueueLock(queue))
      return NULL;

   sql_queue_node *node = queue->first;
   if (node)
   {
      queue->first = node->next;
      if (!queue->first)
         queue->last = NULL;
      queue->count--;
      node->next = NULL;
   }

   DatabaseQueueUnlock(queue);
   return node;
}

static inline bool DatabaseGetColumn(const char *columns, int column_index,
   std::string &column)
{
   if (!columns || column_index < 0)
      return false;

   const char *start = columns;
   for (int i = 0; i < column_index; ++i)
   {
      start = std::strchr(start, ',');
      if (!start)
         return false;
      ++start;
   }

   const char *end = std::strchr(start, ',');
   size_t column_length = end ? (size_t)(end - start) : std::strlen(start);
   if (column_length == 0)
      return false;

   column.assign(start, column_length);
   return true;
}

static inline void DatabaseAppendSqlItem(std::string &sql,
   const std::string &item, bool &first_item, const char *separator = ", ")
{
   if (!first_item)
      sql.append(separator);
   sql.append(item);
   first_item = false;
}

UINT64 MySQLGetRecordCount();
char *MySQLDuplicateString(char *str);
bool MySQLIsTypeEnabled(int type);
bool MySQLSetTypeEnabled(int type, bool enabled);
void FreeDataNodeMemory(int total_fields, int fields_entered, sql_data_node data[]);
void MySQLInit(char *Host, int Port, char *User, char *Password, char *DB);
void MySQLEnd();
BOOL MySQLRecordGeneric(int type, int num_fields, sql_data_node data[]);
BOOL MySQLEmptyTable(int type);

#endif