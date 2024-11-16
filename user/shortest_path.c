#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/proc_metrics.h"
#include "kernel/fcntl.h"

#define MIN_NODES           100
#define MAX_NODES           200
#define MAX_EDGE_WEIGHT     2000
#define NUM_GRAPHS          1000
#define MAX_NODES           200
#define MIN_EDGES           50
#define MAX_EDGES           400
#define INFINITY            0x7FFFFFFF
#define LEFT(i)             (i * 2 + 1)
#define RIGHT(i)            (i * 2 + 2)
#define PARENT(i)           ((i - 1) / 2)

int alloc_time = 0;
int free_time = 0;
int access_time = 0;

struct digraph
{
    int num_nodes;
    struct list **adj;
};

struct list_node
{
    int idx, val;
    struct list_node *next;
};

struct list
{
    int size;
    struct list_node *head;
};

struct heap_node
{
    int idx, val;
};

struct min_heap
{
    int size, max_size; // o size indica quantos elementos tem no heap
    struct heap_node **v;
    int *pos; // vetor com a posição j no vetor v do elemento i;
};

void malloc_successfull_or_panic(void *pointer)
{
    if (pointer == 0)
    {
        printf("Malloc failed\n");
        exit(1);
    }
}

struct list *new_list()
{
    // int time = uptime();
    struct list *list = malloc(sizeof(struct list));
    // alloc_time += uptime() - time;
    malloc_successfull_or_panic(list);

    list->size = 0;
    list->head = 0;

    return list;
}

struct list_node *new_list_node(int idx, int val)
{
    // int time = uptime();
    struct list_node *list_node = malloc(sizeof(struct list_node));
    // alloc_time += uptime() - time;

    malloc_successfull_or_panic(list_node);

    list_node->val = val;
    list_node->idx = idx;
    list_node->next = 0;

    return list_node;
}

struct min_heap *new_min_heap(int size)
{
    // int time = uptime();
    struct min_heap *heap = malloc(sizeof(struct min_heap));
    // alloc_time += uptime() - time;

    malloc_successfull_or_panic(heap);

    // time = uptime();
    heap->pos = malloc(sizeof(int) * size);
    // alloc_time += uptime() - time;
    malloc_successfull_or_panic(heap->pos);
    memset(heap->pos, -1, sizeof(int) * size);

    // time = uptime();
    heap->v = malloc(sizeof(struct heap_node *) * size);
    // alloc_time += uptime() - time;
    malloc_successfull_or_panic(heap->v);

    heap->size = 0;
    heap->max_size = size;

    return heap;
}

struct heap_node *new_heap_node(int idx, int val)
{
    // int time = uptime();
    struct heap_node *heap_node = malloc(sizeof(struct heap_node));
    // alloc_time += uptime() - time;
    malloc_successfull_or_panic(heap_node);

    heap_node->idx = idx;
    heap_node->val = val;

    return heap_node;
}

void swap_vals_on_heap(struct min_heap *heap, int i, int j)
{
    // int time = uptime();
    struct heap_node *aux = heap->v[j];

    int i_pos = heap->v[i]->idx, j_pos = heap->v[j]->idx;
    int idx_aux = heap->pos[i_pos];

    heap->v[j] = heap->v[i];
    heap->v[i] = aux;
    heap->pos[i_pos] = heap->pos[j_pos];
    heap->pos[j_pos] = idx_aux;
    // access_time += uptime() - time;
}

void climb_on_heap(struct min_heap *heap, int i)
{
    int p = PARENT(i);
    // int time = uptime();
    if (i > 0 && heap->v[p]->val > heap->v[i]->val)
    {
        // access_time += uptime() - time;
        swap_vals_on_heap(heap, i, p);
        climb_on_heap(heap, p);
    } else {
        // access_time += uptime() - time;
    }
}

void fall_on_heap(struct min_heap *heap, int i)
{
    int l = LEFT(i);
    int r = RIGHT(i);
    int smallest = i;
    // int time = uptime();
    if (l < heap->size && heap->v[smallest]->val > heap->v[l]->val)
        smallest = l;

    if (r < heap->size && heap->v[smallest]->val > heap->v[r]->val)
        smallest = r;
    // access_time += uptime() - time;

    if (smallest != i)
    {
        swap_vals_on_heap(heap, i, smallest);
        fall_on_heap(heap, smallest);
    }
}

void update_key_value(struct min_heap *min_heap, int idx, int new_val)
{
    if (idx >= min_heap->size || idx < 0)
    {
        printf("Heap index out of range\n");
        exit(2);
    }
    int l = LEFT(idx);
    int r = RIGHT(idx);
    int p = PARENT(idx);
    // int time = uptime();
    min_heap->v[idx]->val = new_val;

    if (idx > 0 && new_val < min_heap->v[p]->val) {
        // access_time += uptime() - time;
        climb_on_heap(min_heap, idx);
    } 
    else if ((l < min_heap->size && new_val > min_heap->v[l]->val) ||
             (r < min_heap->size && new_val > min_heap->v[r]->val)) {
        // access_time += uptime() - time;
        fall_on_heap(min_heap, idx);
    }
    else {
        // access_time += uptime() - time;
    }
}

void add_heap_node(struct min_heap *heap, int idx, int val)
{
    // int time = uptime();
    if (heap->size == heap->max_size)
    {
        printf("Cannot add value to heap :C\n");
        exit(2);
    }

    if (heap->pos[idx] != -1)
    {
        printf("Cannot add the same node twice to heap :C\n");
        exit(2);
    }

    struct heap_node *node = new_heap_node(idx, val);
    heap->v[heap->size] = node;
    heap->pos[idx] = heap->size;
    // access_time += uptime() - time;
    climb_on_heap(heap, heap->size++);
}

struct heap_node *pop_heap(struct min_heap *heap)
{
    // int time = uptime();
    if (!heap->size)
    {
        printf("Cannot extract values from empty heap.\n");
        exit(2);
    }

    struct heap_node *node = heap->v[0];

    heap->v[0] = heap->v[--heap->size];
    heap->pos[heap->v[0]->idx] = 0;
    heap->v[heap->size] = 0;
    // access_time += uptime() - time;

    fall_on_heap(heap, 0);

    // time = uptime();
    heap->pos[node->idx] = -1;
    // access_time += uptime() - time;

    return node;
}

void free_heap_node(struct heap_node *heap_node)
{
    // int time = uptime();
    free(heap_node);
    // free_time += uptime() - time;
}

void free_min_heap(struct min_heap *heap)
{
    for (int i = 0; i < heap->size; i++)
        free_heap_node(heap->v[i]);
    // int time = uptime();
    free(heap->v);
    free(heap->pos);
    free(heap);
    // free_time += uptime() - time;
}

void free_digraph(struct digraph *digraph)
{
    struct list_node *list_node, *list_node_aux;
    // int time;
    for (int i = 0; i < digraph->num_nodes; i++)
    {
        // time = uptime();
        list_node = digraph->adj[i]->head;
        // access_time += uptime() - time;

        while (list_node)
        {
            list_node_aux = list_node;
            list_node = list_node->next;
            // time = uptime();
            free(list_node_aux);
            // free_time += uptime() - time;
        }
        // time = uptime();
        free(digraph->adj[i]);
        // free_time += uptime() - time;
    }
    // time = uptime();
    free(digraph->adj);
    // free_time += uptime() - time;

    // time = uptime();
    free(digraph);
    // free_time += uptime() - time;
}

struct digraph *generate_random_digraph(int num_nodes, int num_edges, int *u, int *v)
{
    if (num_edges > num_nodes * (num_nodes - 1))
    {
        printf("A graph cannot have |E| > |V| * (|V| - 1)");
        exit(2);
    }

    struct digraph *digraph;
    int u_ = -1, v_ = -1;

    // int time = uptime();
    int **is_edge = malloc(num_nodes * sizeof(int *));
    // alloc_time += uptime() - time;

    malloc_successfull_or_panic(is_edge);
    for (int i = 0; i < num_nodes; i++)
    {
        // time = uptime();
        is_edge[i] = malloc(num_nodes * sizeof(int));
        // alloc_time += uptime() - time;
        malloc_successfull_or_panic(is_edge[i]);
        memset(is_edge[i], 0, sizeof(int) * num_nodes);
    }

    // time = uptime();
    digraph = malloc(sizeof(struct digraph));
    // alloc_time += uptime() - time;
    malloc_successfull_or_panic(digraph);

    digraph->num_nodes = num_nodes;
    
    // time = uptime();
    digraph->adj = malloc(sizeof(struct list *) * num_nodes);
    // alloc_time += uptime() - time;
    malloc_successfull_or_panic(digraph->adj);

    for (int i = 0; i < num_nodes; i++)
        digraph->adj[i] = new_list();

    for (int i = 0; i < num_edges; i++)
    {
        int rand_u, rand_v, max_trials = 10;
        struct list_node *new_node;

        do
        {
            rand_u = rand_() % num_nodes;
            rand_v = rand_() % num_nodes;

            while (rand_v == rand_u)
                rand_v = rand_() % num_nodes;
            max_trials--;
            if (max_trials == -1)
                break;
        } while (is_edge[rand_u][rand_v] != 0);

        if (max_trials == -1)
            continue;
        
        // time = uptime();
        is_edge[rand_u][rand_v] = 1;
        // access_time += uptime() - time;
        new_node = new_list_node(rand_v, rand_() % MAX_EDGE_WEIGHT);

        // time = uptime();
        new_node->next = digraph->adj[rand_u]->head;
        digraph->adj[rand_u]->head = new_node;
        digraph->adj[rand_u]->size++;
        // access_time += uptime() - time;

        if (u_ == -1)
        {
            *u = rand_u;
            u_ = 1;
        }
        else if (v_ == -1 && rand_u != *u && !is_edge[*u][rand_u] && !is_edge[rand_u][*u])
        {
            *v = rand_u;
            v_ = 1;
        }
    }

    for (int i = 0; i < num_nodes; i++) {
        // time = uptime();
        free(is_edge[i]);
        // free_time += uptime() - time;
    }
    // time = uptime();
    free(is_edge);
    // free_time += uptime() - time;

    return digraph;
}

int dijkstra(struct digraph *g, int u, int v, int *path)
{
    struct min_heap *min_heap = new_min_heap(g->num_nodes);

    for (int i = 0; i < min_heap->max_size; i++)
        if (i == u)
            add_heap_node(min_heap, i, 0);
        else
            add_heap_node(min_heap, i, INFINITY);

    while (min_heap->size)
    {
        struct heap_node *heap_node = pop_heap(min_heap);
        struct list_node *list_node;

        if (heap_node->idx == v)
        {
            int dist = heap_node->val;
            free_heap_node(heap_node);
            free_min_heap(min_heap);
            return dist;
        }

        for (list_node = g->adj[heap_node->idx]->head; list_node; list_node = list_node->next)
        {
            int pos, relaxed_val;
            // int time = uptime();
            pos = min_heap->pos[list_node->idx];
            relaxed_val = list_node->val + heap_node->val;
            if (pos >= 0 && min_heap->v[pos]->val > relaxed_val)
            {
                path[min_heap->v[pos]->idx] = heap_node->idx;
                // access_time += uptime() - time;
                update_key_value(min_heap, pos, relaxed_val);
            } else {
                // access_time += uptime() - time;
            }
        }
        free_heap_node(heap_node);
    }
    free_min_heap(min_heap);
    return INFINITY;
}

void print_path(int u, int v, int *path)
{
    if (u == v)
    {
        printf("%d-", u);
        return;
    }
    print_path(u, path[v], path);
    printf("%d-", v);
}

void solve()
{
    int num_nodes, num_edges, u, v, *path;
    struct digraph *digraph;

    num_nodes = rand_() % (MAX_NODES - MIN_NODES) + MIN_NODES;
    num_edges = rand_() % (MAX_EDGES - MIN_EDGES) + MIN_EDGES;

    // int time = uptime();
    path = malloc(sizeof(uint) * num_nodes);
    // alloc_time += uptime() - time;
    memset(path, -1, sizeof(uint) * num_nodes);
    malloc_successfull_or_panic(path);

    // u = rand_() % num_nodes;
    // do
    //     v = rand_() % num_nodes;
    // while (u == v && num_nodes > 1);

    digraph = generate_random_digraph(num_nodes, num_edges, &u, &v);
    dijkstra(digraph, u, v, path);

    // if (dijkstra(digraph, u, v, path) == INFINITY)
    //     printf("There is no path from (%d) to (%d)\n", u, v);
    // else
    //     print_path(u, v, path);
    // time = uptime();
    free(path);
    // free_time += uptime() - time;
    free_digraph(digraph);
}

int main(int argc, char *argv[])
{
    if (argc != 2 && argc != 3)
    {
        printf("Invalid parameters.\n");
        exit(1);
    }

    srand_(atoi(argv[1]));
    for (int i = 0; i < NUM_GRAPHS; i++)
        solve();

    // printf("access_time: %d\n", access_time);
    // printf("alloc_time: %d\n", alloc_time);
    // printf("free_time: %d\n", free_time);
    exit(0);
}
