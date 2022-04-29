#!/bin/bash

# ----------------------------------------------------------------------
# the following is for experiments
# bulkload keys must be sorted
./keygen 100000000 sort k100m

# search keys are randomly ordered
./keygen 100000000 random search100m

# insert keys are randomly ordered and are not in bulkload keys
#./getinsert 50000000 k50m 500000 insert500k

# delete keys are randomly ordered and must be in bulkload keys
#./getdelete 50000000 k50m 500000 delete500k
