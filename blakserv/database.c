// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* database.c
*
*/

#include "blakserv.h"
#include "database_schema.h"
#include "database_common.h"

#define SLEEPTIME              10
#define SLEEPTIMENOCON         1000
#define SLEEPINIT              100

// Max number of parameters to an SQL call. Based on max kod params.

sql_queue queue         = {0, 0, 0, 0};
HANDLE hMySQLWorker     = 0;
sql_worker_state state	= STOPPED;
MYSQL* mysql   = 0;
char* host     = 0;
char* user     = 0;
char* password = 0;
char* db       = 0;
int   port     = 3306;
UINT record_count = 0;

// Calls mysql_query with query 'b' and logs an error if query fails.
// Doesn't log 'table already exists' error.
#define MYSQL_QUERY_CHECKED(a, b, c) \
   c = mysql_query(a, b); \
   if (c != 0) { \
      const char *_err = mysql_error(a); \
      if (!(strstr(_err, "Table") && strstr(_err, "already exists"))) { \
         bprintf("MySQL error %i on line %i: %s", c, __LINE__, _err); } }

/*
 * MySQLGenerateDrop: Assembles a string in the passed buffer which specifies
 *   a SQL procedure to drop based on the sql record type passed.
 */
void MySQLGenerateDrop(sql_recordtype type, char *buffer)
{
   int num_chars = sprintf(buffer, "DROP PROCEDURE IF EXISTS %s;",
      Statistics_Table[type].procedure_name);
   buffer[num_chars] = 0;
}

static bool MySQLAppendProcedureColumns(std::string &sql,
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
         item = "p" + std::to_string(i + 1);
      else if (!DatabaseGetColumn(operation->columns, i, item))
         return false;

      DatabaseAppendSqlItem(sql, item, first_item);
   }

   return true;
}

static std::string MySQLGenerateProcedure(const sql_statistic_type *operation)
{
   bool first_item = true;
   std::string sql = "CREATE PROCEDURE ";

   sql += operation->procedure_name;
   sql += "(";

   for (int i = 0; i < operation->num_fields; ++i)
   {
      std::string parameter = "IN p" + std::to_string(i + 1) + " LONGTEXT";
      DatabaseAppendSqlItem(sql, parameter, first_item);
   }

   sql += ") BEGIN ";

   if (operation->write_mode == DB_WRITE_UPDATE)
   {
      sql += "UPDATE ";
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
         sql += column + " = p" + std::to_string(i + 1);
      }
   }
   else
   {
      sql += "INSERT INTO ";
      sql += operation->table_name;
      sql += " (";

      if (!MySQLAppendProcedureColumns(sql, operation, false))
         return std::string();
      sql += ") VALUES (";

      if (!MySQLAppendProcedureColumns(sql, operation, true))
         return std::string();
      sql += ")";

      if (operation->write_mode == DB_WRITE_UPSERT)
      {
         sql += " ON DUPLICATE KEY UPDATE ";

         first_item = true;
         for (int i = 0; ; ++i)
         {
            std::string column;
            if (!DatabaseGetColumn(operation->update_columns, i, column))
               break;
            std::string assignment = column + " = VALUES(" + column + ")";
            DatabaseAppendSqlItem(sql, assignment, first_item);
         }
      }
   }

   sql += "; END";
   return sql;
}

/*
 * MySQLGenerateTruncate: Assembles a string in the passed buffer which specifies
 *   a SQL table to truncate (empty) based on the sql record type passed.
 */
void MySQLGenerateTruncate(sql_recordtype type, char *buffer)
{
   int num_chars = sprintf(buffer, "TRUNCATE TABLE %s;",
      Statistics_Table[type].table_name);
   buffer[num_chars] = 0;
}

/*
 * MySQLGenerateCall: Assembles a string in the passed buffer which specifies
 *   a call for a SQL procedure based on the sql record type passed.
 */
void MySQLGenerateCall(sql_recordtype type, char *buffer)
{
   if (Statistics_Table[type].num_fields == 0)
   {
      sprintf(buffer, "CALL %s()", Statistics_Table[type].procedure_name);
      return;
   }

   int num_chars = sprintf(buffer, "CALL %s(", Statistics_Table[type].procedure_name);
   for (int i = 0; i < Statistics_Table[type].num_fields - 1; ++i)
      num_chars += sprintf(buffer + num_chars, "?,");
   num_chars += sprintf(buffer + num_chars, "?);");
   buffer[num_chars] = 0;
}

#pragma region Public
/*
 * MySQLDuplicateString: Takes a string and returns a copy allocated with
 *   SQL's memory ID for tracking. Returns NULL if passed NULL.
 */
char *MySQLDuplicateString(char *str)
{
   if (!str)
   {
      bprintf("MySQLDuplicateString could not duplicate null string!");
      return NULL;
   }
   int sLen = strlen(str);
   char *ret_str = (char *)AllocateMemory(MALLOC_ID_SQL, sLen + 1);
   memcpy(ret_str, str, sLen);
   ret_str[sLen] = 0;

   return ret_str;
}

void MySQLInit(char* Host, int Port, char* User, char* Password, char* DB)
{	
	// don't init if already running or a parameter is nullptr
	if (state != STOPPED || !Host || !User || !Password || !DB)
		return;
	
   // Initialize the queue before callers can submit records or the worker connects.
   if (!DatabaseQueueInit(&queue))
      return;

	// set state to starting
	state = STARTING;

	// save connection info
	host		= _strdup(Host);
	user		= _strdup(User);
	password	= _strdup(Password);
	db			= _strdup(DB);
   port = Port;

	// spawn workthread
	hMySQLWorker = (HANDLE) _beginthread(_MySQLWorker, 0, 0);

	// sleep a bit to let workthread get initialized
	// e.g. no one calls MySQLRecordXY right after Init
	Sleep(SLEEPINIT);
};

void MySQLEnd()
{
	// set state to stopping
	state = STOPPING;
};


// How many SQL commands have we run/records sent.
UINT64 MySQLGetRecordCount()
{
   return record_count;
}

/*
 * MySQLTypeNumArgs: Return the number of expected arguments for a given
 *   SQL statistic type.
 */
int MySQLTypeNumArgs(int type)
{
   if (type >= STAT_NONE && type <= STAT_MAXSTAT)
      return Statistics_Table[type].num_fields;

   bprintf("Unknown type received in MySQLTypeNumArgs: %i", type);

   return 0;
}

bool MySQLIsTypeEnabled(int type)
{
   if (type >= STAT_NONE && type <= STAT_MAXSTAT)
      return Statistics_Table[type].enabled;

   bprintf("Unknown type received in MySQLIsTypeEnabled: %i", type);

   return false;
}

bool MySQLSetTypeEnabled(int type, bool enabled)
{
   if (type >= STAT_NONE && type <= STAT_MAXSTAT)
   {
      Statistics_Table[type].enabled = enabled;

      return true;
   }

   bprintf("Unknown type received in MySQLSetTypeEnabled: %i", type);

   return false;
}

/*
 * FreeDataNodeMemory: Frees the memory associated with sql_data_node structure.
 *   Handles freeing strings and then frees the structure itself.
 *   total_fields: size of the array
 *   fields_entered: number of valid array elements (for partially processed data)
 *   data: the data array
 */
void FreeDataNodeMemory(int total_fields, int fields_entered, sql_data_node data[])
{
   for (int i = 0; i < fields_entered; ++i)
   {
      switch (data[i].type)
      {
      case TAG_STRING:
      case TAG_RESOURCE:
         FreeMemory(MALLOC_ID_SQL, data[i].value.str, strlen(data[i].value.str) + 1);
         data[i].value.str = NULL;
         break;
      }
   }
   FreeMemory(MALLOC_ID_SQL, data, sizeof(sql_data_node) * total_fields);
}

/*
 * MySQLRecordGeneric: Enqueues the data for a SQL procedure call.
 *   Frees the memory associated with data if any checks fail.
 *      type: the SQL statistic to enqueue
 *      num_fields: the length of the data array passed in
 *      data: an array of data to log to SQL
 */
BOOL MySQLRecordGeneric(int type, int num_fields, sql_data_node data[])
{
   if (num_fields < 0)
      return FALSE;

	if (state == STOPPED || state == STOPPING)
	{
		FreeDataNodeMemory(num_fields, num_fields, data);
		return FALSE;
	}

   // Check number of args - not done in C_RecordStat because blakserv
   // can call MySQLRecordGeneric directly.
   if (MySQLTypeNumArgs(type) != num_fields)
   {
      bprintf("Invalid number of items received in MySQLRecordGeneric for statistic %i, found %i expected %i.",
         type, num_fields, MySQLTypeNumArgs(type));

      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }

   if (num_fields > DATABASE_MAX_SQL_PARAMS)
   {
      bprintf("Too many params received in MySQLRecordGeneric for statistic %i, found %i max %i.",
         type, num_fields, DATABASE_MAX_SQL_PARAMS);

      FreeDataNodeMemory(num_fields, num_fields, data);
      return FALSE;
   }

   sql_queue_node* node = (sql_queue_node*)AllocateMemory(MALLOC_ID_SQL, sizeof(sql_queue_node));
   if (!node)
   {
      bprintf("Could not allocate memory for sql queue node in MySQLRecordGeneric.");
      FreeDataNodeMemory(num_fields, num_fields, data);

      return FALSE;
   }
   node->type = (sql_recordtype)type;
   node->num_fields = num_fields;
   node->data = data;

   // try to enqueue
   BOOL enqueued = _MySQLEnqueue(node);

   // cleanup in case of fail
   if (!enqueued)
   {
      FreeDataNodeMemory(num_fields, num_fields, data);
      node->data = NULL;
      FreeMemory(MALLOC_ID_SQL, node, sizeof(sql_queue_node));
   }

   return enqueued;
}

/*
 * MySQLEmptyTable: Enqueues the data for a SQL truncate table call.
 *      type: the SQL statistic to lookup the table
                truncate proc to enqueue
 */
BOOL MySQLEmptyTable(int type)
{
   sql_queue_node* node = (sql_queue_node*)AllocateMemory(MALLOC_ID_SQL, sizeof(sql_queue_node));
   if (!node)
   {
      bprintf("Could not allocate memory for sql queue node in MySQLEmptyTable.");

      return FALSE;
   }

   node->type = (sql_recordtype)type;
   // Set num_fields to negative value to mark it as a table truncate.
   node->num_fields = -1;
   node->data = NULL;

   // try to enqueue
   BOOL enqueued = _MySQLEnqueue(node);

   // cleanup in case of fail
   if (!enqueued)
      FreeMemory(MALLOC_ID_SQL, node, sizeof(sql_queue_node));

   return enqueued;
}
#pragma endregion

#pragma region Internal
void __cdecl _MySQLWorker(void* parameters)
{
	bool reconnect		= 1;
	int		processedcount	= 0;

	/******************************************
	/*            Initialization
	/******************************************/
		
	// init mysql
	mysql = mysql_init(NULL);
	
	if (!mysql)
		return;
	
	// enable auto-reconnect
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);

	// check if MySQLEnd was somehow already called and switched state to STOPPING
	// we still expect STARTING here and if so go to INITIALIZED
	if (state == STARTING)
		state = INITIALIZED;

	/******************************************
	/*              Work loop
	/******************************************/
	while(state != STOPPING)
	{	
		/**** Something went wrong with mysql ****/
		if (!mysql)		
			state = STOPPING;
				
		/****** No initial connection yet *****/
      else if (state == INITIALIZED)
      {
         // try connect
         if (mysql == mysql_real_connect(mysql, host, user, password, db, port, NULL, 0))
         {
            state = CONNECTED;
         }

         // not successful, sleep
         else
         {
            lprintf("Failed to connect to mysql: %s", mysql_error(mysql));
            Sleep(SLEEPTIMENOCON);
         }
		}

		/****** At least was connected once *****/
		else if (state >= CONNECTED)
		{
			// verify connection is still up
			// this will auto-reconnect
			if (mysql_ping(mysql) == 0)
			{
				// schema not yet verified/created
				if (state == CONNECTED)
					_MySQLVerifySchema();

				// fully running
				else if (state >= SCHEMAVERIFIED)
				{
					// reset counter for processed items
					processedcount = 0;

					// process up to limit pending items
					while(_MySQLDequeue(TRUE))
					{
						processedcount++;

						// reached limit, do more next loop
						// to get updated state etc.
                  if (processedcount >= DATABASE_MAX_ITEMS_PER_LOOP)
							break;						
					}
					
					// defaultsleep if none processed
					if (processedcount == 0)
						Sleep(SLEEPTIME);					
				}
			}
			else
				Sleep(SLEEPTIMENOCON);	
		}
	}

	/******************************************
	/*              Shutdown
	/******************************************/

	// close mysql
	if (mysql)
		mysql_close(mysql);

	// free local connection strings
	if (host)
		free(host);
	
	if (user)
		free(user);
	
	if (password)
		free(password);
	
	if (db)
		free(db);

	// clear queue
	while(_MySQLDequeue(FALSE));

   DatabaseQueueShutdown(&queue);

	// mark stopped
	state = STOPPED;

	// finalize thread
	_endthread();
};

void _MySQLVerifySchema()
{
   if (!mysql || !state)
      return;

   // Used to check errors.
   int status;

   for (size_t i = 0; i < DATABASE_SCHEMA_STATEMENT_COUNT; ++i)
      MYSQL_QUERY_CHECKED(mysql, DatabaseSchemaStatements[i], status);

   char buffer[128];
   // Drop existing procedures if present.
   for (int i = 1; i <= STAT_MAXSTAT; ++i)
   {
      MySQLGenerateDrop(Statistics_Table[i].stat_type, buffer);
      MYSQL_QUERY_CHECKED(mysql, buffer, status);
   }

   // Recreate procedures from the shared write contract.
   for (int i = 1; i <= STAT_MAXSTAT; ++i)
   {
      std::string procedure = MySQLGenerateProcedure(&Statistics_Table[i]);
      if (procedure.empty())
      {
         bprintf("Could not generate MySQL procedure for statistic %i.", i);
         continue;
      }
      MYSQL_QUERY_CHECKED(mysql, procedure.c_str(), status);
   }

   // set state to schema verified
   state = SCHEMAVERIFIED;
};

BOOL _MySQLEnqueue(sql_queue_node* Node)
{
   return DatabaseQueueEnqueue(&queue, Node, DATABASE_MAX_RECORD_QUEUE)
      ? TRUE : FALSE;
};

BOOL _MySQLDequeue(BOOL processNode)
{
   sql_queue_node *node = DatabaseQueueDequeue(&queue);
   if (node)
   {
      // process node
      if (node->num_fields < 0)
      {
         _MySQLTruncate(node, processNode);
      }
      else
      {
         _MySQLWriteNode(node, processNode);

         // free memory of processed record and node
         FreeDataNodeMemory(node->num_fields, node->num_fields, (sql_data_node *)node->data);
      }
      FreeMemory(MALLOC_ID_SQL, node, sizeof(sql_queue_node));
   }

   return node != NULL;
}

/*
 * _MySQLTruncate: Obtain the correct truncate string for given node type
 *   and pass it on to _MySQLCallProc.
 */
void _MySQLTruncate(sql_queue_node* Node, BOOL ProcessNode)
{
   if (!Node)
      return;

   // really write it, or just free mem at end?
   if (ProcessNode)
   {
      // call stored procedure
      char buffer[128];
      MySQLGenerateTruncate(Node->type, buffer);
      _MySQLCallProc(buffer, NULL, TRUE);
   }
}

/*
 * _MySQLCallProc: Call a MySQL procedure on the database. Parameters can be NULL
 *   if calling a TRUNCATE procedure instead of a WRITE, but nullParams must be
 *   passed as TRUE to allow error-checking NULL Paremeters on WRITE statements.
 */
void _MySQLCallProc(char* ProcName, MYSQL_BIND Parameters[], BOOL nullParams = FALSE)
{
   MYSQL_STMT*	stmt;
   int status;

   if (!ProcName || (!Parameters && !nullParams))
      return;

   // init statement
   stmt = mysql_stmt_init(mysql);

   if (!stmt)
      return;

   // execute stored procedure
   status = mysql_stmt_prepare(stmt, ProcName, strlen(ProcName));
   if (status != 0 && mysql)
   {
      bprintf("MySQL error in mysql_stmt_prepare with procedure %s, status code %i: %s",
         ProcName, status, mysql_error(mysql));
      mysql_stmt_close(stmt);
      return;
   }

   if (!nullParams)
   {
      status = mysql_stmt_bind_param(stmt, Parameters);
      if (status != 0 && mysql)
      {
         bprintf("MySQL error in mysql_stmt_bind_param with procedure %s, status code %i: %s",
            ProcName, status, mysql_error(mysql));
         mysql_stmt_close(stmt);
         return;
      }
   }

   status = mysql_stmt_execute(stmt);
   if (status != 0 && mysql)
   {
      bprintf("MySQL error in mysql_stmt_execute with procedure %s, status code %i: %s",
         ProcName, status, mysql_error(mysql));
   }

   ++record_count;

   // close statement
   mysql_stmt_close(stmt);
};

void _MySQLWriteNode(sql_queue_node* Node, BOOL ProcessNode)
{
   if (!Node || !Node->data)
      return;

   MYSQL_BIND params[DATABASE_MAX_SQL_PARAMS];
   unsigned long str_lens[DATABASE_MAX_SQL_PARAMS];
   my_bool is_null = 1;

   // really write it, or just free mem at end?
   if (ProcessNode)
   {
      sql_data_node *data = (sql_data_node *)Node->data;
      // allocate parameters
      memset(params, 0, sizeof(params));
      for (int i = 0; i < Node->num_fields; ++i)
      {
         switch (data[i].type)
         {
         case TAG_NIL:
            params[i].buffer_type = MYSQL_TYPE_NULL;
            params[i].length = 0;
            params[i].is_null = &is_null;
            break;
         case TAG_INT:
            params[i].buffer_type = MYSQL_TYPE_LONG;
            params[i].buffer = &data[i].value.num;
            params[i].length = 0;
            params[i].is_null = 0;
            break;
         case TAG_STRING:
         case TAG_RESOURCE:
            str_lens[i] = strlen(data[i].value.str);
            params[i].buffer_type = MYSQL_TYPE_STRING;
            params[i].buffer = data[i].value.str;
            params[i].length = &str_lens[i];
            params[i].is_null = 0;
            break;
         }
      }

      // call stored procedure
      char buffer[128];
      MySQLGenerateCall(Node->type, buffer);
      _MySQLCallProc(buffer, params);
   }
};
#pragma endregion
