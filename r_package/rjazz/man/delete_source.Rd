\name{delete_source}
\alias{delete_source}

\title{
Delete a source on the server
}

\description{
Deletes a source on the server even if it is not empty. The sources 'sys' and 'www' cannot be deleted.
}

\usage{
delete_source(source, host = .host., silent = FALSE)
}

\arguments{
  \item{source}{
The Jazz source. Jazz persistence is organized in sources. All sources except 'sys' and 'www' are user defined. Sources are 15 char long alphanumeric or underscore.
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
