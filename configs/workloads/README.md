# Workloads
Workload configs that represent typical database access patterns.
## Index Look
The [index lookup](index_lookup.yaml) benchmark depicts accesses to a tree-structured index.
It consists of two benchmarks: the first is a pure index lookup, i.e., small random reads, and the second is a mixed workload - an update - that runs small random read and write operations.
## Intermediate Results
The [intermediate results](intermediate_result.yaml) benchmark represents the usage of persistent memory for intermediate results of joins, aggregations, etc.
The access is similar to hash map inserts, i.e., small random writes, as hash maps are a widely used data structure for joins and aggregations.
## Large Persistent Copy
The [large persistent copy](large_persistent_copy.yaml) benchmark relates to a workload for a shutdown, or similar, of a database.
It consists of large sequential writes to persistent memory.
This could be, e.g., persisting a volatile data structure or data array.
## Logging
The [logging](logging.yaml) workload adapts logging to persistent memory through small sequential writes.
## Page Propagation
The [page propagation](page_propagation.yaml) workload imitates the behaviour of the buffer manager in a database.
Access to persistent memory is characterised by page-sized random writes in the first benchmark.
The second benchmark consists of a mixed workload in which pages are written and read randomly from persistent memory.
Both benchmarks are executed following a zipfian distribution and a uniform distribution.
## Table Scan
The [table scan](table_scan.yaml) benchmark represents the standard database workload of reading a table, i.e., sequentially reading a table with various access sizes and thread counts.
