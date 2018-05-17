/* Test suite for ISO_Fortran_binding.h */
#include "../../../iso_fortran_binding/ISO_Fortran_binding.h"
#include <stdio.h>
#include <stdlib.h>

int main (void)
{

  CFI_rank_t      rank;
  CFI_attribute_t attribute;
  CFI_type_t      type[10] = {CFI_type_Bool,        CFI_type_short,
                         CFI_type_ucs4_char,   CFI_type_double,
                         CFI_type_float128,    CFI_type_float128_Complex,
                         CFI_type_long_double, CFI_type_long_double_Complex,
                         CFI_type_struct,      CFI_type_other};
  size_t elem_len;
  int    ind;
  size_t base_type;
  size_t base_type_size;
  size_t errno;

  /* Test function establish. */
  /* Fresh descriptor, base address is NULL. */
  printf ("Test CFI_establish: dv.base_addr == NULL.\n\n");
  /* Loop through type. */
  for (int i = 0; i < 10; i++)
    {
      elem_len = 0;
      if (type[i] != CFI_type_struct && type[i] != CFI_type_other)
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = (type[i] - base_type) >> CFI_type_kind_shift;
        }
      else
        {
          base_type      = type[i];
          base_type_size = elem_len;
        }
      /* Loop through attribute. */
      for (int j = 1; j <= 3; j++)
        {
          attribute = j;
          /* Loop through rank. */
          for (int k = 0; k <= CFI_MAX_RANK; k++)
            {
              errno = 1;
              rank  = k;
              CFI_CDESC_T (rank) test1;
              /* We do this because C sometimes doesn't make the structures with
               * a null base_addr which leads to weird behaviour inside
               * CFI_establish.
               */
              if (test1.base_addr != NULL)
                {
                  test1.base_addr = NULL;
                  free (test1.base_addr);
                }
              ind = CFI_establish ((CFI_cdesc_t *) &test1, NULL, attribute,
                                   type[i], elem_len, rank, NULL);
              printf ("attribute = %d\ntype = %d\nbase_type = %ld\nrank = "
                      "%d\nelem_len = %ld\n",
                      attribute, type[i], base_type, rank, elem_len);
              if (ind != CFI_SUCCESS)
                {
                  printf ("CFI_establish return value = %d\n", ind);
                  errno *= 2;
                  printf ("errno = %ld\n\n", errno);
                  goto next_attribute1;
                }
              if (attribute != test1.attribute)
                {
                  printf ("Attribute fail.\n");
                  errno *= 3;
                }
              if (type[i] != test1.type)
                {
                  printf ("Type fail.\n");
                  errno *= 5;
                }
              if (rank != test1.rank)
                {
                  printf ("Rank fail.\n");
                  errno *= 7;
                }

              elem_len = base_type_size;
              if (base_type_size == 10)
                {
                  elem_len = 64;
                }
              if (base_type == CFI_type_Complex)
                {
                  elem_len *= 2;
                }
              if (elem_len != test1.elem_len)
                {
                  printf ("Element length fail: type_idx = %d., elem_len = "
                          "%ld\n",
                          i, elem_len);
                  errno *= 11;
                }
              printf ("errno = %ld\n\n", errno);
            }
        next_attribute1:;
        }
    }

  /* Fresh descriptor, base address is not NULL */
  printf ("Test CFI_establish: dv.base_addr != NULL.\n\n");
  CFI_index_t *extents = NULL;
  /* Loop through type. */
  for (int i = 0; i < 10; i++)
    {
      elem_len = 0;
      if (type[i] != CFI_type_struct && type[i] != CFI_type_other)
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = (type[i] - base_type) >> CFI_type_kind_shift;
        }
      else
        {
          base_type      = type[i];
          base_type_size = elem_len;
        }
      /* Loop through attribute. */
      for (int j = 1; j <= 3; j++)
        {
          attribute = j;
          /* Loop through rank. */
          for (int k = 0; k <= CFI_MAX_RANK; k++)
            {
              errno = 1;
              rank  = k;
              if (extents != NULL)
                {
                  free (extents);
                }
              extents = malloc (rank * sizeof (CFI_index_t));
              for (int r = 0; r < rank; r++)
                {
                  extents[r] = r + 1;
                }
              CFI_CDESC_T (rank) test2;
              /* We do this because C sometimes doesn't make the structures with
               * a null base_addr which leads to weird behaviour inside
               * CFI_establish.
               */
              if (test2.base_addr != NULL)
                {
                  test2.base_addr = NULL;
                  free (test2.base_addr);
                }
              ind = CFI_establish ((CFI_cdesc_t *) &test2, &ind, attribute,
                                   type[i], elem_len, rank, extents);
              printf ("attribute = %d\ntype = %d\nbase_type = %ld\nrank = "
                      "%d\nelem_len = %ld\n",
                      attribute, type[i], base_type, rank, elem_len);
              if (ind != CFI_SUCCESS)
                {
                  printf ("CFI_establish return value = %d\n", ind);
                  errno *= 2;
                  printf ("errno = %ld\n\n", errno);
                  goto next_attribute2;
                }
              if (attribute != test2.attribute)
                {
                  printf ("Attribute fail.\n");
                  errno *= 3;
                }
              if (type[i] != test2.type)
                {
                  printf ("Type fail.\n");
                  errno *= 5;
                }
              if (rank != test2.rank)
                {
                  printf ("Rank fail.\n");
                  errno *= 7;
                }

              elem_len = base_type_size;
              if (base_type_size == 10)
                {
                  elem_len = 64;
                }
              if (base_type == CFI_type_Complex)
                {
                  elem_len *= 2;
                }
              if (elem_len != test2.elem_len)
                {
                  printf ("Element length fail: type_idx = %d., elem_len = "
                          "%ld must be equal.\n",
                          i, elem_len);
                  errno *= 11;
                }

              printf ("extents = [ ");
              for (int r = 0; r < rank; r++)
                {
                  if (extents[r] != test2.dim[r].extent)
                    {
                      printf ("Extents fail: extents[%d] = %ld., "
                              "dv.dim[%d].extent = %ld\n",
                              r, extents[r], r, test2.dim[r].extent);
                      errno *= 13;
                    }
                  printf ("%ld ", extents[r]);
                }
              printf ("]\n");

              if (attribute == CFI_attribute_pointer)
                {
                  printf ("test2.dim[].lower_bound = [ ");
                  for (int r = 0; r < rank; r++)
                    {
                      if (test2.dim[r].lower_bound != 0)
                        {
                          printf (
                              "Dimension lower bound fail: if the attribute is "
                              "for a pointer, the lower bounds of every "
                              "dimension must be zero, "
                              "test2.dim[%d].lower_bound = %ld.\n",
                              r, test2.dim[r].lower_bound);
                          errno *= 17;
                        }
                      printf ("%ld ", test2.dim[r].lower_bound);
                    }
                  printf ("]\n");
                }
              printf ("errno = %ld\n\n", errno);
            }
        next_attribute2:;
        }
    }

  /* Fresh descriptor, base address is not NULL */
  printf ("Test CFI_allocate: dv.base_addr != NULL.\n\n");
  CFI_index_t *lower = NULL;
  CFI_index_t *upper = NULL;
  /* Loop through type. */
  for (int i = 0; i < 10; i++)
    {
      elem_len = 0;
      if (type[i] == CFI_type_struct)
        {
          base_type      = type[i];
          base_type_size = 69;
        }
      else if (type[i] == CFI_type_other)
        {
          base_type      = type[i];
          base_type_size = 666;
        }
      else if (type[i] == CFI_type_char || type[i] == CFI_type_ucs4_char ||
               type[i] == CFI_type_signed_char)
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = 3;
        }
      else
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = (type[i] - base_type) >> CFI_type_kind_shift;
        }

      elem_len = base_type_size;
      if (base_type_size == 10)
        {
          elem_len = 64;
        }
      if (base_type == CFI_type_Complex)
        {
          elem_len *= 2;
        }
      /* Loop through attribute. */
      for (int j = 1; j <= 3; j++)
        {
          attribute = j;
          /* Loop through rank. */
          for (int k = 0; k <= CFI_MAX_RANK; k++)
            {
              errno = 1;
              rank  = k;
              if (extents != NULL)
                {
                  free (extents);
                }
              if (lower != NULL)
                {
                  free (lower);
                }
              if (upper != NULL)
                {
                  free (upper);
                }
              extents = malloc (rank * sizeof (CFI_index_t));
              lower   = malloc (rank * sizeof (CFI_index_t));
              upper   = malloc (rank * sizeof (CFI_index_t));
              for (int r = 0; r < rank; r++)
                {
                  extents[r] = 2;
                  lower[r]   = r;
                  upper[r]   = lower[r] + extents[r];
                }
              CFI_CDESC_T (rank) test3;
              /* We do this because C sometimes doesn't make the structures with
               * a null base_addr which leads to weird behaviour inside
               * CFI_establish.
               */
              if (test3.base_addr != NULL)
                {
                  test3.base_addr = NULL;
                  free (test3.base_addr);
                }
              ind = CFI_establish ((CFI_cdesc_t *) &test3, NULL, attribute,
                                   type[i], elem_len, rank, extents);
              ind = CFI_allocate ((CFI_cdesc_t *) &test3, lower, upper,
                                  base_type_size);
              printf ("type = %ld\nelem_len = %ld\n", base_type,
                      test3.elem_len);
              if (ind != CFI_SUCCESS)
                {
                  errno *= 2;
                  goto next_attribute3;
                }
              for (int r = 0; r < rank; r++)
                {
                  if (lower[r] != test3.dim[r].lower_bound)
                    {
                      printf (
                          "Dimension lower bound fail: lower[%d] = %ld, "
                          "test3.dim[%d].lower_bound = %ld must be equal.\n",
                          r, lower[r], r, test3.dim[r].lower_bound);
                      errno *= 3;
                    }
                  if (upper[r] - test3.dim[r].lower_bound + 1 !=
                      test3.dim[r].extent)
                    {
                      printf ("Extent fail: upper[%d] - "
                              "test3.dim[%d].lower_bound + 1 = %ld, "
                              "test3.dim[%d].extent = %ld must be equal.\n",
                              r, r, upper[r] - test3.dim[r].lower_bound + 1, r,
                              test3.dim[r].lower_bound);
                      errno *= 5;
                    }
                  if (test3.dim[r].sm != test3.elem_len)
                    {
                      printf ("Memory stride fail: test3.dim[%d].sm = %ld, "
                              "test3.elem_len = %ld must be equal.\n",
                              r, test3.dim[r].sm, test3.elem_len);
                      errno *= 7;
                    }
                }
              if (elem_len != test3.elem_len)
                {
                  printf ("Element length fail: type_idx = %d., elem_len = "
                          "%ld must be equal.\n",
                          i, elem_len);
                  errno *= 11;
                }
            }
        next_attribute3:;
          printf ("errno = %ld\n\n", errno);
        }
    }

  printf ("Test CFI_allocate: dv.base_addr == NULL.\n\n");
  rank  = 1;
  errno = 1;
  CFI_CDESC_T (rank) test4;
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (type[3] - base_type) >> CFI_type_kind_shift;
  attribute      = CFI_attribute_allocatable;
  ind = CFI_establish ((CFI_cdesc_t *) &test4, NULL, attribute, type[3],
                       elem_len, rank, NULL);
  ind = CFI_allocate ((CFI_cdesc_t *) &test4, NULL, NULL, base_type_size);
  if (ind != CFI_INVALID_EXTENT)
    {
      errno *= 2;
    }
  printf ("errno = %ld\n\n", errno);

  printf ("Test CFI_allocate: lower and upper == NULL.\n\n");
  rank  = 1;
  errno = 1;
  CFI_CDESC_T (rank) test5;
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (type[3] - base_type) >> CFI_type_kind_shift;
  attribute      = CFI_attribute_pointer;
  ind = CFI_establish ((CFI_cdesc_t *) &test5, &ind, attribute, type[3],
                       elem_len, rank, extents);
  ind = CFI_allocate ((CFI_cdesc_t *) &test5, NULL, NULL, base_type_size);
  if (ind != CFI_ERROR_BASE_ADDR_NOT_NULL)
    {
      errno *= 2;
    }
  printf ("errno = %ld\n\n", errno);

  /* Test CFI_deallocate. */
  printf ("Test CFI_deallocate.\n\n");
  rank           = 1;
  errno          = 1;
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (type[3] - base_type) >> CFI_type_kind_shift;
  for (int i = 1; i <= 3; i++)
    {
      attribute = i;
      if (extents != NULL)
        {
          free (extents);
        }
      if (lower != NULL)
        {
          free (lower);
        }
      if (upper != NULL)
        {
          free (upper);
        }
      extents = malloc (rank * sizeof (CFI_index_t));
      lower   = malloc (rank * sizeof (CFI_index_t));
      upper   = malloc (rank * sizeof (CFI_index_t));
      CFI_CDESC_T (rank) test6;
      ind = CFI_establish ((CFI_cdesc_t *) &test6, NULL, attribute, type[i],
                           elem_len, rank, extents);
      ind = CFI_allocate ((CFI_cdesc_t *) &test6, lower, upper, base_type_size);
      ind = CFI_deallocate ((CFI_cdesc_t *) &test6);
      if (ind != CFI_INVALID_ATTRIBUTE && test6.base_addr != NULL)
        {
          errno *= 2;
          printf ("Deallocation failed.\n");
        }
      printf ("attribute = %d\nerrno = %ld\n\n", test6.attribute, errno);
    }

  /* Test CFI_is_contiguous. */
  printf ("Test CFI_is_contiguous.\n\n");
  int tmp_ind;
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (type[3] - base_type) >> CFI_type_kind_shift;
  for (int i = 1; i <= 3; i++)
    {
      attribute = i;
      for (int j = 0; j <= 4; j++)
        {
          errno = 1;
          rank  = j;
          if (extents != NULL)
            {
              free (extents);
            }
          if (lower != NULL)
            {
              free (lower);
            }
          if (upper != NULL)
            {
              free (upper);
            }
          extents = malloc (rank * sizeof (CFI_index_t));
          lower   = malloc (rank * sizeof (CFI_index_t));
          upper   = malloc (rank * sizeof (CFI_index_t));
          for (int r = 0; r < rank; r++)
            {
              extents[r] = 2;
              lower[r]   = r;
              upper[r]   = lower[r] + extents[r];
            }
          CFI_CDESC_T (rank) test7;
          ind = CFI_establish ((CFI_cdesc_t *) &test7, NULL, attribute, type[3],
                               elem_len, rank, extents);
          tmp_ind = CFI_allocate ((CFI_cdesc_t *) &test7, lower, upper,
                                  base_type_size);
          ind = CFI_is_contiguous ((CFI_cdesc_t *) &test7);
          printf ("attribute = %d\nrank = %d\n", attribute, rank);
          if (ind != CFI_INVALID_RANK && rank == 0 &&
              tmp_ind != CFI_INVALID_ATTRIBUTE)
            {
              printf ("CFI_is_contiguous rank failure %d.\n", tmp_ind);
              errno *= 2;
            }
          else if (ind == CFI_ERROR_BASE_ADDR_NULL && test7.base_addr != NULL &&
                   tmp_ind != CFI_SUCCESS)
            {
              printf ("CFI_is_contiguous base addres failure.\n");
              errno *= 3;
            }
          printf ("errno = %ld\n\n", errno);
        }
    }

  /* Test CFI_address. */
  printf ("Test CFI_address.\n\n");
  CFI_index_t *tr_subscripts;
  CFI_dim_t *  tr_dim;
  /* Loop through type. */
  for (int i = 0; i < 10; i++)
    {
      elem_len = 0;
      if (type[i] == CFI_type_struct)
        {
          base_type      = type[i];
          base_type_size = 69;
        }
      else if (type[i] == CFI_type_other)
        {
          base_type      = type[i];
          base_type_size = 666;
        }
      else if (type[i] == CFI_type_char || type[i] == CFI_type_ucs4_char ||
               type[i] == CFI_type_signed_char)
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = 3;
        }
      else
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = (type[i] - base_type) >> CFI_type_kind_shift;
        }

      elem_len = base_type_size;
      if (base_type_size == 10)
        {
          elem_len = 64;
        }
      if (base_type == CFI_type_Complex)
        {
          elem_len *= 2;
        }
      /* Loop through attribute. */
      for (int j = 1; j <= 3; j++)
        {
          attribute = j;
          /* Loop through rank. */
          for (int k = 1; k <= CFI_MAX_RANK; k++)
            {
              errno = 1;
              rank  = k;
              CFI_CDESC_T (rank) source;
              if (extents != NULL)
                {
                  free (extents);
                }
              if (lower != NULL)
                {
                  free (lower);
                }
              if (upper != NULL)
                {
                  free (upper);
                }
              extents = malloc (rank * sizeof (CFI_index_t));
              lower   = malloc (rank * sizeof (CFI_index_t));
              upper   = malloc (rank * sizeof (CFI_index_t));
              for (int r = 0; r < rank; r++)
                {
                  extents[r] = rank - r + 1;
                  lower[r]   = rank - r - 3;
                  upper[r]   = lower[r] + extents[r] - 1;
                }
              ind = CFI_establish ((CFI_cdesc_t *) &source, NULL,
                                   CFI_attribute_allocatable, type[i], elem_len,
                                   rank, extents);
              ind = CFI_allocate ((CFI_cdesc_t *) &source, lower, upper,
                                  elem_len);
              if (ind == CFI_SUCCESS)
                {
                  CFI_index_t dif_addr;
                  CFI_index_t n_entries = 1;
                  dif_addr              = (CFI_index_t) (
                      (char *) CFI_address ((CFI_cdesc_t *) &source, upper) -
                      (char *) CFI_address ((CFI_cdesc_t *) &source, lower));
                  for (int r = 0; r < rank; r++)
                    {
                      n_entries = n_entries * (upper[r] - lower[r] + 1);
                    }
                  tr_subscripts = malloc (rank * sizeof (CFI_index_t));
                  tr_dim        = malloc (rank * sizeof (CFI_dim_t));
                  for (int i = 0; i < rank; i++)
                    {
                      CFI_index_t idx  = rank - i - 1;
                      tr_subscripts[i] = upper[idx];
                      tr_dim[i]        = source.dim[idx];
                      /* Normalise the subscripts to start counting the address
                       * from 0. */
                      tr_subscripts[i] -= tr_dim[i].lower_bound;
                    }
                  /* We assume column major order as that is how Fortran stores
                   * arrays. We
                   * calculate the memory address of the specified element via
                   * the canonical
                   * array dimension reduction map and multiplying by the memory
                   * stride. */
                  CFI_index_t index     = tr_subscripts[0] * tr_dim[0].sm;
                  CFI_index_t tmp_index = 1;
                  for (int i = 1; i < rank; i++)
                    {
                      tmp_index *= tr_subscripts[i] * tr_dim[i - 1].extent *
                                   tr_dim[i - 1].sm;
                      index += tmp_index;
                    }
                  free (tr_subscripts);
                  free (tr_dim);
                  if (index - dif_addr != 0)
                    {
                      errno *= 2;
                      printf ("Error CFI_address is not being calculated "
                              "properly.\n");
                    }
                  printf ("errno = %ld\n", errno);
                }
              else if (ind == CFI_ERROR_MEM_ALLOCATION)
                {
                  goto next_type;
                }
            }
        }
    next_type:;
      printf ("\n");
    }

  /* Test CFI_setpointer */
  printf ("Test CFI_setpointer: Checking component assignment.\n\n");
  for (int i = 0; i < CFI_MAX_RANK; i++)
    {
      rank           = i;
      errno          = 1;
      base_type      = type[3] & CFI_type_mask;
      base_type_size = (type[3] - base_type) >> CFI_type_kind_shift;
      attribute      = CFI_attribute_other;
      CFI_CDESC_T (rank) test8a, test8b;

      if (extents != NULL)
        {
          free (extents);
        }
      if (lower != NULL)
        {
          free (lower);
        }
      extents = malloc (rank * sizeof (CFI_index_t));
      lower   = malloc (rank * sizeof (CFI_index_t));
      for (int r = 0; r < rank; r++)
        {
          extents[r] = r + 1;
          lower[r]   = r - 2;
        }
      ind = CFI_establish ((CFI_cdesc_t *) &test8a, &ind, attribute, type[3],
                           base_type_size, rank, extents);
      for (int r = 0; r < rank; r++)
        {
          extents[r] = r + 2;
        }
      ind = CFI_establish ((CFI_cdesc_t *) &test8b, &errno, attribute, type[3],
                           base_type_size, rank, extents);
      ind = CFI_setpointer ((CFI_cdesc_t *) &test8a, (CFI_cdesc_t *) &test8b,
                            lower);
      for (int r = 0; r < rank; r++)
        {
          if (test8a.dim[r].lower_bound != lower[r])
            {
              printf ("CFI_setpointer failed reassign lower bounds.\n");
              printf ("test8a.dim[%d].lower_bound = %ld\tlower[%d] = %ld\n", r,
                      test8a.dim[r].lower_bound, r, lower[r]);
              errno *= 2;
            }
          if (test8a.dim[r].extent != test8b.dim[r].extent)
            {
              printf ("CFI_setpointer failed reassign lower bounds.\n");
              printf (
                  "test8a.dim[%d].extent = %ld\ttest8b.dim[%d].extent = %ld\n",
                  r, test8a.dim[r].extent, r, test8b.dim[r].extent);
              errno *= 3;
            }
          if (test8a.dim[r].sm != test8b.dim[r].sm)
            {
              printf ("CFI_setpointer failed reassign lower bounds.\n");
              printf ("test8a.dim[%d].sm = %ld\ttest8b.dim[%d].sm = %ld\n", r,
                      test8a.dim[r].sm, r, test8b.dim[r].sm);
              errno *= 5;
            }
        }
      if (test8a.base_addr != test8b.base_addr)
        {
          printf ("CFI_setpointer failed to reassign base address.\n");
          errno *= 7;
        }
      if (test8a.version != test8b.version)
        {
          printf ("CFI_setpointer failed to reassign version.\n");
          errno *= 11;
        }
      if (test8a.attribute != test8b.attribute)
        {
          printf ("CFI_setpointer failed to reassign attribute.\n");
          errno *= 13;
        }
      if (test8a.offset != test8b.offset)
        {
          printf ("CFI_setpointer failed to change lower bounds.\n");
          errno *= 17;
        }
      printf ("errno = %ld\n", errno);
    }
  printf ("\n");

  /* NULL source. */
  printf (
      "CFI_set_pointer: change of attribute to a CFI_attribute_pointer.\n\n");
  rank           = 10;
  errno          = 1;
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (type[3] - base_type) >> CFI_type_kind_shift;
  CFI_CDESC_T (rank) test9;

  if (extents != NULL)
    {
      free (extents);
    }
  if (lower != NULL)
    {
      free (lower);
    }
  extents = malloc (rank * sizeof (CFI_index_t));
  lower   = malloc (rank * sizeof (CFI_index_t));
  for (int r = 0; r < rank; r++)
    {
      extents[r] = r + 1;
      lower[r]   = r - 2;
    }
  ind = CFI_establish ((CFI_cdesc_t *) &test9, &ind, attribute, type[3],
                       base_type_size, rank, extents);
  ind = CFI_setpointer ((CFI_cdesc_t *) &test9, NULL, lower);
  if (test9.attribute != CFI_attribute_pointer)
    {
      printf ("CFI_establish failed to set attribute to pointer.\n");
      errno *= 2;
    }
  if (test9.base_addr != NULL)
    {
      printf ("CFI_establish failed to set base addres to NULL.\n");
      errno *= 3;
    }
  printf ("errno = %ld\n\n", errno);

  printf ("CFI_setpointer testing if statements.\n\n");
  rank      = 3;
  errno     = 1;
  attribute = CFI_attribute_other;
  CFI_CDESC_T (rank) test10a, test10b;
  if (extents != NULL)
    {
      free (extents);
    }
  if (lower != NULL)
    {
      free (lower);
    }
  extents = malloc (rank * sizeof (CFI_index_t));
  lower   = malloc (rank * sizeof (CFI_index_t));
  for (int r = 0; r < rank; r++)
    {
      extents[r] = r + 1;
      lower[r]   = r - 2;
    }
  base_type      = CFI_type_long & CFI_type_mask;
  base_type_size = (CFI_type_long - base_type) >> CFI_type_kind_shift;
  ind = CFI_establish ((CFI_cdesc_t *) &test10a, &ind, attribute, CFI_type_long,
                       base_type_size, rank, extents);
  for (int r = 0; r < rank; r++)
    {
      extents[r] = r + 2;
    }
  base_type      = CFI_type_double & CFI_type_mask;
  base_type_size = (CFI_type_double - base_type) >> CFI_type_kind_shift;
  ind            = CFI_establish ((CFI_cdesc_t *) &test10b, &errno, attribute,
                       CFI_type_double, base_type_size, rank, extents);
  ind = CFI_setpointer ((CFI_cdesc_t *) &test10a, (CFI_cdesc_t *) &test10b,
                        lower);
  if (ind != CFI_INVALID_TYPE)
    {
      printf ("CFI_setpointer failed to detect wrong types.\n");
      errno *= 2;
    }
  printf ("errno = %ld\n\n", errno);

  errno          = 1;
  base_type      = CFI_type_other & CFI_type_mask;
  base_type_size = 666;
  ind            = CFI_establish ((CFI_cdesc_t *) &test10a, &ind, attribute,
                       CFI_type_other, base_type_size, rank, extents);
  base_type      = CFI_type_other & CFI_type_mask;
  base_type_size = 69;
  ind            = CFI_establish ((CFI_cdesc_t *) &test10b, &errno, attribute,
                       CFI_type_other, base_type_size, rank, extents);
  ind = CFI_setpointer ((CFI_cdesc_t *) &test10a, (CFI_cdesc_t *) &test10b,
                        lower);
  if (ind != CFI_INVALID_ELEM_LEN)
    {
      printf ("CFI_setpointer failed to detect wrong element lengths.\n");
      errno *= 2;
    }
  printf ("errno = %ld\n\n", errno);

  errno          = 1;
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (CFI_type_long - base_type) >> CFI_type_kind_shift;
  ind = CFI_establish ((CFI_cdesc_t *) &test10a, &ind, attribute, type[3],
                       base_type_size, rank, extents);
  rank++;
  CFI_CDESC_T (rank) test10c;
  if (extents != NULL)
    {
      free (extents);
    }
  if (lower != NULL)
    {
      free (lower);
    }
  extents = malloc (rank * sizeof (CFI_index_t));
  lower   = malloc (rank * sizeof (CFI_index_t));
  for (int r = 0; r < rank; r++)
    {
      extents[r] = r + 1;
      lower[r]   = r - 2;
    }
  base_type      = CFI_type_other & CFI_type_mask;
  base_type_size = (CFI_type_long - base_type) >> CFI_type_kind_shift;
  ind = CFI_establish ((CFI_cdesc_t *) &test10c, &errno, attribute, type[3],
                       base_type_size, rank, extents);
  ind = CFI_setpointer ((CFI_cdesc_t *) &test10a, (CFI_cdesc_t *) &test10c,
                        lower);
  if (ind != CFI_INVALID_RANK)
    {
      printf ("CFI_setpointer failed to detect wrong element lengths.\n");
      errno *= 2;
    }
  printf ("errno = %ld\n\n", errno);

  /* Test CFI_section */
  printf ("Test CFI_section.\n\n");
  CFI_index_t *strides;
  printf ("Test bounds.\n");
  /* Loop through type. */
  for (int i = 0; i < 10; i++)
    {
      elem_len = 0;
      if (type[i] == CFI_type_struct)
        {
          base_type      = type[i];
          base_type_size = 69;
        }
      else if (type[i] == CFI_type_other)
        {
          base_type      = type[i];
          base_type_size = 666;
        }
      else if (type[i] == CFI_type_char || type[i] == CFI_type_ucs4_char ||
               type[i] == CFI_type_signed_char)
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = 3;
        }
      else
        {
          base_type      = type[i] & CFI_type_mask;
          base_type_size = (type[i] - base_type) >> CFI_type_kind_shift;
        }
      elem_len = base_type_size;
      if (base_type_size == 10)
        {
          elem_len = 64;
        }
      if (base_type == CFI_type_Complex)
        {
          elem_len *= 2;
        }
      /* Loop through rank. */
      for (int k = 1; k <= CFI_MAX_RANK; k++)
        {
          errno = 1;
          rank  = k;
          CFI_CDESC_T (rank) section, source;
          if (extents != NULL)
            {
              free (extents);
            }
          if (lower != NULL)
            {
              free (lower);
            }
          if (upper != NULL)
            {
              free (upper);
            }
          if (strides == NULL)
            {
              free (strides);
            }
          extents = malloc (rank * sizeof (CFI_index_t));
          lower   = malloc (rank * sizeof (CFI_index_t));
          upper   = malloc (rank * sizeof (CFI_index_t));
          strides = malloc (rank * sizeof (CFI_index_t));
          for (int r = 0; r < rank; r++)
            {
              extents[r] = rank - r + 10;
              lower[r]   = rank - r - 5;
              upper[r]   = lower[r] + extents[r] - 1;
            }
          ind = CFI_establish ((CFI_cdesc_t *) &source, NULL,
                               CFI_attribute_allocatable, type[i], elem_len,
                               rank, extents);
          ind = CFI_establish ((CFI_cdesc_t *) &section, NULL,
                               CFI_attribute_other, type[i], elem_len, rank,
                               NULL);
          ind = CFI_allocate ((CFI_cdesc_t *) &source, lower, upper, elem_len);
          if (ind == CFI_ERROR_MEM_ALLOCATION)
            {
              goto next_type2;
            }
          /* Lower is within bounds. */
          printf ("Lower is within bounds.\n");
          for (int r = 0; r < rank; r++)
            {
              lower[r]   = rank - r - 3;
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             lower, NULL, strides);
          if (ind != CFI_SUCCESS)
            {
              errno *= 2;
              printf ("CFI_section failed to properly assign lower "
                      "bounds.\n");
            }
          printf ("errno = %ld\n\n", errno);
          /* Lower is below lower bounds. */
          printf ("Lower is below bounds.\n");
          for (int r = 0; r < rank; r++)
            {
              lower[r]   = rank - r - 6;
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             lower, NULL, strides);
          if (ind != CFI_ERROR_OUT_OF_BOUNDS)
            {
              errno *= 3;
              printf ("CFI_section failed to properly assign lower "
                      "bounds.\n");
            }
          printf ("errno = %ld\n\n", errno);
          /* Lower is above upper bounds. */
          printf ("Lower is above bounds.\n");
          for (int r = 0; r < rank; r++)
            {
              lower[r]   = upper[r] + 1;
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             lower, NULL, strides);
          if (ind != CFI_ERROR_OUT_OF_BOUNDS)
            {
              errno *= 5;
              printf ("CFI_section failed to properly assign lower "
                      "bounds.\n");
            }
          for (int r = 0; r < rank; r++)
            {
              extents[r] = rank - r + 10;
              lower[r]   = rank - r - 5;
              upper[r]   = lower[r] + extents[r] - 1;
            }
          /* Lower is within bounds. */
          printf ("Lower is within bounds.\n");
          for (int r = 0; r < rank; r++)
            {
              upper[r]   = rank - r - 3;
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             NULL, upper, strides);
          if (ind != CFI_SUCCESS)
            {
              errno *= 7;
              printf ("CFI_section failed to properly assign upper "
                      "bounds.\n");
            }
          printf ("errno = %ld\n\n", errno);
          /* Lower is below lower bounds. */
          printf ("Lower is below bounds.\n");
          for (int r = 0; r < rank; r++)
            {
              upper[r]   = rank - r - 6;
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             NULL, upper, strides);
          if (ind != CFI_ERROR_OUT_OF_BOUNDS)
            {
              errno *= 11;
              printf ("CFI_section failed to properly assign upper "
                      "bounds.\n");
            }
          printf ("errno = %ld\n\n", errno);
          /* Lower is above upper bounds. */
          printf ("Lower is above bounds.\n");
          for (int r = 0; r < rank; r++)
            {
              upper[r]   = lower[r] + extents[r];
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             NULL, upper, strides);
          if (ind != CFI_ERROR_OUT_OF_BOUNDS)
            {
              errno *= 13;
              printf ("CFI_section failed to properly assign upper "
                      "bounds.\n");
            }
          printf ("\n");
          printf ("Test whether upper and lower bounds, and memory stride are "
                  "set properly.\n");
          for (int r = 0; r < rank; r++)
            {
              extents[r] = rank - r + 10;
              lower[r]   = rank - r - 3;
              upper[r]   = lower[r] + extents[r] - 3;
              strides[r] = r + 1;
            }
          ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source,
                             lower, upper, strides);
          for (int i = 0; i < rank; i++)
            {
              if (section.dim[i].lower_bound != lower[i])
                {
                  printf (
                      "CFI_section does not correctly assign lower bounds.\n");
                  errno *= 17;
                }
              if (section.dim[i].extent != upper[i] - lower[i] + 1)
                {
                  printf ("CFI_section does not correctly assign extent.\n");
                  errno *= 19;
                }
              if (section.dim[i].sm != strides[i] * section.elem_len)
                {
                  printf ("CFI_section does not correctly assign memory "
                          "strides.\n");
                  errno *= 23;
                }
            }
          printf ("errno = %ld\n\n", errno);
        }
    next_type2:;
      printf ("\n");
    }
  printf ("CFI_section trivial tests.\n\n");
  errno = 1;
  rank  = 1;
  CFI_CDESC_T (rank) section, source;
  if (extents != NULL)
    {
      free (extents);
    }
  if (lower != NULL)
    {
      free (lower);
    }
  if (upper != NULL)
    {
      free (upper);
    }
  if (strides == NULL)
    {
      free (strides);
    }
  extents = malloc (rank * sizeof (CFI_index_t));
  lower   = malloc (rank * sizeof (CFI_index_t));
  upper   = malloc (rank * sizeof (CFI_index_t));
  strides = malloc (rank * sizeof (CFI_index_t));
  for (int r = 0; r < rank; r++)
    {
      extents[r] = rank - r + 10;
      lower[r]   = rank - r - 5;
      upper[r]   = lower[r] + extents[r] - 1;
    }
  ind = CFI_establish ((CFI_cdesc_t *) &source, NULL, CFI_attribute_allocatable,
                       type[3], elem_len, rank, extents);
  ind = CFI_establish ((CFI_cdesc_t *) &section, NULL, CFI_attribute_other,
                       type[3], elem_len, rank, NULL);
  ind = CFI_allocate ((CFI_cdesc_t *) &source, lower, upper, elem_len);
  for (int r = 0; r < rank; r++)
    {
      lower[r]   = rank - r - 3;
      strides[r] = r + 1;
      upper[r]   = lower[r] + extents[r] - 3;
    }
  ind = CFI_section ((CFI_cdesc_t *) &section, NULL, lower, upper, strides);
  if (ind != CFI_INVALID_DESCRIPTOR)
    {
      printf ("CFI_section not picking up that source is NULL.\n");
      errno *= 2;
    }
  ind = CFI_section (NULL, (CFI_cdesc_t *) &source, lower, upper, strides);
  if (ind != CFI_INVALID_DESCRIPTOR)
    {
      printf ("CFI_section not picking up that source is NULL.\n");
      errno *= 3;
    }
  ind =
      CFI_establish ((CFI_cdesc_t *) &section, NULL, CFI_attribute_allocatable,
                     type[3], elem_len, rank, NULL);
  ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source, lower,
                     upper, strides);
  if (ind != CFI_INVALID_ATTRIBUTE)
    {
      printf (
          "CFI_section not accounting for the attribute of result properly.\n");
      errno *= 5;
    }
  ind = CFI_establish ((CFI_cdesc_t *) &section, NULL, CFI_attribute_other,
                       type[3], elem_len, rank, NULL);
  ind = CFI_deallocate ((CFI_cdesc_t *) &source);
  ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source, lower,
                     upper, strides);
  if (ind != CFI_ERROR_BASE_ADDR_NULL)
    {
      printf ("CFI_section not picking up that source->base_addr is NULL.\n");
      errno *= 7;
    }
  CFI_CDESC_T (0) section2, source2;
  ind = CFI_establish ((CFI_cdesc_t *) &source2, &ind, CFI_attribute_other,
                       type[3], 0, 0, NULL);
  ind = CFI_establish ((CFI_cdesc_t *) &section2, &errno, CFI_attribute_other,
                       type[3], 0, 0, NULL);
  ind = CFI_section ((CFI_cdesc_t *) &section2, (CFI_cdesc_t *) &source2, lower,
                     upper, strides);
  if (ind != CFI_INVALID_RANK)
    {
      printf ("CFI_section not picking up that source has rank.\n");
      errno *= 11;
    }
  ind = CFI_establish ((CFI_cdesc_t *) &source, NULL, CFI_attribute_allocatable,
                       type[3], 0, rank, extents);
  ind = CFI_establish ((CFI_cdesc_t *) &section, NULL, CFI_attribute_other,
                       type[6], 0, rank, NULL);
  ind = CFI_allocate ((CFI_cdesc_t *) &source, lower, upper, elem_len);
  for (int r = 0; r < rank; r++)
    {
      lower[r]   = rank - r - 3;
      strides[r] = r + 1;
      upper[r]   = lower[r] + extents[r] - 3;
    }
  ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source, lower,
                     upper, strides);
  if (ind != CFI_INVALID_ELEM_LEN)
    {
      printf ("CFI_section not picking up different element lengths of source "
              "and section.\n");
      errno *= 13;
    }
  ind = CFI_establish ((CFI_cdesc_t *) &section, NULL, CFI_attribute_other,
                       CFI_type_long, 0, rank, NULL);
  ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &source, lower,
                     upper, strides);
  if (ind != CFI_INVALID_TYPE)
    {
      printf ("CFI_section not picking up different types of source and "
              "section.\n");
      errno *= 17;
    }
  printf ("errno = %ld\n\n", errno);
  /*
  CFI_index_t *strides;
  rank = 1;
  CFI_CDESC_T (rank) section, source;
  if (lower != NULL)
    {
      free (lower);
    }
  if (upper != NULL)
    {
      free (upper);
    }
  lower   = malloc (rank * sizeof (CFI_index_t));
  strides = malloc (rank * sizeof (CFI_index_t));
  for (int r = 0; r < rank; r++)
    {
      extents[r] = r + 2;
      lower[r]   = r - 2;
      strides[r] = r + 1;
    }
  base_type      = type[3] & CFI_type_mask;
  base_type_size = (CFI_type_long - base_type) >> CFI_type_kind_shift;
  ind = CFI_establish ((CFI_cdesc_t *) &section, NULL, CFI_attribute_other,
                       type[3], base_type_size, rank, NULL);
  ind = CFI_section ((CFI_cdesc_t *) &section, (CFI_cdesc_t *) &section, lower,
  NULL, strides);
  */

  /* Test CFI_select_part */

  // // This sets the value "val" at position "offset" for a CFI array "arr"
  // with element size "arr.elem_len" described by CFI_cdesc_t.
  // memcpy ((char *)arr.base_addr + offset * arr.elem_len,
  //         (void *)& val, arr.elem_len);
  // // This is the memory address of the l'th element of the array (C indices).
  // char address = (char *)test3.base_addr + l * test3.elem_len;
  // // This is the value of the l'th element of the array (C indices).
  // my_type value = *(my_type*)((char *)test3.base_addr + l * test3.elem_len);
  const int INCOMPLETE_TEST = 1;
  return INCOMPLETE_TEST;
}
