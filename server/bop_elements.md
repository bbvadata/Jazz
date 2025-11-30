# Bebop by the pieces it is made of and how are written as source code

## What the elements are:

### Locator

Is anything accepted by BaseAPI.

  - If it is acceptable as BaseAPI.get(), it is a get locator.
  - If it is acceptable as BaseAPI.put(), it is a put locator. (The same locator can be both get and put.)


### Locators

A object that works as an iterator returning Locator strings. (Possibly: none, one or many.)


### Kind

We use the word "type" for the actual tensor type (int, float, string, etc.). A "kind" is a higher-level type like "image" which is some
tuple with say, r, g, b channels + metadata. The tuple has a shape for each of its tensors. The kind can use placeholders like "width"
and "height". So every complying tuple has a is_a() relationship with the kind.

A Kind is a Block.

  - Everything has a Kind in Bop. Simple Blocks are tensors of in32 or whatever, Tuples have a Kind is_a() method to see if they comply.
  - There is only one space with all the Kinds registered in it.
  - Mosts Kinds are built-in, type conversion of tensors, things that require code (jpeg, wav, ...)


### A Block, a Kind, a Tuple or a Tuple descendant

It is just a Block or a Block descendant (Tuple, Kind, Snippet, BopFile, ObjFile, ...)

This is a Tuple (a named tuple of tensors) or a single Block.

  - It is just a movable block of data, not a service.
  - It has attributes (from Block) to set locators (getters and setters) and kinds.
  - It is managed by some Container.
  - It supports the following operations:
	* getting (and setting its get locator)
	* putting (and setting its put locator)
	* deleting (with or without deleting its source/destination)
	* casting itself or to new Object by kind using a Caster.
	* selecting (when it is a tuple)


### Space

A Space is no longer a Service.

It is an object that:

  - has metadata storage via a locator to a persistent of volatile storage.
  - supports the [] operator
  - supports the '.' operator
  - supports a hierarchy
  - Has only one Kind (its own Kind, Snippet, DataSpace, Concept, SemSpace, ...)


### Objects

Anything that goes from Block, Kind, Tuple, Tuple descendants to Space, is an Object. But Object does not exist in singular.
Whatever produces an object could possibly produce many or none. So an Object with one element is just a particular case of
Objects.


### Void

A Special object that represents a get from a Locators of length zero (like a query that returns no results).


### Namespace

A Namespace is a Space that works a a key/value store of names to expressions.


### Casters

A Caster is method that converts from one Kind to another. Casters is a Namespace in which the compiler looks for the appropriate method.
Source-wise only the destination kind is specified.


### Expressions

  - Expressions are there objects of a class. Just understood by the compiler.

### Operators

  - Operators are neither definable in the language, nor are there objects of a class. Just understood by the compiler.
  - Note that operators inside a locator work just like defined in BaseAPI, Bop treats them as constant strings passed to BaseAPI.

The operators are:

  - '..' : Declares a constant locator. Note that " is frequently used inside locators, so " and ' are not interchangeable.
  - =	 : Define
  - <-   : Get, possibly with casting becomes (a_kind)<-
  - ->   : Put, possibly with casting becomes ->(a_kind)
  - (..) : Depends on context, it can be a function call, a cast, or an operator precedence grouping.
  - [..] : Select. On a Space, the space itself manages get/put separately. On a Tuple, it selects Blocks. Selection of elements from
		   a Block is possible only via a locator or Space interface. (To select elements from a Block you need to create a new Block.)
  - +, -, *, /, ... : Are syntactic sugar for ONNX opcodes.

## How they are written as source code

a = b				// Defines a (b is an expression, a becomes an expression, not the result of evaluating it.)
a <- b				// Get a from b (b is a locator or an expression that returns Locators) A becomes Void or an Objects
a (a_kind)<- b		// Get a from b with explicit casting to a `a_kind`
a <-				// Get a (if a already exists and has a get locator, this is possible)
a -> b				// Put a to b
a ->(a_kind) b		// Put a to b with explicit casting to a `a_kind`
a ->				// Put a (if a
a[..] = b			// Not possible in Bebop-25. That would mean a declarative update of a space. Too complex for now.
a[..] <-			// Not possible in Bebop-25. That would be a get from all to a part.
a[..] <- b			// Correct. A partial get (assuming the space a has a put for the expression '..' and the casting is possible)
a[..] -> b
a[..] ->(a_kind) b
a[..] ->

TODO: function calls, introspection, function definition, space nesting