\name{create_source}
\alias{create_source}

\title{
Create a new source on the server
}

\description{
Creates a new source on the server. You must be authorized to create sources. The server has a maximum number of sources that is configured through the C++ MAX_POSSIBLE_SOURCES and the configuration variable MDB_ENV_SET_MAXDBS including 'sys' and 'www'.
}

\usage{
create_source(source, host = .host.)
}

\arguments{
  \item{source}{
The Jazz source. Jazz persistence is organized in sources. All sources except 'sys' and 'www' are user defined. Sources are 15 char long alphanumeric or underscore.
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
