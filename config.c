--- redis/src/config.c	2017-10-17 08:40:43.042522630 -0500
+++ ApsaraCache/src/config.c	2017-10-17 08:39:01.022524457 -0500
@@ -726,6 +726,9 @@
                 err = sentinelHandleConfiguration(argv+1,argc-1);
                 if (err) goto loaderr;
             }
+        } else if (!strcasecmp(argv[0], "protocol") && argc == 2) {
+            //directive: protocol memcached
+            server.protocol = (!strcasecmp(argv[1], "memcached") || !strcasecmp(argv[1], "memcache")) ? MEMCACHED : REDIS;
         } else {
             err = "Bad directive or wrong number of arguments"; goto loaderr;
         }
@@ -1217,6 +1220,7 @@
     config_get_string_field("logfile",server.logfile);
     config_get_string_field("pidfile",server.pidfile);
     config_get_string_field("slave-announce-ip",server.slave_announce_ip);
+    config_get_string_field("protocol",(char *)server.protocol_str);
 
     /* Numerical values */
     config_get_numerical_field("maxmemory",server.maxmemory);
