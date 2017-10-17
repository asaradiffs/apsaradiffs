--- redis/src/util.c	2017-10-17 08:40:43.094522629 -0500
+++ ApsaraCache/src/util.c	2017-10-17 08:39:01.050524456 -0500
@@ -40,7 +40,7 @@
 #include <stdint.h>
 #include <errno.h>
 
-#include "util.h"
+#include "server.h"
 #include "sha1.h"
 
 /* Glob-style pattern matching. */
@@ -274,13 +274,13 @@
  *
  * Modified in order to handle signed integers since the original code was
  * designed for unsigned integers. */
+static const char digits[201] =
+    "0001020304050607080910111213141516171819"
+    "2021222324252627282930313233343536373839"
+    "4041424344454647484950515253545556575859"
+    "6061626364656667686970717273747576777879"
+    "8081828384858687888990919293949596979899";
 int ll2string(char *dst, size_t dstlen, long long svalue) {
-    static const char digits[201] =
-        "0001020304050607080910111213141516171819"
-        "2021222324252627282930313233343536373839"
-        "4041424344454647484950515253545556575859"
-        "6061626364656667686970717273747576777879"
-        "8081828384858687888990919293949596979899";
     int negative;
     unsigned long long value;
 
@@ -328,6 +328,29 @@
     return length;
 }
 
+unsigned ull2string(char *dst, size_t dstlen, uint64_t value) {
+    uint32_t const length = digits10(value);
+    if (length > dstlen) return 0;
+    uint32_t next = length - 1;
+    while (value >= 100) {
+        int const i = (value % 100) * 2;
+        value /= 100;
+        dst[next] = digits[i + 1];
+        dst[next - 1] = digits[i];
+        next -= 2;
+    }
+    // Handle last 1-2 digits
+    if (value < 10) {
+        dst[next] = '0' + (uint32_t)value;
+    } else {
+        int i = (uint32_t)value * 2;
+        dst[next] = digits[i + 1];
+        dst[next - 1] = digits[i];
+    }
+    return length;
+}
+
+
 /* Convert a string into a long long. Returns 1 if the string could be parsed
  * into a (non-overflowing) long long, 0 otherwise. The value will be set to
  * the parsed value when appropriate.
@@ -673,6 +696,110 @@
     return strchr(path,'/') == NULL && strchr(path,'\\') == NULL;
 }
 
+/* Avoid warnings on solaris, where isspace() is an index into an array, and gcc uses signed chars */
+#define xisspace(c) isspace((unsigned char)c)
+
+int string2d(const char *str, long double *value) {
+    char *end;
+    *value = strtold(str, &end);
+    if (errno == ERANGE || str == end) {
+        return 0;
+    }
+
+    if (xisspace(*end) || (*end == '\0' && str != end))
+        return 1;
+    return 0;
+}
+/*
+ * change a null terminated string to uint64_t 
+ * one uncommon condition is checked:
+ * for a singal optional -, but with a so big value that
+ * it's negative as a singed number
+ */
+int safe_strtoull(const char *str, uint64_t *out) {
+    errno = 0;
+    *out = 0;
+    char *endptr;
+    unsigned long long ull = strtoull(str, &endptr, 10);
+    if ((errno == ERANGE) || (str == endptr)) {
+        return C_ERR;
+    }
+
+    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
+        if ((long long) ull < 0) {
+            /* only check for negative signs in the uncommon case when
+             * the unsigned number is so big that it's negative as a
+             * signed number. */
+            if (strchr(str, '-') != NULL) {
+                return C_ERR;
+            }
+        }
+        *out = ull;
+        return C_OK;
+    }
+    return C_ERR;
+}
+
+int safe_strtoll(const char *str, int64_t *out) {
+    errno = 0;
+    *out = 0;
+    char *endptr;
+    long long ll = strtoll(str, &endptr, 10);
+    if ((errno == ERANGE) || (str == endptr)) {
+        return C_ERR;
+    }
+
+    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
+        *out = ll;
+        return C_OK;
+    }
+    return C_ERR;
+}
+
+int safe_strtoul(const char *str, uint32_t *out) {
+    char *endptr = NULL;
+    unsigned long l = 0;
+    *out = 0;
+    errno = 0;
+
+    l = strtoul(str, &endptr, 10);
+    if ((errno == ERANGE) || (str == endptr)) {
+        return C_ERR;
+    }
+
+    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
+        if ((long) l < 0) {
+            /* only check for negative signs in the uncommon case when
+             * the unsigned number is so big that it's negative as a
+             * signed number. */
+            if (strchr(str, '-') != NULL) {
+                return C_ERR;
+            }
+        }
+        *out = l;
+        return C_OK;
+    }
+
+    return C_ERR;
+}
+
+int safe_strtol(const char *str, int32_t *out) {
+    errno = 0;
+    *out = 0;
+    char *endptr;
+    long l = strtol(str, &endptr, 10);
+    if ((errno == ERANGE) || (str == endptr)) {
+        return C_ERR;
+    }
+
+    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
+        *out = l;
+        return C_OK;
+    }
+    return C_ERR;
+}
+
+
 #ifdef REDIS_TEST
 #include <assert.h>
 
