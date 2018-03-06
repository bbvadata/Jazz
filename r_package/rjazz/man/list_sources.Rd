\name{list_sources}
\alias{list_sources}

\title{
List all the sources on the Jazz server
}

\description{
Lists all the Jazz server sources including the 'sys' and 'www' sources. Jazz persistence is organized in sources. All sources except 'sys' and 'www' are user defined. Sources are 15 char long alphanumeric or underscore.
}

\usage{
list_sources(host = .host.)
}

\arguments{
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
}

\value{
Returns the server's sources as a vector of string.
}

\examples{
\dontrun{
list_sources()
}
}
