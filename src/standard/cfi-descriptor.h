#ifndef CFI_DESCRIPTOR_H_
#define CFI_DESCRIPTOR_H_


#include <iostream>
#include <stddef.h>  // Standard ptrdiff_t tand size_t
#include <stdint.h>  // Integer types

#include "../iso-fortran-binding/ISO_Fortran_binding.h"


#define CFI_MAX_DIMENSIONS 15


typedef struct CFI_base_descriptor_t
{
   void *base_addr;
   size_t elem_len;
   int version;
   CFI_rank_t rank;
   CFI_attribute_t attribute;
   CFI_type_t type;  
} CFI_base_descriptor_t;

/* Define the descriptor of max rank.
 * 
 *  This typedef is made to allow storing a copy of a remote descriptor on the
 *  stack without having to care about the rank. */
typedef struct cfi_max_dim_descriptor_t
{
   CFI_base_descriptor_t base;
   CFI_dim_t dim[CFI_MAX_DIMENSIONS];
} cfi_max_dim_descriptor_t;

typedef struct cfi_dim1_descriptor_t
{
   CFI_base_descriptor_t base;
   CFI_dim_t dim[1];
} cfi_dim1_descriptor_t;


// inline?
template <class DescType> class Descriptor
{
 public:
   Descriptor(DescType* dst) { desc = dst; }
   ~Descriptor() {}

   DescType*   get_desc()                     { return desc;                           }
   void*       get_base_addr()                { return desc->base_addr;                }
   void        set_base_addr(void* i)         { desc->base_addr = i;                   }
   size_t      get_elem_len()                 { return (int)desc->elem_len;            }
   void        set_elem_len(size_t i)         { desc->elem_len = i;                    }
// int         get_version();
// void        set_version(int i);
   CFI_rank_t  get_rank()                     { return (int)desc->rank;                }
   void        set_rank(int i)                { desc->rank = CFI_rank_t(i);            }
// CFI_attribute_t get_attribute();
// void        set_attribute();
   CFI_type_t  get_type()                     { return (int)desc->type;                }
   void        set_type(int i)                { desc->type = (CFI_type_t)(i);          }
   CFI_index_t get_lower_bound(int i)         { return  desc->dim[i].lower_bound;      }
   CFI_index_t get_upper_bound(int i)         { return (desc->dim[i].lower_bound
                                                        + desc->dim[i].extent - 1);    }
   void        set_lower_bound(int i, int lb) { desc->dim[i].lower_bound =
                                                CFI_index_t(lb);                       }
   CFI_index_t get_extent(int i)              { return desc->dim[i].extent;            }
   void        set_extent(int i, int ex)      { desc->dim[i].extent = CFI_index_t(ex); }
   CFI_index_t get_sm(int i)                  { return desc->dim[i].sm;                }
   void        set_sm(int i, int sm)          { desc->dim[i].sm = CFI_index_t(sm);     }
   // static max function
   // static size of function

 private:
   DescType* desc;
} ;

#endif
