
!
! This code illustrates passing a matrix in from OpenViBE, making modifications,
! and sending it out. Actually the changes could be directly made in the input
! matrix but care must be taken that the differences in C/Fortran row/column major
! is taken into account.
!
! The code adds the channel number to the channel data (so each channel will have noticable and different effect)
!

subroutine fortran_init(errorCode) bind(C)
	use iso_c_binding
	INTEGER(C_INT32_T), intent(out) :: errorCode
	write(*,*) "Init in Fortran"
	
	errorCode = 0	
end subroutine fortran_init

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

subroutine fortran_uninit(errorCode) bind(C)
	use iso_c_binding
	INTEGER(C_INT32_T), intent(out) :: errorCode
	write(*,*) "Uninit in Fortran"
	
	errorCode = 0	
end subroutine fortran_uninit

