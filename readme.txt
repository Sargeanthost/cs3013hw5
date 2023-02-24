Carlos Medina, Gabriel Curet-Irizarry
cemedina, gocuretirizarry

Our design is to split the input array into chunks and have each thread work on the indexes inside their designated
chunk. This is done with the thread_args_t struct. This keeps track of the main array used to flush the thread computations
to, and also stores details of its corresponding thread in the threads array. The barriers are used to ensure
in sync reads and writes.

Work is divided among threads evenly based on how many threads are allowed to be created. As the amount of work decreases,
some threads will lose any work to do. This is fine as by this point, the load on the other threads as decreased. It will
eventually become "single" threaded, as the only thread that can stay within the bounds is the first thread. For example,
index := 0 + offset := 2048 => 2048, and so anyone who has an index that is at the offset + the chunk for large enough
chunk values cant do anything. This is fine and maintains concurrency as the single thread, and thus the whole program,
now only has 1/nth amount of work left to do.

This maximizes concurrency because each thread is doing about the same amount of small amount work.
This avoids deadlock as no thread relies on the resources of another thread. An old values array is copied to ensure that
nothing the other threads do will impact any other thread and will preserve data integrity.
