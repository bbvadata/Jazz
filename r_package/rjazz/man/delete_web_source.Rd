\name{delete_web_source}
\alias{delete_web_source}

\title{
Remove a complete web source for the server's "www" source
}

\description{
Removes a complete web source for the server's "www" source. This also deletes all the resources allocated with the web source.
}
\usage{
delete_web_source(web_source, host = .host., silent = FALSE)
}

\arguments{
  \item{web_source}{
The name of the web source to be deleted. All www resource links and urls are grouped under a "web source" which is just a name to allow removing them with a single call.
}
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
  \item{silent}{
(Optional) If this is TRUE, the function returns FALSE instead of throwing an error in case the corresponding PUT or DELETE function returns false.
}
}

\details{
This is a macro function wrapping calls around put_function().
}

\value{
Returns TRUE if successful. When silent == FALSE (default) throws an error on any failure. Otherwise, it returns FALSE when the corresponding PUT or DELETE function returns false.
}

\examples{
\dontrun{
set_jazz_host('127.0.0.1:8888')
page <- '<html>\n<body>\n<br/>Hello world!\n</body>\n</html>'
create_web_resource('my_test',
                    '/my_test/hello.html',
                    type_const[['BLOCKTYPE_RAW_MIME_HTML']],
                    page)
# See http://127.0.0.1:8888/my_test/hello.html with a browser.

list_web_sources()
delete_web_source('my_test')
}
}
