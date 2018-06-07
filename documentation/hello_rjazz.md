## rjazz, an R package to control Jazz, the lightweight data processing framework.

The talk introduces the package rjazz, (in cran).

Jazz (the server https://github.com/bbvadata/jazz-server) and rjazz (the client https://cran.r-project.org/web/packages/rjazz/index.html) were released 
as OSS by BBVA end of 2017. This corresponds to the current version actually available at cran.

During 2018, a community of volunteers started refactoring Jazz into the product showed in this talk. https://github.com/kaalam/Jazz 
It will be available before Nov '18 including:

## From 20,000 feet:

Before a detailed introduction, in case you feel the need to put a label on Jazz or want to pitch it in 10 seconds, you may ask…

### Is it a distributed no-sql database, a language, an http server, a deep learning framework?
Jazz is a new kind of data product. It combines all these things and some others (an automated creator of APIs, an abstraction over data sources, …) while being just one extremely lightweight process. It is free software created on top of third party free software. It is written in C/C++, available through R and Python packages and also through a REST API. The framework is Linux only, but, since it is an http back-end, Jazz consumers include all devices and operating systems. Jazz is designed for millions of users interacting concurrently with large clusters.

## What makes it special?

1. Blocks are volatile, persisted, distributed or ubiquitous as defined. Abstraction from extremely small to extremely large is
automatic. The same function works on a small vector or a distributed data-frame.
2. Extremely efficient persistence (a memory mapped file) in the same process allows sampling without copying. (No need of a DB.)
3. Extremely efficient REST API in the same process with a thread pool. (No need of an http server.)
4. A full language that is close to perfect performance (assuming enough tensor size), thread safe and auto distributing instead of
just some optimized data mangling primitives. (No need of an interpreter.)
5. Distributed data-frames where cells are tensors (E.g., video or sound tracks, not links to files)
6. An abstraction over the file system that allows reading/writing binary and compressed files on demand.
7. Tree Search as a first class citizen
8. Reinforcement learning “out of the box”
9. Inference on small samples done efficiently
10. Support for automatic programming at bytecode level
11. Deep Learning as a first class citizen

## What will the R API look like?

### APIs

And finally, the API. Thanks for reading this far! The API is a keepr named / (root in Unix) where all the keeprs and blocks are linked.

The API is the same for all the languages including REST (where root is // since just one / is used in normal urls.)

Any API call can be seen as an assignment. In an assignment, X = Y there are two sides, the left side (X) called lvalue and the right side (Y) called rvalue. This is C++ naming.

In Jazz an lvalue is one of:

  * A chain of keeprs abstracted as block ids starting from root ending with the name of a block. They must all exist except, possibly, the last name. If the last name is new, it is created, if it exists, overridden.
  * A data block that will be returned as a Python object, R object or http (GET) resource.

In Jazz an rvalue is one of:

  * A block constructor. A constant expression that can be used to build a block from it.
  * Chains of keeprs abstracted as existing blocks starting from root. This includes functions and blocks passed as arguments to functions.
  * A combination of the previous two.
  * A data block that will be passed as a Python object, R object or http (PUT) resource.
  * A delete predicate. This deletes the corresponding lvalue.

Since the API has to be REST compatible and is intended for using over a network.

  * All rvalue evaluations are safe. They cannot have side effects. Function calls cannot have side effects.
  * All lvalue assignments are idempotent. Assigning twice has the same effect than assigning once. There is no += operator.

Summarizing, there is one API not three (R, Python and REST), there are some differences that, for the moment, we will only mention at a very high level.

### R API
The R API has four methods:

  * jazz_get(rvalue)
  * jazz_put(object, lvalue)
  * jazz_delete(lvalue)
  * jazz_assign(lvalue, rvalue)

All methods return error codes, except jazz_get() which returns R vectors. R objects are limited to R core vectors, ergo not all the possible complexity of Jazz types is directly readable or writable to R. It is possible to create complex Jazz objects by “assembling” them from simple parts using Bebop functions written inside R. Since Jazz is accessible through files and the REST API, this is not a serious limitation. Other R packages built on top of rjazz can make working inside R friendlier.

The R package rjazz also includes built-in http client support to use the REST API from R.
