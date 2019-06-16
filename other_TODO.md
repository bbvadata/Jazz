## TODO tasks

The normal list of things to do, as set in the source code, can be retrieved with **./list_todo.sh**. This file includes
other tasks like bug fixes, ideas for changes or complete stories that don't even have a source file.

## Other TODO tasks

### Bugs

  - File issue in the python package build
  - Warnings in R package build_doc
  - (With old R server) And R == 3.6 on the server side:
     106.392668 : 03 :    47 : jzzBLOCKCONV::translate_block_FROM_R() : Wrong signature || format_version.
     106.392723 : 03 :    47 : jzzAPI::set_lmdb_fromR(): translate_block_FROM_R() failed.
	on the client side:
     Error in put_R_block("test_ret_types", "blk_2", 3:6) :
       Http PUT Not Acceptable.

### Missing things

  - Update the main Jazz project's README file
  - changes in the block names
  - update manual page with block selections
  - update manual page for lvalue
  - update manual page for rvalue

### Changes

  - Rename range as shape

### New ideas

#### API

  - changes in character definitions (as explained in API)
  - changes in API definition that should be updated in the source code
  - simplify the PUT mechanism by writing to a previously allocated hash-key. GET (alloc request with target size and target name) PUT (block)

#### Other

  - new function get_block_attributes()
  - new function set_block_attributes()
