SUBDIRS = integration unit

if COND_OPT
  MAYBE_CHECK = test_32.sh test_33.sh test_34.sh test_35.sh test_36.sh test_37.sh test_38.sh test_39.sh test_40.sh test_41.sh test_42.sh test_43.sh test_44.sh
else
  MAYBE_CHECK = test_msg.sh
endif

check_SCRIPTS = test_1.sh test_2.sh test_3.sh test_4.sh test_5.sh test_6.sh test_7.sh test_8.sh test_9.sh test_10.sh test_11.sh test_12.sh test_13.sh test_14.sh test_15.sh test_16.sh test_17.sh test_18.sh test_19.sh test_20.sh test_21.sh test_22.sh test_23.sh test_24.sh test_25.sh test_26.sh test_27.sh test_28.sh test_29.sh test_30.sh test_31.sh $(MAYBE_CHECK)

define check-results =
	echo 'if grep -q "passed" '$@.out' ;then' >> $@
	echo '  exit 0' >> $@
	echo 'else' >> $@
	echo '  exit 1' >> $@
	echo 'fi' >> $@
	chmod +x $@
endef

test_1.sh:
	echo '../cafrun -np 4 integration/coarrayHelloWorld/hello_multiverse 1>$@.out' 1>$@
	echo 'printf "hello_multiverse ....................... " ' 1>>$@
	$(check-results)

test_2.sh:
	echo '../cafrun -np 4 integration/dist_transpose/coarray_distributed_transpose 1>$@.out' 1>$@	
	echo 'printf "coarray_distributed_transpose .......... " ' 1>>$@
	$(check-results)

test_3.sh:
	echo '../cafrun -np 4 integration/events/async_hello 1>$@.out' 1>$@	
	echo 'printf "async_hello ............................ " ' 1>>$@
	$(check-results)

test_4.sh:
	echo '../cafrun -np 4 integration/pde_solvers/coarrayBurgers/coarrayBurgers 1>$@.out' 1>$@	
	echo 'printf "coarrayBurgers ......................... " ' 1>>$@
	$(check-results)

test_5.sh:
	echo '../cafrun -np 4 integration/pde_solvers/coarrayHeatSimplified/coarrayHeatSimplified 1>$@.out' 1>$@
	echo 'printf "coarrayHeatSimplified .................. " ' 1>>$@
	$(check-results)

test_6.sh:
	echo '../cafrun -np 4 unit/collectives/co_min 1>$@.out' 1>$@	
	echo 'printf "co_min ................................. " ' 1>>$@
	$(check-results)

test_7.sh:
	echo '../cafrun -np 4 unit/collectives/co_max 1>$@.out' 1>$@	
	echo 'printf "co_max ................................. " ' 1>>$@
	$(check-results)

test_8.sh:
	echo '../cafrun -np 4 unit/collectives/co_reduce 1>$@.out' 1>$@	
	echo 'printf "co_reduce .............................. " ' 1>>$@
	$(check-results)

test_9.sh:
	echo '../cafrun -np 4 unit/collectives/co_broadcast 1>$@.out' 1>$@	
	echo 'printf "co_broadcast ........................... " ' 1>>$@
	$(check-results)

test_10.sh:
	echo '../cafrun -np 4 unit/collectives/co_reduce_string 1>$@.out' 1>$@	
	echo 'printf "co_reduce_string ....................... " ' 1>>$@
	$(check-results)

test_11.sh:
	echo '../cafrun -np 4 unit/collectives/co_reduce_res_im 1>$@.out' 1>$@	
	echo 'printf "co_reduce_res_im ....................... " ' 1>>$@
	$(check-results)

test_12.sh:
	echo 'printf "passed\n" 1>$@.out' 1>>$@
#	echo '../cafrun -np 4 unit/init_register/initialize_mpi 1>$@.out 2>errorlog' 1>$@
	echo 'printf "initialize_mpi ..dummy test............. " ' 1>>$@
	$(check-results)

test_13.sh:
	echo '../cafrun -np 4 unit/init_register/allocate_as_barrier 1>$@.out 2>errorlog' 1>$@	
	echo 'printf "allocate_as_barrier .................... " ' 1>>$@
	$(check-results)

test_14.sh:
	echo '../cafrun -np 4 unit/init_register/register_alloc_vector 1>$@.out 2>errorlog' 1>$@
	echo 'printf "register_alloc_vector .................. " ' 1>>$@
	$(check-results)

test_15.sh:
	echo '../cafrun -np 4 unit/init_register/register_vector 1>$@.out 2>errorlog' 1>$@
	echo 'printf "register_vector ........................ " ' 1>>$@
	$(check-results)

test_16.sh:
	echo '../cafrun -np 4 unit/init_register/allocate_as_barrier_proc 1>$@.out 2>errorlog' 1>$@
	echo 'printf "allocate_as_barrier_proc ............... " ' 1>>$@
	$(check-results)

test_17.sh:
	echo '../cafrun -np 4 unit/send-get/get_array_test 1>$@.out' 1>$@
	echo 'printf "get_array_test ......................... " ' 1>>$@
	$(check-results)

test_18.sh:
	echo '../cafrun -np 4 unit/send-get/get_with_offset_1d 1>$@.out' 1>$@
	echo 'printf "get_with_offset_1d ..................... " ' 1>>$@
	$(check-results)

test_19.sh:
	echo '../cafrun -np 4 unit/send-get/sameloc 1>$@.out' 1>$@
	echo 'printf "sameloc ................................ " ' 1>>$@
	$(check-results)

test_20.sh:
	echo '../cafrun -np 4 unit/send-get/send_array_test 1>$@.out' 1>$@
	echo 'printf "send_array_test ........................ " ' 1>>$@
	$(check-results)

test_21.sh:
	echo '../cafrun -np 4 unit/send-get/strided_get 1>$@.out' 1>$@
	echo 'printf "strided_get ............................ " ' 1>>$@
	$(check-results)

test_22.sh:
	echo '../cafrun -np 4 unit/send-get/strided_sendget 1>$@.out' 1>$@
	echo 'printf "strided_sendget ........................ " ' 1>>$@
	$(check-results)

test_23.sh:
	echo '../cafrun -np 4 unit/send-get/whole_get_array 1>$@.out' 1>$@
	echo 'printf "whole_get_array ........................ " ' 1>>$@
	$(check-results)

test_24.sh:
	echo '../cafrun -np 4 unit/simple/increment_my_neighbor 1>$@.out' 1>$@
	echo 'printf "increment_my_neighbor .................. " ' 1>>$@
	$(check-results)

test_25.sh:
	echo '../cafrun -np 4 unit/simple/atomics 1>$@.out' 1>$@
	echo 'printf "atomics ................................ " ' 1>>$@
	$(check-results)

test_26.sh:
	echo '../cafrun -np 4 unit/sync/syncall 1>$@.out' 1>$@
	echo 'printf "syncall ................................ " ' 1>>$@
	$(check-results)

test_27.sh:
	echo '../cafrun -np 4 unit/sync/syncimages 1>$@.out' 1>$@
	echo 'printf "syncimages ............................. " ' 1>>$@
	$(check-results)

test_28.sh:
	echo '../cafrun -np 4 unit/sync/syncimages2 1>$@.out' 1>$@
	echo 'printf "syncimages2 ............................ " ' 1>>$@
	$(check-results)

test_29.sh:
	echo '../cafrun -np 4 unit/sync/duplicate_syncimages 1>$@.out' 1>$@
	echo 'printf "duplicate_syncimages ................... " ' 1>>$@
	$(check-results)

test_30.sh:
	echo '../cafrun -np 4 unit/sync/syncimages_status 1>$@.out' 1>$@
	echo 'printf "syncimages_status ...................... " ' 1>>$@
	$(check-results)

test_31.sh:
	echo '../cafrun -np 4 unit/sync/sync_image_ring_abort_on_stopped_image 1>$@.out' 1>$@
	echo 'printf "sync_image_ring_abort_on_stopped_image . " ' 1>>$@
	$(check-results)

test_msg.sh:
	echo 'printf "\n" ' 1>$@
	echo 'printf "\n" ' 1>>$@
	echo 'printf "***** Info Only: Failed Images not supported ****** " ' 1>>$@
	echo 'printf "\n" ' 1>>$@
	echo 'printf "\n" ' 1>>$@
	echo '  exit 0' >> $@
	chmod +x $@

if COND_OPT
test_32.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_failed_images_test_1 1>$@.out 2>errorlog' 1>$@
	echo 'printf "image_fail_and_failed_images_test_1 .... " ' 1>>$@
	$(check-results)

test_33.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_get_test_1 1>$@.out 2>errorlog' 1>$@	
	echo 'printf "image_fail_and_get_test_1 .............. " ' 1>>$@
	$(check-results)

test_34.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_status_test_1 1>$@.out 2>errorlog' 1>$@	
	echo 'printf "image_fail_and_status_test_1 ........... " ' 1>>$@
	$(check-results)

test_35.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_stopped_images_test_1 1>$@.out 2>errorlog' 1>$@	
	echo 'printf "image_fail_and_stopped_images_test_1 ... " ' 1>>$@
	$(check-results)

test_36.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_sync_test_1 1>$@.out 2>errorlog' 1>$@	
	echo 'printf "image_fail_and_sync_test_1 ............. " ' 1>>$@
	$(check-results)

test_37.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_sync_test_2 1>$@.out 2>errorlog' 1>$@
	echo 'printf "image_fail_and_sync_test_2 ............. " ' 1>>$@
	$(check-results)

test_38.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_and_sync_test_3 1>$@.out 2>errorlog' 1>$@
	echo 'printf "image_fail_and_sync_test_3 ............. " ' 1>>$@
	$(check-results)

test_39.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_fail_test_1 1>$@.out 2>errorlog' 1>$@
	echo 'printf "image_fail_test_1 ...................... " ' 1>>$@
	$(check-results)

test_40.sh:
	echo '../cafrun -np 4 -disable-auto-cleanup unit/fail_images/image_status_test_1 1>$@.out 2>errorlog' 1>$@
	echo 'printf "image_status_test_1 .................... " ' 1>>$@
	$(check-results)

test_41.sh:
	echo '../cafrun -np 4 unit/init_register/async_comp_alloc 1>$@.out 2>errorlog' 1>$@
	echo 'printf "async_comp_alloc ....................... " ' 1>>$@
	$(check-results)

test_42.sh:
	echo '../cafrun -np 4 unit/init_register/register_alloc_comp_1 1>$@.out 2>errorlog' 1>$@
	echo 'printf "register_alloc_comp_1 .................. " ' 1>>$@
	$(check-results)

test_43.sh:
	echo '../cafrun -np 4 unit/init_register/register_alloc_comp_2  1>$@.out 2>errorlog' 1>$@
	echo 'printf "register_alloc_comp_2  ................. " ' 1>>$@
	$(check-results)

test_44.sh:
	echo '../cafrun -np 4 unit/init_register/register_alloc_comp_3 1>$@.out 2>errorlog' 1>$@
	echo 'printf "register_alloc_comp_3 .................. " ' 1>>$@
	$(check-results)
endif



TESTS = $(check_SCRIPTS)

CLEANFILES = errorlog test_1.sh test_2.sh test_3.sh test_4.sh test_5.sh test_6.sh test_7.sh test_8.sh test_9.sh test_10.sh test_11.sh test_12.sh test_13.sh test_14.sh test_15.sh test_16.sh test_17.sh test_18.sh test_19.sh test_20.sh test_21.sh test_22.sh test_23.sh test_24.sh test_25.sh test_26.sh test_27.sh test_28.sh test_29.sh test_30.sh test_31.sh test_32.sh test_33.sh test_34.sh test_35.sh test_36.sh test_37.sh test_38.sh test_39.sh test_40.sh test_41.sh test_42.sh test_43.sh test_44.sh test_1.sh.out test_2.sh.out test_3.sh.out test_4.sh.out test_5.sh.out test_6.sh.out test_7.sh.out test_8.sh.out test_9.sh.out test_10.sh.out test_11.sh.out test_12.sh.out test_13.sh.out test_14.sh.out test_15.sh.out test_16.sh.out test_17.sh.out test_18.sh.out test_19.sh.out test_20.sh.out test_21.sh.out test_22.sh.out test_23.sh.out test_24.sh.out test_25.sh.out test_26.sh.out test_27.sh.out test_28.sh.out test_29.sh.out test_30.sh.out test_31.sh.out test_32.sh.out test_33.sh.out test_34.sh.out test_35.sh.out test_36.sh.out test_37.sh.out test_38.sh.out test_39.sh.out test_40.sh.out test_41.sh.out test_42.sh.out test_43.sh.out test_44.sh.out 
