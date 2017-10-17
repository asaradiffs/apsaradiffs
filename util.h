--- redis/src/util.h	2017-10-17 08:40:43.094522629 -0500
+++ ApsaraCache/src/util.h	2017-10-17 08:39:01.050524456 -0500
@@ -46,6 +46,21 @@
 int ld2string(char *buf, size_t len, long double value, int humanfriendly);
 sds getAbsolutePath(char *filename);
 int pathIsBaseName(char *path);
+unsigned ull2string(char *s, size_t slen, uint64_t value);
+
+/* Wrappers around strtoull/strtoll that are safer and easier to
+ * use.  For tests and assumptions, see internal_tests.c.
+ *
+ * str   a NULL-terminated base decimal 10 unsigned integer
+ * out   out parameter, if conversion succeeded
+ *
+ * returns true if conversion succeeded.
+ */
+int safe_strtoull(const char *str, uint64_t *out);
+int safe_strtoll(const char *str, int64_t *out);
+int safe_strtoul(const char *str, uint32_t *out);
+int safe_strtol(const char *str, int32_t *out);
+
 
 #ifdef REDIS_TEST
 int utilTest(int argc, char **argv);
