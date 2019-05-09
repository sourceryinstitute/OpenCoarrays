#ifndef _MPI_STORAGE_WINDOWS_KEYS_H
#define _MPI_STORAGE_WINDOWS_KEYS_H

// MPI Storage Windows keys
#define MPI_SWIN_ALLOC_TYPE "alloc_type"    // Type ({ "memory", "storage" })
#define MPI_SWIN_IMPL_TYPE  "swin_impl"     // Impl. type ({ "mmap", "ummap" })
#define MPI_SWIN_FILENAME   "swin_filename" // File-path for the mapping
#define MPI_SWIN_OFFSET     "swin_offset"   // Offset inside the file
#define MPI_SWIN_UNLINK     "swin_unlink"   // Delete file ({ "true", "false" })
#define MPI_SWIN_SEG_SIZE   "swin_segsize"  // Segment size
#define MPI_SWIN_FLUSH_INT  "swin_flushint" // Flush interval
#define MPI_SWIN_READ_FILE  "swin_readfile" // Read the input file
#define MPI_SWIN_PTYPE      "swin_ptype"    // Policy type for out-of-core
#define MPI_SWIN_ORDER      "swin_order"    // Order of the allocation
#define MPI_SWIN_FACTOR     "swin_factor"   // Allocation factor

// MPI I/O supported keys
#define MPI_IO_ACCESS_STYLE    "access_style"    // Access style of the window
#define MPI_IO_FILE_PERM       "file_perm"       // Permission of file mapping
#define MPI_IO_STRIPING_FACTOR "striping_factor" // Stripe count for the file
#define MPI_IO_STRIPING_UNIT   "striping_unit"   // Stripe unit for the file

#endif

