program main
  !! author: Damian Rouson
  !!
  !! Test co_broadcast with derived-type actual arguments
  implicit none

  integer, parameter :: sender=1 !! co_broadcast source_image
  character(len=*), parameter :: text="text" !! character message data

  associate(me=>this_image())

    test_non_allocatable: block
      type parent
        integer :: heritable=0
      end type

      type component
        integer :: subcomponent=0
      end type

      type, extends(parent) :: child
        type(component) a
        character(len=len(text)) :: c="", z(0)
        complex :: i=(0.,0.), j(1)=(0.,0.)
        integer :: k=0,       l(2,3)=0
        real    :: r=0.,      s(3,2,1)=0.
        logical :: t=.false., u(1,2,3, 1,2,3, 1,2,3, 1,2,3, 1,2,3)=.false.
      end type

      type(child) message
      type(child) :: content = child( &
        parent=parent(heritable=-2), a=component(-1), c=text, z=[character(len=len(text))::],  &
        i=(0.,1.), j=(2.,3.), k=4, l=5, r=7., s=8., t=.true., u=.true.  &
      )
      if (me==sender) message = content

      call co_broadcast(message,source_image=sender)

      associate( failures => [                                &
        message%parent%heritable /= content%parent%heritable, &
        message%a%subcomponent /= content%a%subcomponent,     &
        message%c /= content%c,                               &
        message%z /= content%z,                               &
        message%i /= content%i,                               &
        message%j /= content%j,                               &
        message%k /= content%k,                               &
        message%l /= content%l,                               &
        message%r /= content%r,                               &
        message%s /= content%s,                               &
        message%t .neqv. content%t,                           &
        any( message%u .neqv. content%u )                     &
      ] )

        if ( any(failures) ) error stop "Test failed in non-allocatable block."

      end associate

    end block test_non_allocatable

     test_allocatable: block
       type dynamic
         character(len=:), allocatable :: string
         complex, allocatable :: scalar
         integer, allocatable :: vector(:)
         logical, allocatable :: matrix(:,:)
         real, allocatable ::  superstring(:,:,:, :,:,:, :,:,:, :,:,:, :,:,: )
       end type

       type(dynamic) alloc_message, alloc_content

       alloc_content = dynamic(                                               &
         string=text,                                                         &
         scalar=(0.,1.),                                                      &
         vector=reshape( [integer::], [0]),                                   &
         matrix=reshape( [.true.], [1,1]),                                         &
         superstring=reshape([1,2,3,4], [2,1,2, 1,1,1, 1,1,1, 1,1,1, 1,1,1 ]) &
       )

       if (me==sender) alloc_message = alloc_content

       call co_broadcast(alloc_message,source_image=sender)

       associate( failures => [                                 &
         alloc_message%string /= alloc_content%string,          &
         alloc_message%scalar /= alloc_content%scalar,          &
         alloc_message%vector /= alloc_content%vector,          &
         alloc_message%matrix .neqv. alloc_content%matrix,      &
         alloc_message%superstring /= alloc_content%superstring &
       ] )

         if ( any(failures) ) error stop "Test failed in allocatable block."

       end associate

     end block test_allocatable

    sync all  ! Wait for each image to pass the test
    if (me==sender) print *,"Test passed."

  end associate

end program main
