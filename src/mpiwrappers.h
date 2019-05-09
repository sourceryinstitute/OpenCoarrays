#ifndef _MPIWRAPPERS_H
#define _MPIWRAPPERS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wrapper of the original MPI_Alloc_mem that allows to allocate memory for a
 * given process, useful in combination with MPI windows. The Info hints can
 * be provided for allocations based in storage.
 */
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);

/**
 * Wrapper of the original MPI_Free_mem that allows to release memory that
 * was allocated with MPI_Alloc_mem. The mapping to storage is also removed
 * if the memory was allocated by giving the Info hints.
 */
int MPI_Free_mem(void *base);

/**
 * Wrapper of the original MPI_Win_create that allows to create an MPI window,
 * given a memory buffer that has been pre-allocated.
 */
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, 
                   MPI_Comm comm, MPI_Win *win);

/**
 * Wrapper of the original MPI_Win_allocate that allows to create MPI windows
 * based on storage devices, alongside the traditional memory-based. The Info
 * hints can be provided for allocations based in storage.
 */
int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, 
                     MPI_Comm comm, void *baseptr, MPI_Win *win);

/**
 * Wrapper of the original MPI_Win_sync that allows to force the changes on an
 * MPI window to be flushed to storage. If the window was allocated in RAM, the
 * original implementation is used.
 */
int MPI_Win_sync(MPI_Win win);

/**
 * Wrapper of the original MPI_Win_attach that allows to attach a memory
 * allocation to an MPI dynamic window. The given address will be cached
 * inside the window if it was previously allocated with MPI_Alloc_mem.
 */
int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size);

/**
 * Wrapper of the original MPI_Win_detach that allows to detach a memory
 * allocation inside an MPI dynamic window. If the allocation was cached,
 * the attribute will be removed from the window to avoid issues.
 */
int MPI_Win_detach(MPI_Win win, const void *base);

/**
 * Wrapper of the original MPI_Init_thread that prevents the use of the library
 * on multithreaded applications (i.e., the implementation is not thread-safe).
 */
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided);
  int MPISwin_init();
#ifdef __cplusplus
}
#endif

#endif

