\name{get_R_block}
\alias{get_R_block}

\title{
Read a data block as an R object
}

\description{
Reads a data block as an R object. The server automatically converts the appropriate data
type into logical, integer, numeric or character.
}

\usage{
get_R_block(source, block_key, host = .host., buffsize = 1048576)
}

\arguments{
  \item{source}{
The Jazz source. Jazz persistence is organized in sources. All sources except 'sys' and 'www'
are user defined. Sources are 15 char long alphanumeric or underscore.
}
  \item{block_key}{
The key identifying the block. Keys are 15 alphanumeric or underscore characters. They can
be user defined or created by new_key(). Also, meshes use block keys internally.
}
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via
set_jazz_host().
}
  \item{buffsize}{
(Default = 1 Mb) the size of the buffer to download binary objects. Must be bigger or equal
to the size of the block downloaded.
}
}

\value{
Returns an R object of type logical, integer, numeric or character or raises an error on failure.
}

\examples{
\dontrun{
create_source('demo_put')

create_block_rep('demo_put', 'bool_1', TRUE, 3)
any(rep(TRUE, 3) != get_R_block('demo_put', 'bool_1'))

create_block_rep('demo_put', 'int_1', 2L, 4)
any(rep(2L, 4) != get_R_block('demo_put', 'int_1'))

create_block_rep('demo_put', 'real_1', 3.14, 5)
any(rep(3.14, 5) != get_R_block('demo_put', 'real_1'))

create_block_rep('demo_put', 'str_1', 'Hi!', 6)
any(rep('Hi!', 6) != get_R_block('demo_put', 'str_1'))

create_block_seq('demo_put', 'int_2', 456L, 999L, 123L)
any(seq(456L, 999L, 123L) != get_R_block('demo_put', 'int_2'))

create_block_seq('demo_put', 'real_2', 0.123, 4.56, 0.789)
any(seq(0.123, 4.56, 0.789) != get_R_block('demo_put', 'real_2'))

delete_source('demo_put')
}
}
