\name{create_block_seq}
\alias{create_block_seq}

\title{
Create a data block in the server with a simple linear sequence
}

\description{
Creates an (integer or real) data block in the server and stores it in the persistence with a simple linear sequence. The type of the block can later
be changed to a compatible type: BLOCKTYPE_C_INTEGER to BLOCKTYPE_C_FACTOR or BLOCKTYPE_C_GRADE and BLOCKTYPE_C_REAL to BLOCKTYPE_C_TIMESEC by a
later call to set_compatible_data_type().
}

\usage{
create_block_seq(source, block_key, from, to, by = 1, host = .host.)
}

\arguments{
  \item{source}{
The Jazz source. Jazz persistence is organized in sources. All sources except 'sys' and 'www' are user defined. Sources are 15 char long alphanumeric or underscore.
}
  \item{block_key}{
The key identifying the block. Keys are 15 alphanumeric or underscore characters. They can be user defined or created by new_key(). Also, meshes use block keys internally.
}
  \item{from}{
The starting value. A real number or an integer.
 }
  \item{to}{
The end value, may not be included, it is the supremum or the infimum when 'by' is negative.
}
  \item{by}{
The increment. It may be negative, in that case 'from' must be bigger than 'to'.
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
