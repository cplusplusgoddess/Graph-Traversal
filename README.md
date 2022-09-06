# Graph Traversal
**PROBLEM:**
    Given an edge list compute the cheapest simultaneously 
	branching path, visiting all nodes, of a connected 
	graph starting from the top node. The graph contains 
	at least one node and #nodes <= 100


 **ALGORITHM:**
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

