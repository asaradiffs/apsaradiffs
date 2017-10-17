--- redis/src/redismodule.h	2017-10-17 08:40:43.074522629 -0500
+++ ApsaraCache/src/redismodule.h	2017-10-17 08:39:01.042524456 -0500
@@ -58,30 +58,6 @@
 #define REDISMODULE_HASH_CFIELDS    (1<<2)
 #define REDISMODULE_HASH_EXISTS     (1<<3)
 
-/* Context Flags: Info about the current context returned by RM_GetContextFlags */
-
-/* The command is running in the context of a Lua script */
-#define REDISMODULE_CTX_FLAGS_LUA 0x0001
-/* The command is running inside a Redis transaction */
-#define REDISMODULE_CTX_FLAGS_MULTI 0x0002
-/* The instance is a master */
-#define REDISMODULE_CTX_FLAGS_MASTER 0x0004
-/* The instance is a slave */
-#define REDISMODULE_CTX_FLAGS_SLAVE 0x0008
-/* The instance is read-only (usually meaning it's a slave as well) */
-#define REDISMODULE_CTX_FLAGS_READONLY 0x0010
-/* The instance is running in cluster mode */
-#define REDISMODULE_CTX_FLAGS_CLUSTER 0x0020
-/* The instance has AOF enabled */
-#define REDISMODULE_CTX_FLAGS_AOF 0x0040 //
-/* The instance has RDB enabled */
-#define REDISMODULE_CTX_FLAGS_RDB 0x0080 //
-/* The instance has Maxmemory set */
-#define REDISMODULE_CTX_FLAGS_MAXMEMORY 0x0100
-/* Maxmemory is set and has an eviction policy that may delete keys */
-#define REDISMODULE_CTX_FLAGS_EVICT 0x0200 
-
-
 /* A special pointer that we can use between the core and the module to signal
  * field deletion, and that is impossible to be a valid pointer. */
 #define REDISMODULE_HASH_DELETE ((RedisModuleString*)(long)1)
@@ -207,7 +183,6 @@
 int REDISMODULE_API_FUNC(RedisModule_IsKeysPositionRequest)(RedisModuleCtx *ctx);
 void REDISMODULE_API_FUNC(RedisModule_KeyAtPos)(RedisModuleCtx *ctx, int pos);
 unsigned long long REDISMODULE_API_FUNC(RedisModule_GetClientId)(RedisModuleCtx *ctx);
-int REDISMODULE_API_FUNC(RedisModule_GetContextFlags)(RedisModuleCtx *ctx);
 void *REDISMODULE_API_FUNC(RedisModule_PoolAlloc)(RedisModuleCtx *ctx, size_t bytes);
 RedisModuleType *REDISMODULE_API_FUNC(RedisModule_CreateDataType)(RedisModuleCtx *ctx, const char *name, int encver, RedisModuleTypeMethods *typemethods);
 int REDISMODULE_API_FUNC(RedisModule_ModuleTypeSetValue)(RedisModuleKey *key, RedisModuleType *mt, void *value);
@@ -327,7 +302,6 @@
     REDISMODULE_GET_API(IsKeysPositionRequest);
     REDISMODULE_GET_API(KeyAtPos);
     REDISMODULE_GET_API(GetClientId);
-    REDISMODULE_GET_API(GetContextFlags);
     REDISMODULE_GET_API(PoolAlloc);
     REDISMODULE_GET_API(CreateDataType);
     REDISMODULE_GET_API(ModuleTypeSetValue);
