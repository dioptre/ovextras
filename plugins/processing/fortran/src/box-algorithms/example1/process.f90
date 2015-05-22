
!
! This code illustrates passing a matrix in from OpenViBE, making modifications in-place,
! and sending it back. Actually the changes could be directly made into the input
! matrix but care must be taken that the difference in C/Fortran row/column major order
! of the matrix storage is taken into account.
!
! This example adds the channel number to the channel data (so each channel will have noticable and different effect)
!

! this routine is run once at the beginning of playback. it can be used to precompute/load things needed later.
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine fortran_init(errorCode) bind(C)
	use iso_c_binding
	INTEGER(C_INT32_T), intent(out) :: errorCode

	! in this example, the routine does nothing much
	
	write(*,*) "Init in Fortran"
	
	errorCode = 0	
end subroutine fortran_init

! this routine is runs once for each matrix chunk of data. It can modify the matrix in place.
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine fortran_process(mat,rows,cols,errorCode) bind(C)
	use iso_c_binding
	INTEGER(C_INT32_T), intent(in) :: rows,cols
	INTEGER(C_INT32_T), intent(out) :: errorCode
	REAL(C_DOUBLE), intent(inout) :: mat(rows*cols)
	REAL, DIMENSION(1:rows,1:cols) :: A; 
	! write(*,*) "Process in Fortran" , rows, " ", cols
   
	! matrix from C/C++ is in row-major order, reshape
	A = reshape(mat, (/ rows, cols /), ORDER = (/ 2, 1 /) )
   
	! Add +nChannel to each channel
	do i=1,rows
		do j=1,cols
			A(i,j) = A(i,j) + i;
		end do
	end do	
 
	! print the matrix
	!WRITE(*,*)
	!DO I = LBOUND(A,1), UBOUND(A,1)
	!  WRITE(*,*) (A(I,J), J = LBOUND(A,2), UBOUND(A,2))
	!END DO
   
	mat = reshape( TRANSPOSE(A), (/ rows * cols /) )
   
	errorCode = 0
end subroutine fortran_process

! this routine is run once at the end of playback. it can be used to deallocate resources (memory, files, ...).
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine fortran_uninit(errorCode) bind(C)
	use iso_c_binding
	INTEGER(C_INT32_T), intent(out) :: errorCode
	
	! in this example, the routine does nothing much
	
	write(*,*) "Uninit in Fortran"
	
	errorCode = 0	
end subroutine fortran_uninit

