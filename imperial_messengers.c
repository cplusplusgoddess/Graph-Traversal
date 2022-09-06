#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
/* -------------------------------------------------------

  FILE: imperial_messengers.c
  AUTHOR: Amber Rogowicz
  DATE: Sept 04, 2022

  PROBLEM:
    Given an edge list compute the cheapest simultaneously 
	branching path, visiting all nodes, of a fully connected 
	graph starting from the top node. The graph contains 
	at least one node and #nodes <= 100
    ALGORITHM:
    Starting from the Capitol (node 0) traverse the graph 
	using breadth first search using a greedy priority 
	of nodes to visit, overriding each node's min_to_capitol 
	if a faster path exists, along the way. Finally, choose 
	the MAX of all node's min_to_capitol to return the farthest city 
	from the capitol, of all paths.
    Implementation is a combo of a Dijkstra algo and Breadth 
	First with a Priority Queue Adjacency List processed at
    Graph build/create time. Ignoring build_graph() work:
	dijkstra_graph() -
    Work Complexity 0(|V|*|V-1|) worst case approx 0(V^2)
    Space Complexity 0(V + E=(V*(V-1)) + visited[MAX_NODES] + 
										visit queue[V])
      *
     |*|
 l--(*0*)--10--(4)--10--(3)
 | |_____|    ^^^^^^^^^^/ \
 | ~~~~~~\--30-(2)-^50-/  |
 | ~~~~~ |       \5 ^ /   | 
 | ~~~~~~'--50--(1)--20   |
 | ~~~~~~~~  ~~~~~~~~~~~  |
 | ~~~~~lake Big ~~~~~~~~ | 
100-----------------------l


Unable to acquire version of Visual C++ Studio for OS-X 10.13 High Sierra. 
- program compiles for GCC version 6.5.0

Time of effort: TOO LONG! at least 16 hours including 
	* Graph algo research (violating the restriction for this solution). 
	* C I/O review. 
	* Installation effort of Visual Studio on Mac OSX
	* Dot i's and cross t's (memory management, documentation, testing) 

Other Algorithm Considerations: There is promise in
A Bidirectional Heuristic Search, parallelizing computations
from each end, using std::thread::hardware_concurrency()
*/

 
/* Define the maximum number of vertices in the graph */
#define MAX_NODES 100
#define MIN(x, y)((x < y)?x:y)
#define MAX(x, y)((x > y)?x:y)

#define CAPITOL 0
#define VISITED(list,id)( (!CAPITOL && list[id])?1:0)

#define CLEAR_INT(buf, size)(memset(buf, 0, sizeof(int)*size))
 
/* Data structure to store one direction of an undirected edge */
typedef struct _edge {
	int dest, weight;
}Edge, *EdgePtr;

/* edge list element held in a queue */
typedef struct _edge_ele {
	EdgePtr           edge;
	struct _edge_ele *next ;
}EdgeEle, *EdgeElePtr;

/* Edge Queue */
typedef struct _edgelist {
	EdgeElePtr front ;
	EdgeElePtr back ;
} EdgeQueue, *EdgeQueuePtr;


/* Node or Vertex structure */
typedef struct node {
    int id, min_to_capitol;
    EdgeQueue neighbors;
} Node, *NodePtr;



EdgeElePtr ele_alloc(int dest, int weight) {
  EdgeElePtr newedge_ele = (EdgeElePtr)malloc(sizeof(EdgeEle));
  EdgePtr edge          = (EdgePtr)malloc(sizeof(Edge));
  edge->dest = dest;
  edge->weight = weight;
  newedge_ele->edge = edge;
  newedge_ele->next = NULL;
  return (newedge_ele);
}

#define Q_EMPTY(q)((q->front==NULL)?1:0)

/* pop is the same for a priority and plain queue */
EdgePtr q_pop(EdgeQueuePtr q) {
	if (Q_EMPTY(q))
		return NULL;

    EdgeElePtr ele =  q->front;   /* pop front off the queue */
    EdgePtr ret_edge =  ele->edge; /* return the alloc edge now responsibility of
									caller */
    q->front = q->front->next;
	free(ele);
	/* case one item on the queue i.e. front == back */
    if(q->front == NULL) 
		q->back  = NULL;
	return ret_edge;

}

/* memory release */
EdgeElePtr q_erase(EdgeQueuePtr q) {
    EdgePtr edge ;
	while (!Q_EMPTY(q)) {
		edge = q_pop(q);
		free(edge);
	}
	/* caller allocates the queue */
	return NULL;
}
	
/* Just a normal Queue insert (for visit queue) */
void q_insert(EdgeQueuePtr  q, EdgePtr e) {
	EdgeElePtr ele = ele_alloc(e->dest, e->weight);
	if (Q_EMPTY(q)){
		q->front = ele;
		q->back  = ele;
	} else {
		q->back->next = ele;
		q->back = q->back->next;
	}
}


/* for node adjacency lists, orders by increasing edge weight */
void priority_push(EdgeQueuePtr  q, EdgePtr e) {
	/* make a copy */
	EdgeElePtr ele = ele_alloc(e->dest, e->weight);
    EdgeElePtr curr_e = q->front; 
    EdgeElePtr prev_e = NULL; 
	if (Q_EMPTY(q)){
		q->front = q->back = ele;
		return;
	} 
	/* traverse the queue for an insert point based on weight */
	/* keep the light weights close to the front */
	curr_e = q->front;
	while((curr_e != NULL) && (curr_e->edge->weight <= ele->edge->weight )){
		prev_e = curr_e;
		curr_e = curr_e->next;
	}
    if( prev_e == NULL ){ /* put the new edge in the front */
		ele->next = q->front;
		q->front = ele;
		return;
	}
	ele->next = prev_e->next;
	prev_e->next = ele;
}

/* Data structure to store a graph object */
typedef struct _graph {
    /* An array of nodes in a graph allocated block */
    NodePtr nodes; 
	int     num_nodes;
} Graph;


/* 	Implementation of a Dijkstra algo and Breadth First with a Priority Queue
   	Adjacency List  (priority queue processing already spent in build_graph())
	A greedy queue entry for all nodes in a breadth first 
	traversal of the graph. Updating min_to_capitol value 
	in adjacency lists for each node popped from visit queue.
	Finally, all nodes in the graph are examined for the 
	MAX of min_to_capitol for all nodes in the graph.
	dijkstra_graph() -
    Work Complexity 0(|V|*|V-1|) worst case approx 0(V^2)
    Space Complexity 0(V + E=(V*(V-1)) + visited[MAX_NODES] + 
										visit queue[V])

*/
void dijkstra_graph( Graph *graph ){

  int visited[MAX_NODES] ;
  CLEAR_INT(&visited, MAX_NODES);

  EdgeQueue visit_queue ;
  EdgeQueuePtr visit_q = &(visit_queue);
  visit_q->front = visit_q->back = NULL;

  Edge edge  = {CAPITOL, 0};

  q_insert(visit_q, &edge);

  int pushed_visit ;
  EdgePtr e;
  EdgeElePtr ele;
  NodePtr u;
  NodePtr v = &(graph->nodes[CAPITOL]);
  visited[v->id] = 1;
  v->min_to_capitol = 0;

  /* while there are still paths to explore */
  while (!Q_EMPTY(visit_q) ){
    e = q_pop(visit_q);
    v = &(graph->nodes[e->dest]);
#ifdef DEBUG
  	printf("Exploring paths from node %d ->min = %d\n", v->id, v->min_to_capitol);
#endif
    /* Loop thru node adjacency list */
	
    free(e);  /* my responsibility popped off queue */
	pushed_visit = 0;
    ele = v->neighbors.front;
	while(ele != NULL ){
		/* push the closest neighbor that hasnt been visited yet */
		if(!pushed_visit && !visited[ele->edge->dest]){
			q_insert(visit_q, ele->edge) ;
			visited[ele->edge->dest] = 1;
#ifdef DEBUG
  			printf("Pushing node %d \n", ele->edge->dest);
#endif
			pushed_visit = 1;
		}
    	u = &(graph->nodes[ele->edge->dest]);
		u->min_to_capitol = MIN(u->min_to_capitol ,
								v->min_to_capitol + ele->edge->weight);
		ele = ele->next;
	}
  }
  /* this should be unnecessary, we've popped everything 
  q_erase(visit_q); */
  return ;
} 
 

#define MAXLINELEN 200

/* Function to add an edge to a node's adjacency list 
   Im called twice for to and from directions of the edge */
void add_edge(EdgeQueuePtr q, int src, int dest, int weight){
	assert(src != dest);
    Edge newedge = { dest, weight };
    newedge.dest = dest;
    newedge.weight = weight;
	
    priority_push( q, &newedge) ;
}


/* Function to print adjacency list representation of a graph */
void print_graph(Graph *graph)
{
    for (int i = 0; i < graph->num_nodes; i++)
    {
        // print current vertex and all its neighbors
		NodePtr v  = &(graph->nodes[i]);
        EdgeElePtr ele = graph->nodes[i].neighbors.front;
        printf("min dist: %d ", v->min_to_capitol );
        while (ele != NULL)
        {
            printf("(%d -—%d--> %d)  ", i, ele->edge->weight, ele->edge->dest);
            ele = ele->next;
        }
 
        printf("\n");
    }
}

Graph * destroy_graph(Graph *graph) {
	NodePtr nodeptr;
    for (int i = 0; i < graph->num_nodes; i++) {
		nodeptr = &(graph->nodes[i]);
		q_erase(&(nodeptr->neighbors));
    }
	free(graph->nodes);
	return((Graph*)NULL);
}
/* Reads input from standard in and constructs a graph with the 
   worst space complexity of V = num nodes E = num edges = V*(V-1) 
   0(V + E)
*/
Graph * build_graph(void) {
	int     num_nodes;
	char    buf[300];
	int     e_num = 0;
	char*   e_strptr;
	NodePtr nodeptr;
	char*   delim = " \n";
	int     weight;
    /* get number of nodes */
	/* dummy = getline(&buf, MAXLINELEN); */
	fgets ( buf, MAXLINELEN, stdin );
	sscanf(buf, "%d", &num_nodes);
    /* allocate storage for the graph data structure */
    Graph* graph= (Graph*)malloc(sizeof(Graph));
    graph->nodes = (NodePtr)malloc(sizeof(Node)*num_nodes);
    graph->num_nodes = num_nodes;
    /* initialize edge list pointer for all vertices */
    for (int i = 0; i < num_nodes; i++) {
		graph->nodes[i].id = i;
		graph->nodes[i].min_to_capitol  = INT_MAX;
		graph->nodes[i].neighbors.front = NULL;
		graph->nodes[i].neighbors.back  = NULL;
    }
 
    for(int i = 1; i < num_nodes; i++) {
		fgets ( buf, MAXLINELEN, stdin );
		if (feof(stdin))  
			break;
		e_num = 0;
		nodeptr = &(graph->nodes[i]);
		e_strptr = strtok(buf, delim);

		/* consume a line of edges */
		while(e_strptr != NULL) {
			if('x' == tolower(e_strptr[0]) ) {
				e_num++;
				e_strptr = strtok(NULL, delim);
				continue;
			}
			weight = atoi(e_strptr);
			add_edge(&(nodeptr->neighbors), i, e_num, weight);
			/* if there's a direct path to capitol, set as min dist */
			// nodeptr->min_to_capitol = (e_num == 0)?weight:INT_MAX;
			add_edge(&(graph->nodes[e_num].neighbors), e_num, i, weight);
			e_num++;
			e_strptr = strtok(NULL, delim);
		}
				
    } /* end for each node */
 
    return (graph);
}

/* loops thru Number of nodes in the graph to seek the minimum
   distance to farthest node away from capitol. 
   does 0(V) work complexity
*/

int fastest_throughout_kingdom(Graph *graph){

	int farthest_city=0;

	if( graph->num_nodes == 0 )
		return(farthest_city);

	dijkstra_graph(graph);
#ifdef DEBUG
	print_graph(graph);
#endif
	for (int i = 1; i < graph->num_nodes; i++){
		
		farthest_city = MAX(farthest_city, 
						graph->nodes[i].min_to_capitol);
	}
	return (farthest_city);
}
 
 
/* Undirected graph implementation in C */
int main(void)
{
	int i, n = 0;

	
   	Graph *graph = build_graph();
 
#ifdef DEBUG
    print_graph(graph);
#endif
 
    printf("%d \n", fastest_throughout_kingdom(graph));
	graph = destroy_graph(graph);
    return 0;
}
