--- redis/src/rax.c	2017-10-17 08:40:43.062522629 -0500
+++ ApsaraCache/src/rax.c	2017-10-17 08:39:01.038524456 -0500
@@ -186,10 +186,10 @@
 void raxSetData(raxNode *n, void *data) {
     n->iskey = 1;
     if (data != NULL) {
+        n->isnull = 0;
         void **ndata = (void**)
             ((char*)n+raxNodeCurrentLength(n)-sizeof(void*));
         memcpy(ndata,&data,sizeof(data));
-        n->isnull = 0;
     } else {
         n->isnull = 1;
     }
@@ -396,6 +396,7 @@
                   position to 0 to signal this node represents
                   the searched key. */
     }
+    debugnode("Lookup stop node is",h);
     if (stopnode) *stopnode = h;
     if (plink) *plink = parentlink;
     if (splitpos && h->iscompr) *splitpos = j;
@@ -424,18 +425,21 @@
      * our key. We have just to reallocate the node and make space for the
      * data pointer. */
     if (i == len && (!h->iscompr || j == 0 /* not in the middle if j is 0 */)) {
+        debugf("### Insert: node representing key exists\n");
+        if (!h->iskey || h->isnull) {
+            h = raxReallocForData(h,data);
+            if (h) memcpy(parentlink,&h,sizeof(h));
+        }
+        if (h == NULL) {
+            errno = ENOMEM;
+            return 0;
+        }
         if (h->iskey) {
             if (old) *old = raxGetData(h);
             raxSetData(h,data);
             errno = 0;
             return 0; /* Element already exists. */
         }
-        h = raxReallocForData(h,data);
-        if (h == NULL) {
-            errno = ENOMEM;
-            return 0;
-        }
-        memcpy(parentlink,&h,sizeof(h));
         raxSetData(h,data);
         rax->numele++;
         return 1; /* Element inserted. */
@@ -734,9 +738,7 @@
     }
 
     /* We walked the radix tree as far as we could, but still there are left
-     * chars in our string. We need to insert the missing nodes.
-     * Note: while loop never entered if the node was split by ALGO2,
-     * since i == len. */
+     * chars in our string. We need to insert the missing nodes. */
     while(i < len) {
         raxNode *child;
 
@@ -1091,6 +1093,7 @@
 /* This is the core of raxFree(): performs a depth-first scan of the
  * tree and releases all the nodes found. */
 void raxRecursiveFree(rax *rax, raxNode *n) {
+    debugnode("free traversing",n);
     int numchildren = n->iscompr ? 1 : n->size;
     raxNode **cp = raxNodeLastChildPtr(n);
     while(numchildren--) {
