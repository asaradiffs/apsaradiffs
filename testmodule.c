--- redis/src/modules/testmodule.c	2017-10-17 08:40:43.058522629 -0500
+++ ApsaraCache/src/modules/testmodule.c	2017-10-17 08:39:01.034524457 -0500
@@ -121,81 +121,6 @@
 }
 
 
-/* TEST.CTXFLAGS -- Test GetContextFlags. */
-int TestCtxFlags(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
-    REDISMODULE_NOT_USED(argc);
-    REDISMODULE_NOT_USED(argv);
-  
-    RedisModule_AutoMemory(ctx);
-  
-    int ok = 1;
-    const char *errString = NULL;
-  
-  #define FAIL(msg)    \
-    {                  \
-      ok = 0;          \
-      errString = msg; \
-      goto end;        \
-    }
-  
-    int flags = RedisModule_GetContextFlags(ctx);
-    if (flags == 0) {
-      FAIL("Got no flags");
-    }
-  
-    if (flags & REDISMODULE_CTX_FLAGS_LUA) FAIL("Lua flag was set");
-    if (flags & REDISMODULE_CTX_FLAGS_MULTI) FAIL("Multi flag was set");
-  
-    if (flags & REDISMODULE_CTX_FLAGS_AOF) FAIL("AOF Flag was set")
-    /* Enable AOF to test AOF flags */
-    RedisModule_Call(ctx, "config", "ccc", "set", "appendonly", "yes");
-    flags = RedisModule_GetContextFlags(ctx);
-    if (!(flags & REDISMODULE_CTX_FLAGS_AOF))
-      FAIL("AOF Flag not set after config set");
-  
-    if (flags & REDISMODULE_CTX_FLAGS_RDB) FAIL("RDB Flag was set");
-    /* Enable RDB to test RDB flags */
-    RedisModule_Call(ctx, "config", "ccc", "set", "save", "900 1");
-    flags = RedisModule_GetContextFlags(ctx);
-    if (!(flags & REDISMODULE_CTX_FLAGS_RDB))
-      FAIL("RDB Flag was not set after config set");
-  
-    if (!(flags & REDISMODULE_CTX_FLAGS_MASTER)) FAIL("Master flag was not set");
-    if (flags & REDISMODULE_CTX_FLAGS_SLAVE) FAIL("Slave flag was set");
-    if (flags & REDISMODULE_CTX_FLAGS_READONLY) FAIL("Read-only flag was set");
-    if (flags & REDISMODULE_CTX_FLAGS_CLUSTER) FAIL("Cluster flag was set");
-  
-    if (flags & REDISMODULE_CTX_FLAGS_MAXMEMORY) FAIL("Maxmemory flag was set");
-    ;
-    RedisModule_Call(ctx, "config", "ccc", "set", "maxmemory", "100000000");
-    flags = RedisModule_GetContextFlags(ctx);
-    if (!(flags & REDISMODULE_CTX_FLAGS_MAXMEMORY))
-      FAIL("Maxmemory flag was not set after config set");
-  
-    if (flags & REDISMODULE_CTX_FLAGS_EVICT) FAIL("Eviction flag was set");
-    RedisModule_Call(ctx, "config", "ccc", "set", "maxmemory-policy",
-                     "allkeys-lru");
-    flags = RedisModule_GetContextFlags(ctx);
-    if (!(flags & REDISMODULE_CTX_FLAGS_EVICT))
-      FAIL("Eviction flag was not set after config set");
-  
-  end:
-    /* Revert config changes */
-    RedisModule_Call(ctx, "config", "ccc", "set", "appendonly", "no");
-    RedisModule_Call(ctx, "config", "ccc", "set", "save", "");
-    RedisModule_Call(ctx, "config", "ccc", "set", "maxmemory", "0");
-    RedisModule_Call(ctx, "config", "ccc", "set", "maxmemory-policy", "noeviction");
-  
-    if (!ok) {
-      RedisModule_Log(ctx, "warning", "Failed CTXFLAGS Test. Reason: %s",
-                      errString);
-      return RedisModule_ReplyWithSimpleString(ctx, "ERR");
-    }
-  
-    return RedisModule_ReplyWithSimpleString(ctx, "OK");
-  }
-
-
 /* ----------------------------- Test framework ----------------------------- */
 
 /* Return 1 if the reply matches the specified string, otherwise log errors
@@ -263,9 +188,6 @@
     T("test.call","");
     if (!TestAssertStringReply(ctx,reply,"OK",2)) goto fail;
 
-    T("test.ctxflags","");
-    if (!TestAssertStringReply(ctx,reply,"OK",2)) goto fail;
-
     T("test.string.append","");
     if (!TestAssertStringReply(ctx,reply,"foobar",6)) goto fail;
 
@@ -307,10 +229,6 @@
         TestStringPrintf,"write deny-oom",1,1,1) == REDISMODULE_ERR)
         return REDISMODULE_ERR;
 
-    if (RedisModule_CreateCommand(ctx,"test.ctxflags",
-        TestCtxFlags,"readonly",1,1,1) == REDISMODULE_ERR)
-        return REDISMODULE_ERR;
-
     if (RedisModule_CreateCommand(ctx,"test.it",
         TestIt,"readonly",1,1,1) == REDISMODULE_ERR)
         return REDISMODULE_ERR;
