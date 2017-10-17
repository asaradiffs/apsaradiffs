--- redis/src/endianconv.h	2017-10-17 08:40:43.050522630 -0500
+++ ApsaraCache/src/endianconv.h	2017-10-17 08:39:01.026524457 -0500
@@ -52,6 +52,8 @@
 #define intrev16ifbe(v) (v)
 #define intrev32ifbe(v) (v)
 #define intrev64ifbe(v) (v)
+#define ntohll(v) intrev64(v)
+#define htonll(v) intrev64(v)
 #else
 #define memrev16ifbe(p) memrev16(p)
 #define memrev32ifbe(p) memrev32(p)
@@ -59,6 +61,8 @@
 #define intrev16ifbe(v) intrev16(v)
 #define intrev32ifbe(v) intrev32(v)
 #define intrev64ifbe(v) intrev64(v)
+#define ntohll(v) (v)
+#define htonll(v) (v)
 #endif
 
 /* The functions htonu64() and ntohu64() convert the specified value to
