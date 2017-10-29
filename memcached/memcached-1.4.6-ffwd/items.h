/* See items.c */
uint64_t get_cas_id(void);

/*@null@*/
item *do_item_alloc_del(char *key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes);
void item_free_del(item *it);
bool item_size_ok(const size_t nkey, const int flags, const int nbytes);

int  do_item_link_del(item *it);     /** may fail if transgresses limits */
void do_item_unlink_del(item *it);
void do_item_remove_del(item *it);
void do_item_update(item *it);   /** update LRU time to current and reposition */
int  do_item_replace_del(item *it, item *new_it);

/*@null@*/
char *do_item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
void do_item_stats_del(ADD_STAT add_stats, void *c);
/*@null@*/
void do_item_stats_sizes_del(ADD_STAT add_stats, void *c);
void do_item_flush_expired_del(void);

item *do_item_get_del(const char *key, const size_t nkey);
item *do_item_get_nocheck(const char *key, const size_t nkey);
void item_stats_reset(void);
extern pthread_mutex_t cache_lock;
