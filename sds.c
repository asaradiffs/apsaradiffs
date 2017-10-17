--- redis/src/sds.c	2017-10-17 08:40:43.078522629 -0500
+++ ApsaraCache/src/sds.c	2017-10-17 08:39:01.042524456 -0500
@@ -239,6 +239,46 @@
     return s;
 }
 
+/* Allocate a sds string from a memcached buffer
+ */
+sds sdsfromMemcachedStringbuffer(const char *buf, size_t len, size_t cas_flag_size) {
+    void *sh;
+    int hdrlen, total;
+    char type;
+    sds s;
+
+    total = len + cas_flag_size;
+    type = sdsReqType(total);
+    if (type == SDS_TYPE_5) type = SDS_TYPE_8;
+
+    hdrlen = sdsHdrSize(type);
+    sh = s_malloc(hdrlen+total+1);
+    if (sh == NULL) return NULL;
+    memcpy((char*)sh+hdrlen+cas_flag_size, buf, len);
+    s = (char*)sh+hdrlen;
+    s[-1] = type;
+    s[total] = 0;
+    sdssetlen(s, total);
+    sdssetalloc(s, total);
+    return s;
+}
+
+sds sdsfromMemcachedSds(sds s, size_t cas_flag_size) {
+    size_t len;
+    char type;
+
+    len = sdslen(s);
+    len -= cas_flag_size;
+    type = s[-1];
+    
+    s = s + cas_flag_size;
+    s[-1] = type;
+    sdssetlen(s, len);
+    sdssetalloc(s, len);
+    return s;
+}
+
+
 /* Reallocate the sds string so that it has no free space at the end. The
  * contained string remains not altered, but next concatenation operations
  * will require a reallocation.
