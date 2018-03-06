\name{get_raw_block}
\alias{get_raw_block}

\title{
Generic (low level) GET call to a block in the block API
}

\description{
Writes data from a block persisted in the server.
}

\usage{
get_raw_block(source, block_key, host = .host., buffsize = 1048576)
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
  \item{buffsize}{
(Default = 1 Mb) the size of the buffer to download binary objects. Must be bigger or equal to the size of the block downloaded.
}
}

\value{
An R raw object or throws an exception on failure.
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
