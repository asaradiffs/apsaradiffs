--- redis/src/replication.c	2017-10-17 08:40:43.074522629 -0500
+++ ApsaraCache/src/replication.c	2017-10-17 08:39:01.042524456 -0500
@@ -2205,7 +2205,7 @@
     server.repl_state = REPL_STATE_CONNECTED;
 
     /* Re-add to the list of clients. */
-    listAddNodeTail(server.clients,server.master);
+    server.master->client_list_node = listAddNodeTailReturnNode(server.clients,server.master);
     if (aeCreateFileEvent(server.el, newfd, AE_READABLE,
                           readQueryFromClient, server.master)) {
         serverLog(LL_WARNING,"Error resurrecting the cached master, impossible to add the readable handler: %s", strerror(errno));
