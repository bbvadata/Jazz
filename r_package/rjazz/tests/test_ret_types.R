# BBVA - Jazz: A lightweight analytical web server for data-driven applications. (R client)
# ------------
#
#	Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.
#
# This product includes software developed at
#
# BBVA (https://www.bbva.com/)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
library(rjazz)

assert_type <- function(sexp, cl = 'logical')
{
	if (class(sexp) != cl) stop('Wrong class.')

	if (!is.null(rownames(sexp))) stop('Has rownames.')
	if (!is.null(colnames(sexp))) stop('Has colnames.')

	if (cl != 'list' && !is.null(names(sexp))) stop('Has names.')
}

assert_type(create_source('test_ret_types'))

assert_type(create_block_rep('test_ret_types', 'bool_1', TRUE, 3))
assert_type(get_R_block('test_ret_types', 'bool_1'))

assert_type(create_block_rep('test_ret_types', 'int_1', 2L, 4))
assert_type(get_R_block('test_ret_types', 'int_1'), 'integer')

assert_type(create_block_rep('test_ret_types', 'real_1', 3.14, 5))
assert_type(get_R_block('test_ret_types', 'real_1'), 'numeric')

assert_type(create_block_rep('test_ret_types', 'str_1', 'Hi!', 6))
assert_type(get_R_block('test_ret_types', 'str_1'), 'character')

assert_type(create_block_seq('test_ret_types', 'int_2', 456L, 999L, 123L))
assert_type(get_R_block('test_ret_types', 'int_2'), 'integer')

assert_type(create_block_seq('test_ret_types', 'real_2', 0.123, 4.56, 0.789))
assert_type(get_R_block('test_ret_types', 'real_2'), 'numeric')


page <- '<html>\n<body>\n<br/>Resource was not found on this node.\n</body>\n</html>'
assert_type(create_error_page(404, page))

assert_type(set_jazz_host(rjazz:::.host.))

page <- '<html>\n<body>\n<br/>Hello world!\n</body>\n</html>'
assert_type(create_web_resource('my_test', '/my_test/hello.html', type_const[['BLOCKTYPE_RAW_MIME_HTML']], page), 'character')

assert_type(list_web_sources(), 'character')
assert_type(delete_web_source('my_test'))

assert_type(put_raw_block('test_ret_types', 'blk_1', 'Hello world!'))

assert_type(get_raw_block('test_ret_types', 'blk_1'), 'raw')

assert_type(delete_block('test_ret_types', 'blk_1'))

assert_type(delete_block('test_ret_types', 'blk_1', silent = TRUE))

txt <- c('Hi all,', '', 'This is a file.', '', 'bye,', 'me')
str <- paste(txt, collapse = '\n')

assert_type(put_raw_block('test_ret_types', 'blk_1', str))

assert_type(set_compatible_data_type('test_ret_types', 'blk_1', type_const[['BLOCKTYPE_RAW_MIME_TXT']]))

assert_type(get_block_attributes('test_ret_types', 'blk_1'), 'list')

assert_type(put_block_flags('test_ret_types', 'blk_1', 123000444))

assert_type(get_block_attributes('test_ret_types', 'blk_1'), 'list')

assert_type(put_R_block('test_ret_types', 'blk_2', 3:6))

assert_type(create_block_seq('test_ret_types', 'blk_2', 3L, 6))

assert_type(get_block_attributes('test_ret_types', 'blk_2'), 'list')

assert_type(set_compatible_data_type('test_ret_types', 'blk_2', type_const[['BLOCKTYPE_C_R_GRADE']]))

assert_type(get_block_attributes('test_ret_types', 'blk_2'), 'list')

assert_type(get_block_as_string('test_ret_types', 'blk_2', '%d'), 'character')

rs <- c('1', '2.7', '3.14')

assert_type(put_strings_as_block('test_ret_types', 'blk_3', rs, type_const[['BLOCKTYPE_C_R_REAL']]))

assert_type(get_server_version(), 'character')
assert_type(get_server_version(full=T), 'list')

assert_type(list_sources(), 'character')

assert_type(new_key(), 'character')

assert_type(type_const[['BLOCKTYPE_RAW_MIME_HTML']], 'numeric')

assert_type(delete_source('test_ret_types'))
