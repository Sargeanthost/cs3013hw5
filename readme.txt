Carlos Medina, Gabriel Curet-Irizarry
cemedina, gocuretirizarry

Checkpoint: works with single thread. Believe there is a synchronization issue on writes that causes a read while there
are still writes going on. Will mess with placement of barrier and perhaps have a guiding loop in main

Our design is to split the input array into chunks and have each thread work on the indexes inside their designated
chunk. This is done with the thread_args_t struct. This keeps track of the main array used to flush the thread computations
to, and also stores details of what the parallel thread in the threads array should have. The barriers are used to ensure
syncd reads and writes. This maximizes concurrency because each thread is doing about the same amount of small amount work
so each thread isnt going to be waiting at the barrier for long and will operate as if their small work does all the work.
This avoides deadlock as no thread relies on the resources of another thread. an old values array is copied to ensure that
nothing the other threads do will impact any other thread
