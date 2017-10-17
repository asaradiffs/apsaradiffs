--- redis/src/adlist.c	2017-10-17 08:40:43.034522630 -0500
+++ ApsaraCache/src/adlist.c	2017-10-17 08:39:01.018524457 -0500
@@ -131,6 +131,32 @@
     return list;
 }
 
+/* Add a new node to the list, to tail, containing the specified 'value'
+ * pointer as value.
+ *
+ * On error, NULL is returned and no operation is performed (i.e. the
+ * list remains unaltered).
+ * On success the 'listNode' pointer point to the value you pass to the function is returned. */
+listNode *listAddNodeTailReturnNode(list *list, void *value)
+{
+    listNode *node;
+
+    if ((node = zmalloc(sizeof(*node))) == NULL)
+        return NULL;
+    node->value = value;
+    if (list->len == 0) {
+        list->head = list->tail = node;
+        node->prev = node->next = NULL;
+    } else {
+        node->prev = list->tail;
+        node->next = NULL;
+        list->tail->next = node;
+        list->tail = node;
+    }
+    list->len++;
+    return node;
+}
+
 list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
     listNode *node;
 
