--- redis/src/object.c	2017-10-17 08:40:43.058522629 -0500
+++ ApsaraCache/src/object.c	2017-10-17 08:39:01.034524457 -0500
@@ -393,6 +393,11 @@
      * they are not handled. We handle them only as values in the keyspace. */
      if (o->refcount > 1) return o;
 
+    /* When recover data from aof, don't let the fake client command do INT encoding */
+    if (server.protocol == MEMCACHED) {
+        return o;
+    }
+
     /* Check if we can represent this string as a long integer.
      * Note that we are sure that a string larger than 20 chars is not
      * representable as a 32 nor 64 bit integer. */
