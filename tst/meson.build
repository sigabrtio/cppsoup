types_test = executable('types_test', 'thesoup/types/types.cc',
                    include_directories : inc, dependencies: [catch])

vector_cache_test = executable('persistent_vector_test', 'thesoup/types/vector.cc',
                    include_directories : inc, dependencies: [catch])

graph_test = executable('graph_test', 'thesoup/types/graph.cc',
                    include_directories : inc, dependencies: [catch])

test('Types test', types_test)
test('Vector test', vector_cache_test)
test('Graph test', graph_test)
