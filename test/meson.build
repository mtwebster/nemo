
test_nemo_search_engine = executable('test-nemo-search-engine',
  [ 'test-nemo-search-engine.c' ],
  include_directories: [ rootInclude, ],
  dependencies: [ gtk, nemo_private ],
)
test('Search Engine test',
  test_nemo_search_engine,
  args: []
)

test_nemo_directory_async = executable('test-nemo-directory-async',
  [ 'test-nemo-directory-async.c' ],
  include_directories: [ rootInclude, ],
  dependencies: [ gtk, nemo_private ],
)
test('Directory Async test',
  test_nemo_directory_async,
  args: []
)

test_nemo_copy = executable('test-nemo-copy',
  [ 'test-copy.c', 'test.c' ],
  include_directories: [ rootInclude, ],
  dependencies: [ gtk, nemo_private ],
  c_args: nemo_definitions,
)
# test('Copy test',
#     test_nemo_copy,
#     args: [])
