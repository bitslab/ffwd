typedef struct node {
	int value;
	struct node *child[2];
} node_t;

typedef struct rlu_tree {
	node_t *root;
} rlu_tree_t;

uint64_t ffwd_list_ins(int);
uint64_t ffwd_list_del(int);
uint64_t ffwd_list_find(int);

extern rlu_tree_t *tree;