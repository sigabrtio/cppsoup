types_test = executable('types_test', 'thesoup/types/types.cc',
                    include_directories : inc, dependencies: [catch])

vector_cache_test = executable('persistent_vector_test', 'thesoup/types/vector.cc',
                    include_directories : inc, dependencies: [catch])

indexed_graph_test = executable('indexed_graph_test', 'thesoup/types/indexedgraph.cc',
                    include_directories : inc, dependencies: [catch])

simple_graph_test = executable('simple_graph_test', 'thesoup/types/simplegraph.cc',
                    include_directories : inc, dependencies: [catch])

graph_traversals_test = executable('graph_traversals_test', 'thesoup/algorithms/graph_traversals.cc',
                    include_directories : inc, dependencies: [catch])

disjoint_sets_test = executable('disjoint_sets_test', 'thesoup/types/disjoint_sets.cc',
                    include_directories : inc, dependencies: [catch])

test('Types test', types_test)
test('Vector test', vector_cache_test)
test('Indexed graph test', indexed_graph_test)
test('Simple graph test', simple_graph_test)
test('Graph traversals test', graph_traversals_test)
test('Disjoint sets test', disjoint_sets_test)

