// Meridian 59 - PostgreSQL database implementation.
// Replaces MySQL database.c with libpq-based PostgreSQL support.

#include "blakserv.h"
#ifndef BLAK_PLATFORM_WINDOWS
#include <pthread.h>
#include <unistd.h>
#endif
#include <libpq-fe.h>
#define DATABASE_SCHEMA_POSTGRESQL
#include "database_schema.h"
#include "database_common.h"

// Worker thread state
enum { DB_STOPPED=0, DB_STOPPING, DB_STARTING, DB_CONNECTED, DB_READY };
static int db_state = DB_STOPPED;

// Connection
static PGconn *pg_conn = NULL;
static char *db_host = NULL;
static int db_port = 0;
static char *db_user = NULL;
static char *db_pass = NULL;
static char *db_name = NULL;

// Queue
static sql_queue queue;
static UINT64 record_count = 0;

// Worker thread
#ifdef BLAK_PLATFORM_WINDOWS
static HANDLE db_thread = NULL;
#else
static pthread_t db_thread;
#endif

#define DB_WORKER_SLEEP_MS 10
#define DB_RECONNECT_SLEEP_MS 5000

#ifdef BLAK_PLATFORM_WINDOWS
static void PgSleepMs(unsigned int delay_ms)
{
   Sleep(delay_ms);
}

static char *PgStrdup(const char *value)
{
   return value != NULL ? _strdup(value) : NULL;
}

static bool PgThreadStart(void);
static void PgThreadJoin(void);
static unsigned __stdcall PgWorker(void *arg);
#else
static void PgSleepMs(unsigned int delay_ms)
{
   usleep(delay_ms * 1000);
}

static char *PgStrdup(const char *value)
{
   return value != NULL ? strdup(value) : NULL;
}

static bool PgThreadStart(void);
static void PgThreadJoin(void);
static void *PgWorker(void *arg);
#endif

static bool PgThreadStart(void)
{
#ifdef BLAK_PLATFORM_WINDOWS
   uintptr_t thread_handle = _beginthreadex(NULL, 0, PgWorker, NULL, 0, NULL);
   if (thread_handle == 0)
      return false;

   db_thread = (HANDLE)thread_handle;
   return true;
#else
   return pthread_create(&db_thread, NULL, PgWorker, NULL) == 0;
#endif
}

static void PgThreadJoin(void)
{
#ifdef BLAK_PLATFORM_WINDOWS
   if (db_thread != NULL)
   {
      WaitForSingleObject(db_thread, INFINITE);
      CloseHandle(db_thread);
      db_thread = NULL;
   }
#else
   pthread_join(db_thread, NULL);
#endif
}

/******************************************************************************/
// Internal functions
/******************************************************************************/

static bool PgConnect(void)
{
   char conninfo[512];
   snprintf(conninfo, sizeof(conninfo),
      "host=%s port=%d user=%s password=%s dbname=%s",
      db_host, db_port, db_user, db_pass, db_name);

   pg_conn = PQconnectdb(conninfo);
   if (PQstatus(pg_conn) != CONNECTION_OK)
   {
      eprintf("PostgreSQL connection failed: %s\n", PQerrorMessage(pg_conn));
      PQfinish(pg_conn);
      pg_conn = NULL;
      return false;
   }

   // Set client encoding to WIN1252 for German umlauts and legacy data
   PQsetClientEncoding(pg_conn, "WIN1252");

   lprintf("PostgreSQL connected to %s:%d/%s\n", db_host, db_port, db_name);
   return true;
}

static bool PgVerifySchema(void)
{
   for (size_t i = 0; i < DATABASE_SCHEMA_STATEMENT_COUNT; ++i)
   {
      PGresult *res = PQexec(pg_conn, DatabaseSchemaStatements[i]);
      if (PQresultStatus(res) != PGRES_COMMAND_OK)
      {
         eprintf("PostgreSQL schema creation failed: %s\n", PQerrorMessage(pg_conn));
         PQclear(res);
         return false;
      }
      PQclear(res);
   }
   dprintf("PostgreSQL schema verified\n");
   return true;
}

static bool PgIsConnected(void)
{
   if (!pg_conn)
      return false;
   if (PQstatus(pg_conn) != CONNECTION_OK)
   {
      // Try to reset the connection
      PQreset(pg_conn);
      return PQstatus(pg_conn) == CONNECTION_OK;
   }
   return true;
}

static bool PgAppendInsertColumns(std::string &sql,
   const sql_statistic_type *operation, bool values)
{
   bool first_item = true;
   for (int i = 0; i <= operation->num_fields; ++i)
   {
      if (operation->timestamp_column && operation->timestamp_position == i)
      {
         DatabaseAppendSqlItem(sql,
            values ? "NOW()" : operation->timestamp_column, first_item);
      }

      if (i == operation->num_fields)
         break;

      std::string item;
      if (values)
         item = "$" + std::to_string(i + 1);
      else if (!DatabaseGetColumn(operation->columns, i, item))
         return false;

      DatabaseAppendSqlItem(sql, item, first_item);
   }

   return true;
}

static std::string PgBuildWriteQuery(
   const sql_statistic_type *operation)
{
   bool first_item = true;
   std::string sql;

   if (operation->write_mode == DB_WRITE_UPDATE)
   {
      sql = "UPDATE ";
      sql += operation->table_name;
      sql += " SET ";
      sql += operation->update_assignments;
      sql += " WHERE ";

      for (int i = 0; i < operation->num_fields; ++i)
      {
         std::string column;
         if (!DatabaseGetColumn(operation->columns, i, column))
            return std::string();
         if (i > 0)
            sql += " AND ";
         sql += column + " = $" + std::to_string(i + 1);
      }
      return sql;
   }

   sql = "INSERT INTO ";
   sql += operation->table_name;
   sql += " (";

   if (!PgAppendInsertColumns(sql, operation, false))
      return std::string();
   sql += ") VALUES (";

   first_item = true;
   if (!PgAppendInsertColumns(sql, operation, true))
      return std::string();
   sql += ")";

   if (operation->write_mode == DB_WRITE_UPSERT)
   {
      sql += " ON CONFLICT (";
      sql += operation->conflict_columns;
      sql += ") DO UPDATE SET ";

      first_item = true;
      for (int i = 0; ; ++i)
      {
         std::string column;
         if (!DatabaseGetColumn(operation->update_columns, i, column))
            break;
         std::string assignment = column + " = EXCLUDED." + column;
         DatabaseAppendSqlItem(sql, assignment, first_item);
      }
   }

   return sql;
}

static void PgWriteNode(sql_queue_node *node)
{
   sql_data_node *data = (sql_data_node *)node->data;
   int type = node->type;
   int nf = node->num_fields;

   if (type <= STAT_NONE || type > STAT_MAXSTAT)
      return;

   if (!Statistics_Table[type].enabled)
      return;

   const sql_statistic_type *operation = &Statistics_Table[type];
   if (!operation->table_name)
      return;

   if (nf != operation->num_fields || nf > DATABASE_MAX_SQL_PARAMS)
   {
      eprintf("Invalid PostgreSQL field count for statistic %d: found %d, expected %d\n",
         type, nf, operation->num_fields);
      return;
   }

   // Build parameterized INSERT
   // Convert all fields to string params for PQexecParams
   const char *paramValues[DATABASE_MAX_SQL_PARAMS];
   char numbufs[DATABASE_MAX_SQL_PARAMS][20];
   int nParams = 0;

   for (int i = 0; i < nf && i < DATABASE_MAX_SQL_PARAMS; i++)
   {
      switch (data[i].type)
      {
      case TAG_NIL:
         paramValues[nParams] = NULL;
         break;
      case TAG_INT:
         snprintf(numbufs[nParams], sizeof(numbufs[0]), "%d", data[i].value.num);
         paramValues[nParams] = numbufs[nParams];
         break;
      case TAG_STRING:
      case TAG_RESOURCE:
         paramValues[nParams] = data[i].value.str ? data[i].value.str : "";
         break;
      case TAG_SESSION:
         snprintf(numbufs[nParams], sizeof(numbufs[0]), "%d", data[i].value.num);
         paramValues[nParams] = numbufs[nParams];
         break;
      default:
         paramValues[nParams] = NULL;
         break;
      }
      nParams++;
   }

   std::string sql = PgBuildWriteQuery(operation);
   if (sql.empty())
   {
      eprintf("Could not build PostgreSQL write query for statistic %d\n", type);
      return;
   }

   PGresult *res = PQexecParams(pg_conn, sql.c_str(), nParams, NULL,
      paramValues, NULL, NULL, 0);

   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      // Don't spam errors - just log once
      static int error_count = 0;
      if (error_count++ < 10)
         eprintf("PostgreSQL write to %s failed: %s\n", operation->table_name, PQerrorMessage(pg_conn));
   }
   else
   {
      record_count++;
   }
   PQclear(res);
}

static void PgTruncateTable(int type)
{
   if (type <= STAT_NONE || type > STAT_MAXSTAT)
      return;

   const char *table = Statistics_Table[type].table_name;
   if (!table)
      return;

   std::string sql = "TRUNCATE TABLE ";
   sql += table;
   PGresult *res = PQexec(pg_conn, sql.c_str());
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
      eprintf("PostgreSQL TRUNCATE %s failed: %s\n", table, PQerrorMessage(pg_conn));
   PQclear(res);
}

static bool PgEnqueueNode(sql_queue_node *node)
{
   if (!node || db_state == DB_STOPPED || db_state == DB_STOPPING)
      return false;

   if (db_state == DB_STOPPED || db_state == DB_STOPPING)
      return false;

   return DatabaseQueueEnqueue(&queue, node, DATABASE_MAX_RECORD_QUEUE);
}

static bool PgDequeue(bool processNode)
{
   sql_queue_node *node = DatabaseQueueDequeue(&queue);
   if (!node)
      return false;

   if (processNode)
   {
      if (node->num_fields < 0)
         PgTruncateTable(node->type);
      else
         PgWriteNode(node);
   }

   // Free data
   if (node->data)
      FreeDataNodeMemory(abs(node->num_fields),
         abs(node->num_fields), (sql_data_node *)node->data);
   FreeMemory(MALLOC_ID_SQL, node, sizeof(sql_queue_node));

   return true;
}

#ifdef BLAK_PLATFORM_WINDOWS
static unsigned __stdcall PgWorker(void *arg)
#else
static void *PgWorker(void *arg)
#endif
{
   // Connect
   while (db_state != DB_STOPPING)
   {
      if (PgConnect())
      {
         if (PgVerifySchema())
         {
            db_state = DB_READY;
            break;
         }
         PQfinish(pg_conn);
         pg_conn = NULL;
      }
      // Retry after delay
      PgSleepMs(DB_RECONNECT_SLEEP_MS);
   }

   // Main work loop
   while (db_state != DB_STOPPING)
   {
      if (db_state == DB_READY && PgIsConnected())
      {
         int processed = 0;
         for (int i = 0; i < DATABASE_MAX_ITEMS_PER_LOOP; i++)
         {
            if (PgDequeue(true))
               processed++;
            else
               break;
         }
         if (processed == 0)
            PgSleepMs(DB_WORKER_SLEEP_MS);
      }
      else
      {
         // Try reconnect
         if (!PgIsConnected())
         {
            eprintf("PostgreSQL connection lost, reconnecting...\n");
            if (!PgConnect())
               PgSleepMs(DB_RECONNECT_SLEEP_MS);
            else
               db_state = DB_READY;
         }
      }
   }

   // Discard queued records if shutdown occurs before they can be written.
   while (PgDequeue(false)) {}

   if (pg_conn)
   {
      PQfinish(pg_conn);
      pg_conn = NULL;
   }
   DatabaseQueueShutdown(&queue);
   db_state = DB_STOPPED;
#ifdef BLAK_PLATFORM_WINDOWS
   return 0;
#else
   return NULL;
#endif
}

/******************************************************************************/
// Public API
/******************************************************************************/

void MySQLInit(char *Host, int Port, char *User, char *Password, char *DB)
{
   if (db_state != DB_STOPPED)
      return;

   if (!Host || !User || !Password || !DB)
   {
      eprintf("PostgreSQL init failed: missing connection parameters\n");
      return;
   }

   db_host = PgStrdup(Host);
   db_port = Port;
   db_user = PgStrdup(User);
   db_pass = PgStrdup(Password);
   db_name = PgStrdup(DB);

   if (!db_host || !db_user || !db_pass || !db_name)
   {
      free(db_host); db_host = NULL;
      free(db_user); db_user = NULL;
      free(db_pass); db_pass = NULL;
      free(db_name); db_name = NULL;
      eprintf("PostgreSQL init failed: could not allocate connection parameters\n");
      return;
   }

   if (!DatabaseQueueInit(&queue))
   {
      free(db_host); db_host = NULL;
      free(db_user); db_user = NULL;
      free(db_pass); db_pass = NULL;
      free(db_name); db_name = NULL;
      eprintf("PostgreSQL init failed: could not initialize queue\n");
      return;
   }

   db_state = DB_STARTING;

   if (!PgThreadStart())
   {
      DatabaseQueueShutdown(&queue);
      free(db_host); db_host = NULL;
      free(db_user); db_user = NULL;
      free(db_pass); db_pass = NULL;
      free(db_name); db_name = NULL;
      db_state = DB_STOPPED;
      eprintf("PostgreSQL init failed: could not start writer thread\n");
      return;
   }

   PgSleepMs(100); // 100ms for thread startup
}

void MySQLEnd()
{
   if (db_state == DB_STOPPED)
      return;

   db_state = DB_STOPPING;
   PgThreadJoin();

   free(db_host); db_host = NULL;
   free(db_user); db_user = NULL;
   free(db_pass); db_pass = NULL;
   free(db_name); db_name = NULL;
}

BOOL MySQLRecordGeneric(int type, int num_fields, sql_data_node data[])
{
   if (num_fields < 0)
      return FALSE;

   if (type <= STAT_NONE || type > STAT_MAXSTAT)
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }

   if (!Statistics_Table[type].enabled)
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }

   if (num_fields < 0 || num_fields > DATABASE_MAX_SQL_PARAMS ||
      num_fields != Statistics_Table[type].num_fields)
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }

   if (db_state == DB_STOPPED || db_state == DB_STOPPING)
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }

   sql_queue_node *node = (sql_queue_node *)AllocateMemory(MALLOC_ID_SQL,
      sizeof(sql_queue_node));
   if (!node)
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }
   node->type = (sql_recordtype)type;
   node->num_fields = num_fields;
   node->data = data;
   node->next = NULL;

   if (!PgEnqueueNode(node))
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      FreeMemory(MALLOC_ID_SQL, node, sizeof(sql_queue_node));
      eprintf("PostgreSQL queue unavailable or full, dropping record\n");
      return FALSE;
   }

   return TRUE;
}

BOOL MySQLEmptyTable(int type)
{
   if (type <= STAT_NONE || type > STAT_MAXSTAT ||
      db_state == DB_STOPPED || db_state == DB_STOPPING)
      return FALSE;

   sql_queue_node *node = (sql_queue_node *)AllocateMemory(MALLOC_ID_SQL,
      sizeof(sql_queue_node));
   if (!node)
      return FALSE;
   node->type = (sql_recordtype)type;
   node->num_fields = -1;
   node->data = NULL;
   node->next = NULL;

   if (!PgEnqueueNode(node))
   {
      FreeMemory(MALLOC_ID_SQL, node, sizeof(sql_queue_node));
      return FALSE;
   }

   return TRUE;
}

UINT64 MySQLGetRecordCount()
{
   return record_count;
}

char *MySQLDuplicateString(char *str)
{
   if (!str)
   {
      bprintf("MySQLDuplicateString could not duplicate null string!");
      return NULL;
   }
   int sLen = (int)strlen(str);
   char *ret_str = (char *)AllocateMemory(MALLOC_ID_SQL, sLen + 1);
   memcpy(ret_str, str, sLen);
   ret_str[sLen] = 0;
   return ret_str;
}

bool MySQLIsTypeEnabled(int type)
{
   if (type >= STAT_NONE && type <= STAT_MAXSTAT)
      return Statistics_Table[type].enabled;
   return false;
}

bool MySQLSetTypeEnabled(int type, bool enabled)
{
   if (type >= STAT_NONE && type <= STAT_MAXSTAT)
   {
      Statistics_Table[type].enabled = enabled;
      return true;
   }
   return false;
}

void FreeDataNodeMemory(int total_fields, int fields_entered, sql_data_node data[])
{
   for (int i = 0; i < fields_entered; i++)
   {
      if ((data[i].type == TAG_STRING || data[i].type == TAG_RESOURCE)
         && data[i].value.str)
      {
         FreeMemory(MALLOC_ID_SQL, data[i].value.str, (int)strlen(data[i].value.str) + 1);
      }
   }
   FreeMemory(MALLOC_ID_SQL, data, sizeof(sql_data_node) * total_fields);
}
