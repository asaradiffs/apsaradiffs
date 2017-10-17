--- redis/src/server.h	2017-10-17 08:40:43.082522629 -0500
+++ ApsaraCache/src/server.h	2017-10-17 08:39:01.046524456 -0500
@@ -72,10 +72,12 @@
 #include "sha1.h"
 #include "endianconv.h"
 #include "crc64.h"
+#include "memcached_binary_define.h"
 
 /* Error codes */
 #define C_OK                    0
 #define C_ERR                   -1
+#define C_AGAIN                 -2
 
 /* Static server configuration */
 #define CONFIG_DEFAULT_HZ        10      /* Time interrupt calls/sec. */
@@ -248,6 +250,7 @@
 #define CLIENT_LUA_DEBUG (1<<25)  /* Run EVAL in debug mode. */
 #define CLIENT_LUA_DEBUG_SYNC (1<<26)  /* EVAL debugging without fork() */
 #define CLIENT_MODULE (1<<27) /* Non connected client used by some module. */
+#define CLIENT_NO_REPLY (1 << 28) /* Client need no response*/
 
 /* Client block type (btype field in client structure)
  * if CLIENT_BLOCKED flag is set. */
@@ -256,9 +259,11 @@
 #define BLOCKED_WAIT 2    /* WAIT for synchronous replication. */
 #define BLOCKED_MODULE 3  /* Blocked by a loadable module. */
 
-/* Client request types */
+/* Client request types, add memcached text and binary request protocol */
 #define PROTO_REQ_INLINE 1
 #define PROTO_REQ_MULTIBULK 2
+#define PROTO_REQ_ASCII 3
+#define PROTO_REQ_BINARY 4
 
 /* Client classes for client limits, currently used only for
  * the max-client-output-buffer limit implementation. */
@@ -670,6 +675,44 @@
     robj *key;
 } readyList;
 
+/* a redis instance runs in one of the follow modes:
+ * redis mode, or memcached mode; default is the redis mode */
+typedef enum runProtocol {
+    REDIS = 0x0,
+    MEMCACHED
+} runProtocol;
+
+#define PROTOCOL_REDIS_STR "REDIS"
+#define PROTOCOL_MEMCACHED_STR "MEMCACHED"
+#define USER_PASSWORD_FREE 0x1
+
+#define MEMCACHED_VERSION "1.4.33"
+/* reply wrapper
+ */
+struct client;
+typedef void (*replyWrapper)(struct client *c, const char *err_str);
+struct helpWrapper {
+    replyWrapper replyQuit;
+    replyWrapper replyDisableRWErr;
+    replyWrapper replyNoAuthErr;
+    replyWrapper replyOomErr;
+    replyWrapper replyWritingAofErr;
+    replyWrapper replyOpenningAofErr;
+    replyWrapper replyBgsaveErr;
+    replyWrapper replyNoreplicasErr;
+    replyWrapper replyRoslaveErr;
+    replyWrapper replyReadOnlyErr;
+    replyWrapper replyMasterdownErr;
+    replyWrapper replyLoadingErr;
+    replyWrapper replyUnknownCommandErr;
+};
+struct redisServer;
+void initHelpWrappers(struct redisServer *server, struct helpWrapper *wrappers);
+/*
+ * protocol parse handler
+ */
+typedef int protocolParseProc(struct client *c);
+
 /* With multiplexing we need to take per-client state.
  * Clients are taken in a linked list. */
 typedef struct client {
@@ -688,6 +731,7 @@
     int reqtype;            /* Request protocol type: PROTO_REQ_* */
     int multibulklen;       /* Number of multi bulk arguments left to read. */
     long bulklen;           /* Length of bulk argument in multi bulk request. */
+    memcachedBinaryRequestHeader binary_header; /* this is where the memcached binary header goes*/
     list *reply;            /* List of reply objects to send to the client. */
     unsigned long long reply_bytes; /* Tot bytes of objects in reply list. */
     size_t sentlen;         /* Amount of bytes already sent in the current
@@ -722,6 +766,7 @@
     dict *pubsub_channels;  /* channels a client is interested in (SUBSCRIBE) */
     list *pubsub_patterns;  /* patterns a client is interested in (SUBSCRIBE) */
     sds peerid;             /* Cached peer ID. */
+    listNode *client_list_node; /* list node in client list */
 
     /* Response buffer */
     int bufpos;
@@ -755,6 +800,19 @@
     sds minstring, maxstring;
 };
 
+/*memcached text shared objects*/
+struct sharedMemcachedObjectsStruct {
+    robj *crlf, *ok, *err, *noautherr, *norw, *emptybulk,*oomerr,
+         *interr, *bgsaveerr, *noreplicaserr, *roslaveerr, *roerr,
+         *masterdownerr, *loadingerr, *version, *vercmd, *quitcmd,
+         *noopcmd, *badfmt, *stored, *ex, *end, *invalidexp, *touched,
+         *not_found, *exists, *not_stored, *invalid_numeric, *deleted,
+         *invalid_delta, *px, *setcmd, *flushallcmd, *delcmd, *expirecmd,
+         *persistcmd, *invalid_pw_or_uname, *sasl_list_cmd, *too_large,
+         *not_in_white_list_err;
+};
+
+
 /* ZSETs use a specialized version of Skiplists */
 typedef struct zskiplistNode {
     sds ele;
@@ -880,6 +938,8 @@
     redisDb *db;
     dict *commands;             /* Command table */
     dict *orig_commands;        /* Command table before command renaming. */
+    dict *memcached_commands;   /* memcached command table*/
+    uint32_t max_memcached_read_request_length; /* the max read size of key + value */
     aeEventLoop *el;
     unsigned int lruclock;      /* Clock for LRU eviction */
     int shutdown_asap;          /* SHUTDOWN needed ASAP */
@@ -1100,6 +1160,11 @@
     int slave_priority;             /* Reported in INFO and used by Sentinel. */
     int slave_announce_port;        /* Give the master this listening port. */
     char *slave_announce_ip;        /* Give the master this ip address. */
+
+    runProtocol protocol; /* instance run mode */
+    const char *protocol_str;   /* run mode str: redis or memcached */
+    protocolParseProc *protocolParseProcess;
+
     /* The following two fields is where we store master PSYNC replid/offset
      * while the PSYNC is in progress. At the end we'll copy the fields into
      * the server->master client structure. */
@@ -1224,6 +1289,19 @@
     long long microseconds, calls;
 };
 
+/*just deal 18 memcached commands*/
+#define MEMCACHED_TEXT_REQUEST_NUM 16
+#define MEMCACHED_BINARY_REQUEST_NUM 35
+#define MEMCACHED_MAX_OPCODE 0x22
+/*define memcached text request parser help function*/
+typedef int textParserHelperProc(struct client *c, const size_t len, void *tokens, const size_t ntokens, const int hint, const int pos);
+struct memcachedCommand {
+    struct redisCommand command;
+    textParserHelperProc *proc;
+    uint8_t opcode;
+    uint8_t has_cas;
+};
+
 struct redisFunctionSym {
     char *name;
     unsigned long pointer;
@@ -1287,6 +1365,7 @@
 
 extern struct redisServer server;
 extern struct sharedObjectsStruct shared;
+extern struct sharedMemcachedObjectsStruct memcached_shared;
 extern dictType objectKeyPointerValueDictType;
 extern dictType setDictType;
 extern dictType zsetDictType;
@@ -1339,6 +1418,8 @@
 void *addDeferredMultiBulkLength(client *c);
 void setDeferredMultiBulkLength(client *c, void *node, long length);
 void processInputBuffer(client *c);
+int  processRedisProtocolBuffer(client *c);
+int  processMemcachedProtocolBuffer(client *c);
 void acceptHandler(aeEventLoop *el, int fd, void *privdata, int mask);
 void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
 void acceptUnixHandler(aeEventLoop *el, int fd, void *privdata, int mask);
@@ -1604,6 +1685,8 @@
 struct redisCommand *lookupCommand(sds name);
 struct redisCommand *lookupCommandByCString(char *s);
 struct redisCommand *lookupCommandOrOriginal(sds name);
+struct memcachedCommand *lookupMemcachedCommand(sds name);
+struct memcachedCommand *lookupMemcachedCommandByCString(char *name);
 void call(client *c, int flags);
 void propagate(struct redisCommand *cmd, int dbid, robj **argv, int argc, int flags);
 void alsoPropagate(struct redisCommand *cmd, int dbid, robj **argv, int argc, int target);
