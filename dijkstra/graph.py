class Graph:
    def __init__(self):
        self.edges = dict()
        self.indexes = dict()
        self.nodes = dict()
        #self.name = "graph_name"

    def add_edge(self, from_node, to_node):
        self.edges.setdefault(from_node, [])
        self.edges.setdefault(to_node, [])
        self.edges[from_node].append(to_node)
        self.edges[to_node].append(from_node)

    def has_edge(self, from_node, to_node):
        return to_node in self.edges.get(from_node, [])

    def num_edges(self):
        return sum([len(x) for x in self.edges.values()])
    """
    getNeighbors() takes a node N from a graph
    and returns a list of all neighbors of N.
    """
    def get_neighbors(self, node):
       return self.edges.get(node, [])

    def __repr__(self):
        return "\n".join([str(s) for s in self.edges.items()])
    
    def __len__(self):
        return len(self.edges)
