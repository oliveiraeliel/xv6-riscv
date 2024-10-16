#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#define MAX_NODES 200
#define MAX_EDGE_WEIGHT 500
#define INFINITY 1000000
#define LEFT(i) (i * 2 + 1)
#define RIGHT(i) (i * 2 + 2)
#define PARENT(i) ((i - 1) / 2)

static unsigned int seed = 0;

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
    struct list *list = malloc(sizeof(struct list));
    malloc_successfull_or_panic(list);

    list->size = 0;
    list->head = 0;

    return list;
}

struct list_node *new_list_node(int idx, int val)
{
    struct list_node *list_node = malloc(sizeof(struct list_node));
    malloc_successfull_or_panic(list_node);

    list_node->val = val;
    list_node->idx = idx;
    list_node->next = 0;

    return list_node;
}

struct min_heap *new_min_heap(int size)
{
    struct min_heap *heap = malloc(sizeof(struct min_heap));
    malloc_successfull_or_panic(heap);

    heap->pos = malloc(sizeof(int) * size);
    malloc_successfull_or_panic(heap->pos);
    memset(heap->pos, -1, sizeof(int) * size);

    heap->v = malloc(sizeof(struct heap_node *) * size);
    malloc_successfull_or_panic(heap->v);

    heap->size = 0;
    heap->max_size = size;

    return heap;
}

struct heap_node *new_heap_node(int idx, int val)
{
    struct heap_node *heap_node = malloc(sizeof(struct heap_node));
    malloc_successfull_or_panic(heap_node);

    heap_node->idx = idx;
    heap_node->val = val;

    return heap_node;
}

void swap_vals_on_heap(struct min_heap *heap, int i, int j)
{
    struct heap_node *aux = heap->v[j];
    int i_pos = heap->v[i]->idx, j_pos = heap->v[j]->idx;
    int idx_aux = heap->pos[i_pos];
    heap->v[j] = heap->v[i];
    heap->v[i] = aux;
    heap->pos[i_pos] = heap->pos[j_pos];
    heap->pos[j_pos] = idx_aux;
}

void climb_on_heap(struct min_heap *heap, int i)
{
    int p = PARENT(i);
    if (i > 0 && heap->v[p]->val > heap->v[i]->val)
    {
        swap_vals_on_heap(heap, i, p);
        climb_on_heap(heap, p);
    }
}

void fall_on_heap(struct min_heap *heap, int i)
{
    int l = LEFT(i);
    int r = RIGHT(i);
    int smallest = i;
    if (l < heap->size && heap->v[smallest]->val > heap->v[l]->val)
        smallest = l;

    if (r < heap->size && heap->v[smallest]->val > heap->v[r]->val)
        smallest = r;

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

    min_heap->v[idx]->val = new_val;

    if (idx > 0 && new_val < min_heap->v[p]->val)
        climb_on_heap(min_heap, idx);
    else if ((l < min_heap->size && new_val > min_heap->v[l]->val) ||
             (r < min_heap->size && new_val > min_heap->v[r]->val))
        fall_on_heap(min_heap, idx);
}

void add_heap_node(struct min_heap *heap, int idx, int val)
{
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
    climb_on_heap(heap, heap->size++);
}

struct heap_node *pop_heap(struct min_heap *heap)
{
    if (!heap->size)
    {
        printf("Cannot extract values from empty heap.\n");
        exit(2);
    }

    struct heap_node *node = heap->v[0];

    heap->v[0] = heap->v[--heap->size];
    heap->pos[heap->v[0]->idx] = 0;
    heap->v[heap->size] = 0;

    fall_on_heap(heap, 0);

    heap->pos[node->idx] = -1;

    return node;
}

void free_heap_node(struct heap_node *heap_node)
{
    free(heap_node);
}

void free_min_heap(struct min_heap *heap)
{
    for (int i = 0; i < heap->size; i++)
        free_heap_node(heap->v[i]);
    free(heap->v);
    free(heap->pos);
    free(heap);
}

void free_digraph(struct digraph *digraph)
{
    struct list_node *list_node, *list_node_aux;
    for (int i = 0; i < digraph->num_nodes; i++)
    {
        list_node = digraph->adj[i]->head;
        while (list_node)
        {
            list_node_aux = list_node;
            list_node = list_node->next;
            free(list_node_aux);
        }
        free(digraph->adj[i]);
    }
    free(digraph->adj);
    free(digraph);
}

unsigned int
rand()
{
    const unsigned int a = 1103515245;
    const unsigned int c = 12345;
    const unsigned int m = 0x80000000;

    seed = (a * seed + c) % m;

    return seed;
}

void srand(unsigned int new_seed)
{
    seed = new_seed;
}

struct digraph *generate_random_digraph(int num_nodes, int num_edges)
{
    if (num_edges > num_nodes * (num_nodes - 1))
    {
        printf("A graph cannot have |E| > |V| * (|V| - 1)");
        exit(2);
    }

    struct digraph *digraph;
    // int is_edge[10][10];
    int **is_edge = malloc(num_nodes * sizeof(int *));
    malloc_successfull_or_panic(is_edge);
    for (int i = 0; i < num_nodes; i++)
    {
        is_edge[i] = malloc(num_nodes * sizeof(int));
        malloc_successfull_or_panic(is_edge[i]);
        memset(is_edge[i], 0, sizeof(is_edge[i]));
    }

    digraph = malloc(sizeof(struct digraph));
    malloc_successfull_or_panic(digraph);

    digraph->num_nodes = num_nodes;

    digraph->adj = malloc(sizeof(struct list *) * num_nodes);
    malloc_successfull_or_panic(digraph->adj);

    for (int i = 0; i < num_nodes; i++)
        digraph->adj[i] = new_list();

    for (int i = 0; i < num_edges; i++)
    {
        int rand_u, rand_v;
        struct list_node *new_node;

    random_edge:
        rand_u = rand() % num_nodes;
        rand_v = rand() % num_nodes;

        while (rand_v == rand_u)
            rand_v = rand() % num_nodes;

        while (is_edge[rand_u][rand_v] != 0)
            goto random_edge;

        is_edge[rand_u][rand_v] = 1;
        new_node = new_list_node(rand_v, rand() % MAX_EDGE_WEIGHT);
        new_node->next = digraph->adj[rand_u]->head;
        digraph->adj[rand_u]->head = new_node;
        digraph->adj[rand_u]->size++;
    }

    for (int i = 0; i < num_nodes; i++)
    {
        free(is_edge[i]);
    }
    free(is_edge);

    return digraph;
}

int dijkstra(struct digraph *g, int u, int v)
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
            return heap_node->val;

        for (list_node = g->adj[heap_node->idx]->head; list_node; list_node = list_node->next)
        {
            int pos, relaxed_val;
            
            pos = min_heap->pos[list_node->idx];
            relaxed_val = list_node->val + heap_node->val;
            if (pos >= 0 && min_heap->v[pos]->val > relaxed_val)
                update_key_value(min_heap, pos, relaxed_val);
        }
    }
    return INFINITY;
}

void test_min_heap()
{
    // Cria um heap mínimo com capacidade 10
    struct min_heap *min_heap = new_min_heap(10);

    // Teste inicial de inserção
    printf("Teste inicial de inserção\n");
    for (int i = 0; i < 10; i++)
    {
        add_heap_node(min_heap, i, i); // Insere pares (chave, valor)
        if (min_heap->size != i + 1)
        {
            printf("Erro: Tamanho do heap está incorreto após inserção de %d.\n", i);
            exit(1);
        }
        printf("idx: %d | val: %d\n", min_heap->pos[i], min_heap->v[0]->val); // Exibe a posição e o valor mínimo atual
    }
    printf("\n");

    // Verifica se o heap está ordenado corretamente (propriedade de heap mínimo)
    for (int i = 1; i < min_heap->size; i++)
    {
        if (min_heap->v[i]->val < min_heap->v[(i - 1) / 2]->val)
        {
            printf("Erro: Propriedade de heap mínima violada na posição %d.\n", i);
            exit(1);
        }
    }

    // Teste de update_key_value para diminuir o valor da chave 9 para -1
    printf("Atualizando valor da chave 9 para -1\n");
    update_key_value(min_heap, 9, -1);
    if (min_heap->v[0]->val != -1)
    {
        printf("Erro: A atualização de 9 para -1 falhou. Topo do heap é %d.\n", min_heap->v[0]->val);
        exit(1);
    }
    printf("idx: %d | top: %d\n", min_heap->pos[9], min_heap->v[0]->val);

    // Teste de update_key_value para diminuir o valor da chave 5 para -10
    printf("Atualizando valor da chave 5 para -10\n");
    update_key_value(min_heap, 5, -10);
    if (min_heap->v[0]->val != -10)
    {
        printf("Erro: A atualização de 5 para -10 falhou. Topo do heap é %d.\n", min_heap->v[0]->val);
        exit(1);
    }
    printf("idx: %d | top: %d\n", min_heap->pos[5], min_heap->v[0]->val);

    // Verifica novamente a propriedade de heap mínimo
    printf("Verificando propriedade do heap após updates...\n");
    for (int i = 1; i < min_heap->size; i++)
    {
        if (min_heap->v[i]->val < min_heap->v[(i - 1) / 2]->val)
        {
            printf("Erro: Propriedade de heap mínima violada após update na posição %d.\n", i);
            exit(1);
        }
    }

    printf("Todos os testes passaram com sucesso!\n");

    // Teste da função pop_heap
    printf("\nTestando pop_heap\n");

    int expected_order[] = {-10, -1, 0, 1, 2, 3, 4, 6, 7, 8};
    for (int i = 0; i < 10; i++)
    {
        struct heap_node *node = pop_heap(min_heap);
        if (node->val != expected_order[i])
        {
            printf("Erro: Valor esperado ao remover era %d, mas foi %d\n", expected_order[i], node->val);
            exit(1);
        }
        printf("Removido do heap: idx: %d | val: %d\n", node->idx, node->val);
        free_heap_node(node);
    }

    if (min_heap->size != 0)
    {
        printf("Erro: Heap deveria estar vazio após todas as remoções.\n");
        exit(1);
    }

    printf("Todos os elementos removidos corretamente e o heap está vazio.\n");

    free_min_heap(min_heap);
}

struct digraph *create_known_graph()
{
    int num_nodes = 5;
    struct digraph *g = malloc(sizeof(struct digraph));
    malloc_successfull_or_panic(g);

    g->num_nodes = num_nodes;
    g->adj = malloc(num_nodes * sizeof(struct list *));
    malloc_successfull_or_panic(g->adj);

    // Inicializa a lista de adjacências
    for (int i = 0; i < num_nodes; i++)
    {
        g->adj[i] = new_list();
    }

    // Adiciona arestas com pesos conhecidos
    // Exemplo:
    // 0 -> 1 (peso 10)
    // 0 -> 4 (peso 5)
    // 1 -> 2 (peso 1)
    // 1 -> 4 (peso 2)
    // 2 -> 3 (peso 4)
    // 3 -> 0 (peso 7)
    // 3 -> 2 (peso 6)
    // 4 -> 1 (peso 3)
    // 4 -> 2 (peso 9)
    // 4 -> 3 (peso 2)

    // Adiciona as arestas na lista de adjacências
    g->adj[0]->head = new_list_node(1, 10);
    g->adj[0]->head->next = new_list_node(4, 5);

    g->adj[1]->head = new_list_node(2, 1);
    g->adj[1]->head->next = new_list_node(4, 2);

    g->adj[2]->head = new_list_node(3, 4);

    g->adj[3]->head = new_list_node(0, 7);
    g->adj[3]->head->next = new_list_node(2, 6);

    g->adj[4]->head = new_list_node(1, 3);
    g->adj[4]->head->next = new_list_node(3, 2);
    g->adj[4]->head->next->next = new_list_node(2, 9);

    return g;
}

int main(int argc, char *argv[])
{

    // struct digraph *g = create_known_graph();
    // int start_node = 0;
    // int end_node = 3;

    // int distance = dijkstra(g, start_node, end_node);
    // printf("Shortest path from %d to %d is: %d\n", start_node, end_node, distance);

    // free_digraph(g);
    // return 0;

    // test_min_heap();

    srand(90);
    int max_nodes = 200;
    struct digraph *g = generate_random_digraph(max_nodes, 200);
    struct list_node *list_node;
    for (int i = 0; i < g->num_nodes; i++)
        for (list_node = g->adj[i]->head; list_node; list_node = list_node->next)
            printf("%d -> %d (%d)\n", i, list_node->idx, list_node->val);
    int u, v;
    u = rand() % max_nodes;
    v = rand() % max_nodes;

    while (v == u)
        v = rand() % max_nodes;

    printf("%d ~ %d | %d\n", u, v, dijkstra(g, u, v));

    free_digraph(g);
    exit(0);
}