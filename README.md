# About
This is yet another utility library: a collection of classes and functions that **may** be useful to some.

# Contents
## Utility classes
There are some small day-to-day life improvement utilities.
1. thesoup::types::Result - A class, similar to Rust's result. This allows us to return error information in a cleaner fashion, without throwing exceptions.
2. thesoup::types::Slice - A class representing a fat pointer. It can be considered a much simpler version of std::vector
3. thesoup::types::Unit - An enum to act like Scala's unit class.
4. thesoup::types::IsForwardIteratorOfType - A template, that can (as the name suggests) help us determine whether something is the forward iterator of a given type. This is very helpful in static asserts.
5. thesoup::types::IsTemplateSpecialization - A template that helps us determine if a type is a specialization of a template class. This is also something that is useful in static asserts.

## Graph classes
There are some graph classes provided
1. thesoup::types::Graph - This is a CRTP interface for implementing your own graph implementations. This is passed to many functions that work on generic graphs.
2. thesoup::types::IndexedPropertyDiGraph - This is a class that implements a property graph on top of the base graph interface (thesoup::types::Graph).
3. thesoup::types::SimpleWeightedGraph - This class implements a weighted graph on top the base graph interface (thesoup::types::Graph).

## Other classes
There are some other misc types also provided for convenience.
1. thesoup::types::VectorCache - A class similar to ctd::vector, but with the added capability that we can keep only a part of it in memory. The swap mechanism is abstracted out.
2. thesoup::types::DisjointSets - A class implementing the disjoint sets data structure. Please read its documentation to understand what exactly is it that it does.

## Graph functions
There are some functions that work on graph classes.
1. thesoup::algorithms::bfs - Breadth first search graph traversal.
2. thesoup::algorithms::kruskal - Kruskal's algorithm for computing the minimum cost spanning tree.

# Getting and building
## Build the code
Clone this repo and use meson to build and install. The only dependency is [Catch2](https://github.com/catchorg/Catch2). Build and install that first.
```bash
git clone https://github.com/amartya00/cppsoup.git
cd cppsoup
mkdir _build && cd _build
meson ..
ninja
ninja test
sudo ninja install
```
## Build the documentation
Documentation is built using doxygen. The usage is explained in the docs. To build the docs, navigate to the root folder `cppsoup` and run the following command:
```bash
doxygen .doxyfile
```
The HTML will be created in a `html/` folder. Start by opening the `index.html` file and navigate around.