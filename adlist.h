--- redis/src/adlist.h	2017-10-17 08:40:43.034522630 -0500
+++ ApsaraCache/src/adlist.h	2017-10-17 08:39:01.018524457 -0500
@@ -75,6 +75,7 @@
 void listEmpty(list *list);
 list *listAddNodeHead(list *list, void *value);
 list *listAddNodeTail(list *list, void *value);
+listNode *listAddNodeTailReturnNode(list *list, void *value);
 list *listInsertNode(list *list, listNode *old_node, void *value, int after);
 void listDelNode(list *list, listNode *node);
 listIter *listGetIterator(list *list, int direction);
