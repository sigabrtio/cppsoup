types_test = executable('types_test', 'thesoup/types/types.cc',
                    include_directories : inc, dependencies: [catch])

persistent_vector_test = executable('persistent_vector_test', 'thesoup/types/vector.cc',
                    include_directories : inc, dependencies: [catch])

test('Types test', types_test)
test('Vector test', persistent_vector_test)
