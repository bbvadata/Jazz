## Parsing Bob

In Bop something can be:

  - a tensor (The data itself)
  - a url (more general than a locator, the BaseAPI parses a url into a stdAPIQueryState structure)
  - a locator (//base/entity/key)
  - a Space (A node in a hierarchy of code, down to opcodes)
  - operators
  - an expression using all of the above
  - a name (a reference for all of the above)

### Constants

  a = 'something'		// This is a url, not a string. Parsed by BaseAPI
  a = "//something"		// This is locator. Parsed by parse_locator(...) the " prevents // becoming a comment
  a = ["hello"]			// This is block with one string. It is syntactic sugar for '["hello"]' Parsed and provided by BaseAPI

### ()

A pair of brackets encloses an expression, possibly nested. The outermost expression becomes a (possibly named) tuple.
A tuple is typically used in a function call.

### []

A pair of square brackets filters, as in a[2], but whatever is inside is just an expression. Space descendants like DataSpace makes it
SQL-like, Concept makes it natural language friendly.

### .

The dot operator accesses members of a space. At then minimum, a Space has a key/value store (the children spaces). Concepts have
grounding (like breaking polysemy with numbers, POS tags, semspace), functions have parameters and a body, etc.
