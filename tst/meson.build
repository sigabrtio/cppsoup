test_utils_headers = include_directories('thesoup/test_utils')

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

kruskal_test = executable('kruskal_test', 'thesoup/algorithms/kruskal.cc',
                    include_directories : [inc, test_utils_headers], dependencies: [catch])

utils_test = executable('utils_test', 'thesoup/algorithms/utils.cc',
                    include_directories : [inc, test_utils_headers], dependencies: [catch])

round_robin_threadpool_test = executable('simple_thread_pool_test', 'thesoup/async/round_robin_threadpool.cc',
                    include_directories : [inc, test_utils_headers], dependencies: [catch])

test('Types test', types_test)
test('Vector test', vector_cache_test)
test('Indexed graph test', indexed_graph_test)
test('Simple graph test', simple_graph_test)
test('Graph traversals test', graph_traversals_test)
test('Disjoint sets test', disjoint_sets_test)
test('Kruskal test', kruskal_test)
test('Utils test', utils_test)
test('Round robin thread-pool test', round_robin_threadpool_test)

