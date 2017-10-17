--- redis/src/redis-cli.c	2017-10-17 08:40:43.070522629 -0500
+++ ApsaraCache/src/redis-cli.c	2017-10-17 08:39:01.038524456 -0500
@@ -1806,6 +1806,7 @@
     }
     close(s); /* Close the file descriptor ASAP as fsync() may take time. */
     fsync(fd);
+    close(fd);
     fprintf(stderr,"Transfer finished with success.\n");
     exit(0);
 }
