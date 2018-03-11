\name{put_raw_block}
\alias{put_raw_block}

\title{
Write a raw object or a string as a block.
}

\description{
Writes a raw object or a string as a block in persistence. That block can be stored as raw, converted to a compatible raw type or converted into data by the server.
}

\usage{
put_raw_block(source, block_key, block_val, host = .host.)
}

\arguments{
  \item{source}{
The Jazz source. Jazz persistence is organized in sources. All sources except 'sys' and 'www' are user defined. Sources are 15 char long alphanumeric or underscore.
}
  \item{block_key}{
The key identifying the block. Keys are 15 alphanumeric or underscore characters. They can be user defined or created by new_key(). Also, meshes use block keys internally.
}
  \item{block_val}{
The content of the block. This function is for data blocks and the type is automatic. The block must be an array of: boolean, integer, double or string. For blocks other than data blocks, such as web resources, use the appropriate function.
}
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
}

\value{
TRUE or raises an error on failure.
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
