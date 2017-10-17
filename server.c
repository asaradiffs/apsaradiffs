--- redis/src/server.c	2017-10-17 08:40:43.082522629 -0500
+++ ApsaraCache/src/server.c	2017-10-17 08:39:01.046524456 -0500
@@ -60,6 +60,17 @@
 
 struct sharedObjectsStruct shared;
 
+/*declare the memcachedCommandTable*/
+extern struct memcachedCommand memcachedCommandTable[MEMCACHED_TEXT_REQUEST_NUM + MEMCACHED_BINARY_REQUEST_NUM];
+extern void createMemcachedSharedObjects(void);
+extern void saslListMechsMemcachedCommand(struct client *c);
+extern void saslAuthMemcachedCommand(struct client *c);
+extern void saslStepMemcachedCommand(struct client *c);
+extern void versionMemcachedCommand(struct client *c);
+extern void authMemcachedCommand(struct client *c);
+struct helpWrapper helpWrappers;
+
+
 /* Global vars that are actually used as constants. The following double
  * values are used for double on-disk serialization, and are initialized
  * at runtime to avoid strange compiler optimizations. */
@@ -464,6 +475,15 @@
     return strcasecmp(key1, key2) == 0;
 }
 
+/* A case sensitive version used for the memcached command lookup table. */
+int dictSdsKeyCaseSensitiveCompare(void *privdata, const void *key1,
+        const void *key2)
+{
+    DICT_NOTUSED(privdata);
+
+    return strcmp(key1, key2) == 0;
+}
+
 void dictObjectDestructor(void *privdata, void *val)
 {
     DICT_NOTUSED(privdata);
@@ -499,6 +519,10 @@
     return dictGenCaseHashFunction((unsigned char*)key, sdslen((char*)key));
 }
 
+uint64_t dictSdsCaseSensitiveHash(const void *key) {
+    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
+}
+
 int dictEncObjKeyCompare(void *privdata, const void *key1,
         const void *key2)
 {
@@ -611,6 +635,19 @@
     NULL                        /* val destructor */
 };
 
+/* Memcached command table. sds string -> command struct pointer. 
+ * because memcached ascii command is case sensitive, so using sensitive
+ * hash and compare functions*/
+dictType memcachedCommandTableDictType = {
+    dictSdsCaseSensitiveHash,           /* hash function */
+    NULL,                      /* key dup */
+    NULL,                      /* val dup */
+    dictSdsKeyCaseSensitiveCompare,     /* key compare */
+    dictSdsDestructor,         /* key destructor */
+    NULL                       /* val destructor */
+};
+
+
 /* Hash type hash table (note that small hashes are represented with ziplists) */
 dictType hashDictType = {
     dictSdsHash,                /* hash function */
@@ -1440,6 +1477,10 @@
     server.lazyfree_lazy_server_del = CONFIG_DEFAULT_LAZYFREE_LAZY_SERVER_DEL;
     server.always_show_logo = CONFIG_DEFAULT_ALWAYS_SHOW_LOGO;
     server.lua_time_limit = LUA_SCRIPT_TIME_LIMIT;
+    server.protocol = REDIS;
+    server.protocol_str = PROTOCOL_REDIS_STR;
+    server.protocolParseProcess = processRedisProtocolBuffer; 
+    server.max_memcached_read_request_length = MAX_MEMCACHED_READ_REQUEST_LENGTH_DEFAULT;
 
     unsigned int lruclock = getLRUClock();
     atomicSet(server.lruclock,lruclock);
@@ -1786,6 +1827,11 @@
     server.aof_delayed_fsync = 0;
 }
 
+void initServerHelperWrappers() {
+    initHelpWrappers(&server, &helpWrappers);
+}
+
+
 void initServer(void) {
     int j;
 
@@ -1823,6 +1869,20 @@
         exit(1);
     }
     server.db = zmalloc(sizeof(redisDb)*server.dbnum);
+    server.memcached_commands = NULL;
+
+    initServerHelperWrappers();
+    if (server.protocol == MEMCACHED) {
+        /* memcached ascii command is case sensitive */
+        server.memcached_commands = dictCreate(&memcachedCommandTableDictType,NULL);
+        server.protocolParseProcess = processMemcachedProtocolBuffer;
+        /* Because redis needs initialize command dict before loading config file, but 
+         * only we have loaded config file  we can know 
+         * the instance is memcached or not, so I add memcached dict, and initialize it here; */
+        populateCommandTable();
+        createMemcachedSharedObjects();
+        server.protocol_str = PROTOCOL_MEMCACHED_STR;
+    }
 
     /* Open the TCP listening socket for the user commands. */
     if (server.port != 0 &&
@@ -1953,14 +2013,34 @@
 
 /* Populates the Redis Command Table starting from the hard coded list
  * we have on top of redis.c file. */
+#define REDIS_COMMAND_STRUCT_SIZE sizeof(struct redisCommand)
+#define REDIS_COMMAND_TABLE_SIZE sizeof(redisCommandTable)
+#define REDIS_AUX_THREAD_COMMAND_TABLE_SIZE sizeof(redisAuxThreadCommandTable)
+#define MEMCACHED_COMMAND_STRUCT_SIZE sizeof(struct memcachedCommand)
+#define MEMCACHED_COMMAND_TABLE_SIZE sizeof(memcachedCommandTable)
 void populateCommandTable(void) {
     int j;
+    char *commandTable = NULL;
     int numcommands = sizeof(redisCommandTable)/sizeof(struct redisCommand);
+    size_t cmss = 1;
+    if (server.protocol == REDIS) {
+        cmss = sizeof(struct redisCommand);
+    } else {
+        cmss = sizeof(struct memcachedCommand);
+    }
+
+    if (server.protocol == REDIS) {
+        numcommands = sizeof(redisCommandTable) / cmss;
+        commandTable = (char *)redisCommandTable;
+    } else {
+        numcommands = sizeof(memcachedCommandTable) / cmss;
+        commandTable = (char *)memcachedCommandTable;
+    }
 
     for (j = 0; j < numcommands; j++) {
-        struct redisCommand *c = redisCommandTable+j;
+        struct redisCommand *c = (struct redisCommand *)(commandTable + j * cmss); 
         char *f = c->sflags;
-        int retval1, retval2;
+        int retval1, retval2, binary = 0;
 
         while(*f != '\0') {
             switch(*f) {
@@ -1977,15 +2057,25 @@
             case 'M': c->flags |= CMD_SKIP_MONITOR; break;
             case 'k': c->flags |= CMD_ASKING; break;
             case 'F': c->flags |= CMD_FAST; break;
+            case 'b': binary = 1; break;
             default: serverPanic("Unsupported command flag"); break;
             }
             f++;
         }
 
-        retval1 = dictAdd(server.commands, sdsnew(c->name), c);
+        if (server.protocol == REDIS) {
+            retval1 = dictAdd(server.commands, sdsnew(c->name), c);
+        } else {
+            if (!binary) {
+                retval1 = dictAdd(server.memcached_commands, sdsnew(c->name), c);
+            } else {
+                retval1 = DICT_OK;
+            }
+        }
+
         /* Populate an additional dictionary that will be unaffected
          * by rename-command statements in redis.conf. */
-        retval2 = dictAdd(server.orig_commands, sdsnew(c->name), c);
+        retval2 = (server.protocol == REDIS) ? dictAdd(server.orig_commands, sdsnew(c->name), c) : DICT_OK;
         serverAssert(retval1 == DICT_OK && retval2 == DICT_OK);
     }
 }
@@ -2057,6 +2147,20 @@
     return cmd;
 }
 
+struct memcachedCommand *lookupMemcachedCommandByCString(char *s) {
+    struct memcachedCommand *cmd;
+    sds name = sdsnew(s);
+
+    cmd = dictFetchValue(server.memcached_commands, name);
+    sdsfree(name);
+    return cmd;
+}
+
+struct memcachedCommand *lookupMemcachedCommand(sds name) {
+    return dictFetchValue(server.memcached_commands, name);
+}
+
+
 /* Lookup the command in the current table, if not found also check in
  * the original table containing the original command names unaffected by
  * redis.conf rename-command statement.
@@ -2311,21 +2415,31 @@
      * when FORCE_REPLICATION is enabled and would be implemented in
      * a regular command proc. */
     if (!strcasecmp(c->argv[0]->ptr,"quit")) {
-        addReply(c,shared.ok);
-        c->flags |= CLIENT_CLOSE_AFTER_REPLY;
+        helpWrappers.replyQuit(c, NULL);
         return C_ERR;
     }
 
-    /* Now lookup the command and check ASAP about trivial error conditions
-     * such as wrong arity, bad command name and so forth. */
-    c->cmd = c->lastcmd = lookupCommand(c->argv[0]->ptr);
+    if (server.protocol == REDIS) {
+        /* Now lookup the command and check ASAP about trivial error conditions
+         * such as wrong arity, bad command name and so forth. 
+         */
+        c->cmd = c->lastcmd = lookupCommand(c->argv[0]->ptr);
+    } else {
+         /* When running in memcached mode and the reqtype is REDIS_REQ_INLINE or MEMCACHED_REQ_ASCII or MEMCACHED_REQ_BINARY, 
+          * the cmd and lastcmd are already assigned, except reqtype is REDIS_REQ_MULTIBULK, so check it here
+          */
+        if (!c->cmd) {
+            c->cmd = c->lastcmd = lookupCommand(c->argv[0]->ptr);
+        }
+    }
     if (!c->cmd) {
+        /* Unknown memcached command should never run here,
+         * but check for safty as well; */
         flagTransaction(c);
-        addReplyErrorFormat(c,"unknown command '%s'",
-            (char*)c->argv[0]->ptr);
+        helpWrappers.replyUnknownCommandErr(c, c->argv[0]->ptr);
         return C_OK;
-    } else if ((c->cmd->arity > 0 && c->cmd->arity != c->argc) ||
-               (c->argc < -c->cmd->arity)) {
+    } else if ((c->reqtype == PROTO_REQ_INLINE || c->reqtype == PROTO_REQ_MULTIBULK) && ((c->cmd->arity > 0 && c->cmd->arity != c->argc) ||
+            (c->argc < -c->cmd->arity))) {
         flagTransaction(c);
         addReplyErrorFormat(c,"wrong number of arguments for '%s' command",
             c->cmd->name);
@@ -2333,13 +2447,23 @@
     }
 
     /* Check if the user is authenticated */
-    if (server.requirepass && !c->authenticated && c->cmd->proc != authCommand)
+    if (server.requirepass && !c->authenticated && c->cmd->proc != authCommand && c->cmd->proc != authMemcachedCommand && c->cmd->proc != saslListMechsMemcachedCommand && c->cmd->proc != saslAuthMemcachedCommand && c->cmd->proc != saslStepMemcachedCommand && c->cmd->proc != versionMemcachedCommand )
     {
         flagTransaction(c);
-        addReply(c,shared.noautherr);
+        helpWrappers.replyNoAuthErr(c, NULL);
         return C_OK;
     }
 
+    if (server.protocol == MEMCACHED) {
+        /* Running mode is memcached, but use redis protocol request */
+        if (!(c->flags & CLIENT_MASTER) && 
+            (c->reqtype == PROTO_REQ_INLINE || c->reqtype == PROTO_REQ_MULTIBULK) &&
+            (c->cmd->flags & CMD_WRITE)) {
+            addReplyError(c, "operation not permitted, invalid requst.");
+            return C_OK;
+        }
+    }
+
     /* If cluster is enabled perform the cluster redirection here.
      * However we don't perform the redirection if:
      * 1) The sender of this command is our master.
@@ -2381,7 +2505,7 @@
          * is trying to execute is denied during OOM conditions? Error. */
         if ((c->cmd->flags & CMD_DENYOOM) && retval == C_ERR) {
             flagTransaction(c);
-            addReply(c, shared.oomerr);
+            helpWrappers.replyOomErr(c, NULL);
             return C_OK;
         }
     }
@@ -2397,13 +2521,11 @@
          c->cmd->proc == pingCommand))
     {
         flagTransaction(c);
-        if (server.aof_last_write_status == C_OK)
-            addReply(c, shared.bgsaveerr);
-        else
-            addReplySds(c,
-                sdscatprintf(sdsempty(),
-                "-MISCONF Errors writing to the AOF file: %s\r\n",
-                strerror(server.aof_last_write_errno)));
+        if (server.aof_last_write_status == C_OK) {
+            helpWrappers.replyWritingAofErr(c, NULL);
+        } else {
+            helpWrappers.replyBgsaveErr(c, NULL);
+        }
         return C_OK;
     }
 
@@ -2416,7 +2538,7 @@
         server.repl_good_slaves_count < server.repl_min_slaves_to_write)
     {
         flagTransaction(c);
-        addReply(c, shared.noreplicaserr);
+        helpWrappers.replyNoreplicasErr(c, NULL);
         return C_OK;
     }
 
@@ -2426,7 +2548,7 @@
         !(c->flags & CLIENT_MASTER) &&
         c->cmd->flags & CMD_WRITE)
     {
-        addReply(c, shared.roslaveerr);
+        helpWrappers.replyRoslaveErr(c, NULL);
         return C_OK;
     }
 
@@ -2448,14 +2570,14 @@
         !(c->cmd->flags & CMD_STALE))
     {
         flagTransaction(c);
-        addReply(c, shared.masterdownerr);
+        helpWrappers.replyMasterdownErr(c, NULL);
         return C_OK;
     }
 
     /* Loading DB? Return an error if the command has not the
      * CMD_LOADING flag. */
     if (server.loading && !(c->cmd->flags & CMD_LOADING)) {
-        addReply(c, shared.loadingerr);
+        helpWrappers.replyLoadingErr(c, NULL);
         return C_OK;
     }
 
@@ -3283,6 +3405,19 @@
                 (c->calls == 0) ? 0 : ((float)c->microseconds/c->calls));
         }
         dictReleaseIterator(di);
+
+        int numcommands = MEMCACHED_COMMAND_TABLE_SIZE / MEMCACHED_COMMAND_STRUCT_SIZE;;
+        for (j = 0; j < numcommands; j++) {
+            struct redisCommand *c = (struct redisCommand *)(memcachedCommandTable+j);
+
+            if (c->calls == 0) continue;
+            /* for ascii command, whose opcode is not equal PROTOCOL_BINARY_CMD_FAKE, its statistics data is already added to its binary command */ 
+            if (j < MEMCACHED_TEXT_REQUEST_NUM && memcachedCommandTable[j].opcode != MEMCACHED_BINARY_CMD_FAKE) continue;
+            info = sdscatprintf(info,
+                "cmdstat_mem_%s:calls=%lld,usec=%lld,usec_per_call=%.2f\r\n",
+                c->name, c->calls, c->microseconds,
+                (c->calls == 0) ? 0 : ((float)c->microseconds/c->calls));
+        }
     }
 
     /* Cluster */
