--- redis/src/aof.c	2017-10-17 08:40:43.038522630 -0500
+++ ApsaraCache/src/aof.c	2017-10-17 08:39:01.018524457 -0500
@@ -542,8 +542,13 @@
         /* Translate SET [EX seconds][PX milliseconds] to SET and PEXPIREAT */
         buf = catAppendOnlyGenericCommand(buf,3,argv);
         for (i = 3; i < argc; i ++) {
-            if (!strcasecmp(argv[i]->ptr, "ex")) exarg = argv[i+1];
-            if (!strcasecmp(argv[i]->ptr, "px")) pxarg = argv[i+1];
+            if (server.protocol == REDIS) {
+                if (!strcasecmp(argv[i]->ptr, "ex")) exarg = argv[i+1];
+                if (!strcasecmp(argv[i]->ptr, "px")) pxarg = argv[i+1];
+            } else {
+                if (argv[i]->encoding == OBJ_ENCODING_RAW && !strcasecmp(argv[i]->ptr, "ex")) exarg = argv[i+1];
+                if (argv[i]->encoding == OBJ_ENCODING_RAW && !strcasecmp(argv[i]->ptr, "px")) pxarg = argv[i+1];
+            }
         }
         serverAssert(!(exarg && pxarg));
         if (exarg)
