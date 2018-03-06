\name{create_error_page}
\alias{create_error_page}

\title{
Customize the error pages on the server
}

\description{
Defines a new page for any http error status. These pages are always html (mime = text/html) but can use any number of resources (css, png, js, ...) which must be uploaded appropriately using link_web_resource().
}

\usage{
create_error_page(http_status, html, host = .host.)
}

\arguments{
  \item{http_status}{
The http status code returned as an error. Only those specified in Jazz API. E.g., 400 (Bad Request) - Syntactical error at top level. (Malformed URI)
}
  \item{html}{
The html page to be served for that error.
}
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
}

\value{
Returns TRUE on success or throws an error on failure.
}

\examples{
\dontrun{
page <- '<html>\n<body>\n<br/>Resource was not found on this node.\n</body>\n</html>'
create_error_page(404, page)
}
}
