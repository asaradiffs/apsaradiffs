--- redis/src/module.c	2017-10-17 08:40:43.054522630 -0500
+++ ApsaraCache/src/module.c	2017-10-17 08:39:01.034524457 -0500
@@ -1262,74 +1262,6 @@
     return ctx->client->db->id;
 }
 
-
-/* Return the current context's flags. The flags provide information on the 
- * current request context (whether the client is a Lua script or in a MULTI),
- * and about the Redis instance in general, i.e replication and persistence. 
- * 
- * The available flags are:
- * 
- *  * REDISMODULE_CTX_FLAGS_LUA: The command is running in a Lua script
- * 
- *  * REDISMODULE_CTX_FLAGS_MULTI: The command is running inside a transaction
- * 
- *  * REDISMODULE_CTX_FLAGS_MASTER: The Redis instance is a master
- * 
- *  * REDISMODULE_CTX_FLAGS_SLAVE: The Redis instance is a slave
- * 
- *  * REDISMODULE_CTX_FLAGS_READONLY: The Redis instance is read-only
- * 
- *  * REDISMODULE_CTX_FLAGS_CLUSTER: The Redis instance is in cluster mode
- * 
- *  * REDISMODULE_CTX_FLAGS_AOF: The Redis instance has AOF enabled
- * 
- *  * REDISMODULE_CTX_FLAGS_RDB: The instance has RDB enabled
- * 
- *  * REDISMODULE_CTX_FLAGS_MAXMEMORY:  The instance has Maxmemory set
- * 
- *  * REDISMODULE_CTX_FLAGS_EVICT:  Maxmemory is set and has an eviction
- *    policy that may delete keys
- */
-int RM_GetContextFlags(RedisModuleCtx *ctx) {
-    
-    int flags = 0;
-    /* Client specific flags */
-    if (ctx->client) {
-        if (ctx->client->flags & CLIENT_LUA) 
-         flags |= REDISMODULE_CTX_FLAGS_LUA;
-        if (ctx->client->flags & CLIENT_MULTI) 
-         flags |= REDISMODULE_CTX_FLAGS_MULTI;
-    }
-
-    if (server.cluster_enabled)
-        flags |= REDISMODULE_CTX_FLAGS_CLUSTER;
-    
-    /* Maxmemory and eviction policy */
-    if (server.maxmemory > 0) {
-        flags |= REDISMODULE_CTX_FLAGS_MAXMEMORY;
-        
-        if (server.maxmemory_policy != MAXMEMORY_NO_EVICTION)
-            flags |= REDISMODULE_CTX_FLAGS_EVICT;
-    }
-
-    /* Persistence flags */
-    if (server.aof_state != AOF_OFF)
-        flags |= REDISMODULE_CTX_FLAGS_AOF;
-    if (server.saveparamslen > 0)
-        flags |= REDISMODULE_CTX_FLAGS_RDB;
-
-    /* Replication flags */
-    if (server.masterhost == NULL) {
-        flags |= REDISMODULE_CTX_FLAGS_MASTER;
-    } else {
-        flags |= REDISMODULE_CTX_FLAGS_SLAVE;
-        if (server.repl_slave_ro)
-            flags |= REDISMODULE_CTX_FLAGS_READONLY;
-    }
-    
-    return flags;
-}
-
 /* Change the currently selected DB. Returns an error if the id
  * is out of range.
  *
@@ -3401,16 +3333,14 @@
 RedisModuleBlockedClient *RM_BlockClient(RedisModuleCtx *ctx, RedisModuleCmdFunc reply_callback, RedisModuleCmdFunc timeout_callback, void (*free_privdata)(void*), long long timeout_ms) {
     client *c = ctx->client;
     int islua = c->flags & CLIENT_LUA;
-    int ismulti = c->flags & CLIENT_MULTI;
 
     c->bpop.module_blocked_handle = zmalloc(sizeof(RedisModuleBlockedClient));
     RedisModuleBlockedClient *bc = c->bpop.module_blocked_handle;
 
     /* We need to handle the invalid operation of calling modules blocking
-     * commands from Lua or MULTI. We actually create an already aborted
-     * (client set to NULL) blocked client handle, and actually reply with
-     * an error. */
-    bc->client = (islua || ismulti) ? NULL : c;
+     * commands from Lua. We actually create an already aborted (client set to
+     * NULL) blocked client handle, and actually reply to Lua with an error. */
+    bc->client = islua ? NULL : c;
     bc->module = ctx->module;
     bc->reply_callback = reply_callback;
     bc->timeout_callback = timeout_callback;
@@ -3421,11 +3351,9 @@
     bc->dbid = c->db->id;
     c->bpop.timeout = timeout_ms ? (mstime()+timeout_ms) : 0;
 
-    if (islua || ismulti) {
+    if (islua) {
         c->bpop.module_blocked_handle = NULL;
-        addReplyError(c, islua ?
-            "Blocking module command called from Lua script" :
-            "Blocking module command called from transaction");
+        addReplyError(c,"Blocking module command called from Lua script");
     } else {
         blockClient(c,BLOCKED_MODULE);
     }
@@ -3963,7 +3891,6 @@
     REGISTER_API(IsKeysPositionRequest);
     REGISTER_API(KeyAtPos);
     REGISTER_API(GetClientId);
-    REGISTER_API(GetContextFlags);
     REGISTER_API(PoolAlloc);
     REGISTER_API(CreateDataType);
     REGISTER_API(ModuleTypeSetValue);
