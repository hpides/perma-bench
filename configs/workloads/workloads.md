# Workloads
Workload configs that depict typical database access patterns.
## Index Look
The [index lookup](index_lookup.yaml) benchmark depicts accesses to a tree-structured index.
It consists of two benchmarks: the first is a pure index lookup, i.e., small random reads, and the second is a mixed workload - an update - that run small random read and write operations.  
## Intermediate Results
The [intermediate results](intermediate_result.yaml) benchmark represents the usage of persistent memory for intermediate results of joins, aggregations, etc.
Hereby, the accesses are similar to hash map inserts, i.e., small random writes.
## Large Persistent Copy
The [large persistent copy](large_persistent_copy.yaml) benchmark relates to a workload for a shutdown, or similar, of a database.
It consists of large sequential writes to persistent memory.
## Logging
The [logging](logging.yaml) workload adapts logging to persistent memory.
Thereby, persistent memory is accessed through small sequential writes. 
## Page Propagation
The [page propagation](page_propagation.yaml) workload imitates the behaviour of the buffer manager in a database.
Accesses to persistent memory are characterised by page sized random writes for the first benchmark.
The second benchmark consists of a mixed workload in which pages are written and read randomly from memory.
Both benchmarks are executed following a zipfian distribution and a uniform distribution.
Further, the writes are either persisted through non-temporal writes, i.e., writes that bypass the cache, or without any persist instruction by falling back to enhanced ADR.
## Table Scan
The [table scan](table_scan.yaml) benchmark depicts the simple database workload of reading a table, i.e., reading a table with various access sizes sequentially.
