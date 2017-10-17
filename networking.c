--- redis/src/networking.c	2017-10-17 08:40:43.058522629 -0500
+++ ApsaraCache/src/networking.c	2017-10-17 08:39:01.034524457 -0500
@@ -33,7 +33,7 @@
 #include <math.h>
 #include <ctype.h>
 
-static void setProtocolError(const char *errstr, client *c, int pos);
+void setProtocolError(const char *errstr, client *c, int pos);
 
 /* Return the size consumed from the allocator, for the specified SDS string,
  * including internal fragmentation. This function is used in order to compute
@@ -133,9 +133,10 @@
     c->pubsub_channels = dictCreate(&objectKeyPointerValueDictType,NULL);
     c->pubsub_patterns = listCreate();
     c->peerid = NULL;
+    c->binary_header.request.magic = 0;
     listSetFreeMethod(c->pubsub_patterns,decrRefCountVoid);
     listSetMatchMethod(c->pubsub_patterns,listMatchObjects);
-    if (fd != -1) listAddNodeTail(server.clients,c);
+    if (fd != -1) c->client_list_node = listAddNodeTailReturnNode(server.clients,c);
     initClientMultiState(c);
     return c;
 }
@@ -616,11 +617,13 @@
      * for this condition, since now the socket is already set in non-blocking
      * mode and we can send an error for free using the Kernel I/O */
     if (listLength(server.clients) > server.maxclients) {
-        char *err = "-ERR max number of clients reached\r\n";
+        if (server.protocol == REDIS) {
+            char *err = "-ERR max number of clients reached\r\n";
 
-        /* That's a best effort error message, don't check write errors */
-        if (write(c->fd,err,strlen(err)) == -1) {
-            /* Nothing to do, Just to avoid the warning... */
+            /* That's a best effort error message, don't check write errors */
+            if (write(c->fd,err,strlen(err)) == -1) {
+                /* Nothing to do, Just to avoid the warning... */
+            }
         }
         server.stat_rejected_conn++;
         freeClient(c);
@@ -743,9 +746,10 @@
      * fd is already set to -1. */
     if (c->fd != -1) {
         /* Remove from the list of active clients. */
-        ln = listSearchKey(server.clients,c);
-        serverAssert(ln != NULL);
-        listDelNode(server.clients,ln);
+        if (c->client_list_node) {
+            listDelNode(server.clients,c->client_list_node);
+            c->client_list_node = NULL;
+        }
 
         /* Unregister async I/O handlers and close the socket. */
         aeDeleteFileEvent(server.el,c->fd,AE_READABLE);
@@ -1107,7 +1111,7 @@
 /* Helper function. Trims query buffer to make the function that processes
  * multi bulk requests idempotent. */
 #define PROTO_DUMP_LEN 128
-static void setProtocolError(const char *errstr, client *c, int pos) {
+void setProtocolError(const char *errstr, client *c, int pos) {
     if (server.verbosity <= LL_VERBOSE) {
         sds client = catClientInfoString(sdsempty(),c);
 
@@ -1284,6 +1288,31 @@
     return C_ERR;
 }
 
+/* Because we will porcess redis and memcached protocol,
+ * so we create two functions.
+ */
+int processRedisProtocolBuffer(client *c) {
+    /* Determine request type when unknown. */
+    int ret = C_ERR;
+    if (!c->reqtype) {
+        if (c->querybuf[0] == '*') {
+            c->reqtype = PROTO_REQ_MULTIBULK;
+        } else {
+            c->reqtype = PROTO_REQ_INLINE;
+        }
+    }
+
+    if (c->reqtype == PROTO_REQ_INLINE) {
+        ret = processInlineBuffer(c);
+    } else if (c->reqtype == PROTO_REQ_MULTIBULK) {
+        ret = processMultibulkBuffer(c);
+    } else {
+        serverPanic("Unknown request type");
+    }
+    return ret;
+}
+
+
 /* This function is called every time, in the client structure 'c', there is
  * more query buffer to process, because we read more data from the socket
  * or because a client was blocked and later reactivated, so there could be
@@ -1305,21 +1334,12 @@
          * The same applies for clients we want to terminate ASAP. */
         if (c->flags & (CLIENT_CLOSE_AFTER_REPLY|CLIENT_CLOSE_ASAP)) break;
 
-        /* Determine request type when unknown. */
-        if (!c->reqtype) {
-            if (c->querybuf[0] == '*') {
-                c->reqtype = PROTO_REQ_MULTIBULK;
-            } else {
-                c->reqtype = PROTO_REQ_INLINE;
-            }
-        }
-
-        if (c->reqtype == PROTO_REQ_INLINE) {
-            if (processInlineBuffer(c) != C_OK) break;
-        } else if (c->reqtype == PROTO_REQ_MULTIBULK) {
-            if (processMultibulkBuffer(c) != C_OK) break;
-        } else {
-            serverPanic("Unknown request type");
+        int ret = server.protocolParseProcess(c);
+        if (ret == C_ERR) {
+            break;
+        } else if (ret == C_AGAIN) { 
+            resetClient(c);
+            continue;
         }
 
         /* Multibulk processing could see a <= 0 length. */
@@ -1363,7 +1383,7 @@
      * at the risk of requiring more read(2) calls. This way the function
      * processMultiBulkBuffer() can avoid copying buffers to create the
      * Redis Object representing the argument. */
-    if (c->reqtype == PROTO_REQ_MULTIBULK && c->multibulklen && c->bulklen != -1
+    if ((c->reqtype == PROTO_REQ_MULTIBULK || c->reqtype == PROTO_REQ_ASCII) && c->multibulklen && c->bulklen != -1
         && c->bulklen >= PROTO_MBULK_BIG_ARG)
     {
         int remaining = (unsigned)(c->bulklen+2)-sdslen(c->querybuf);
