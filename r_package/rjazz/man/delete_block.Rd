\name{delete_block}
\alias{delete_block}

\title{
Delete a block on the server
}

\description{
Deletes a block on the server. You must own the block to be able to delete it. It is the lowest level function and it can create harm on the server to delete blocks belonging to meshes or API functions.
}

\usage{
delete_block(source, block_key, host = .host., silent = FALSE)
}

\arguments{
  \item{source}{
The Jazz source. Jazz persistence is organized in sources. All sources except 'sys' and 'www' are user defined. Sources are 15 char long alphanumeric or underscore.
}
  \item{block_key}{
The key identifying the block. Keys are 15 alphanumeric or underscore characters. They can be user defined or created by new_key(). Also, meshes use block keys internally.
}
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
  \item{silent}{
(Optional) If this is TRUE, the function returns FALSE instead of throwing an error in case the corresponding PUT or DELETE function returns false.
}
}

\value{
Returns TRUE if successful. When silent == FALSE (default) throws an error on any failure. Otherwise, it returns FALSE when the corresponding PUT or DELETE function returns false.
}

\examples{
\dontrun{
create_source('demo_bin')

# When a string is written into a raw block, charToRaw() is applied automatically.
put_raw_block('demo_bin', 'blk_1', 'Hello world!')

a <- get_raw_block('demo_bin', 'blk_1')
# a is raw
rawToChar(a)

# This is the same.
put_raw_block('demo_bin', 'blk_2', charToRaw('Hello again!'))
rawToChar(get_raw_block('demo_bin', 'blk_2'))

# Anything else can be written by serializing as raw.
put_raw_block('demo_bin', 'blk_3', serialize(iris, NULL))

head(unserialize(get_raw_block('demo_bin', 'blk_3')))

# Delete the block or fail
delete_block('demo_bin', 'blk_1')

# Delete will fail, but make it silent
delete_block('demo_bin', 'blk_1', silent = TRUE)

# No need to delete all blocks, they will be deleted by deleting the source.
delete_source('demo_bin')
}
}
