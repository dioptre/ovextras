
!
! This code illustrates passing a matrix in from OpenViBE, making modifications,
! and sending another matrix back. Care must be taken that the difference in C/Fortran 
! row/column major order of the matrix storage is taken into account.
!
! This example adds the channel number to the channel data (so each channel will have noticable and different effect).
! It also adds one channel with a constant pattern, to illustrate how to change the channel count. The
! change of dimensions must be specified in the box_process_header() function.
!

Module box

! Matrix dimensions in and out
INTEGER g_rowsIn,g_colsIn, g_rowsOut,g_colsOut
! Sampling rates in and out
INTEGER g_samplingRateIn, g_samplingRateOut

CONTAINS

! this routine is run once at the beginning of playback. it can be used to precompute/load things needed later.
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine box_init(inputParamsLength, inputParams,  errorCode) bind(C)
	!DEC$ ATTRIBUTES DLLEXPORT :: box_init
	use,intrinsic:: iso_c_binding
	implicit none
	INTEGER(C_INT32_T), intent(in) :: inputParamsLength;
	CHARACTER(KIND=C_CHAR), dimension(inputParamsLength), intent(in) :: inputParams
	INTEGER(C_INT32_T), intent(out) :: errorCode;

	! in this example, the routine does nothing much
	
	write(*,*) "box_init() in Fortran"
	write(*,*) "  Received params: ", inputParams
	
	errorCode = 0	
end subroutine box_init

! this routine is run when OpenViBE receives a stream header. The purpose of this
! function is to return the sizes of the output matrices, and change in the
! sampling rate if any. If sampling rate is 0, the input of process()
! will be just a matrix without a known sampling frequency. 
!
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine box_process_header(rowsIn,colsIn,samplingRateIn,rowsOut,colsOut,samplingRateOut,errorCode) bind(C)
	!DEC$ ATTRIBUTES DLLEXPORT :: box_process_header
	use iso_c_binding
	INTEGER(C_INT32_T), intent(in) :: rowsIn, colsIn, samplingRateIn
	INTEGER(C_INT32_T), intent(out) :: rowsOut, colsOut, samplingRateOut
	INTEGER(C_INT32_T), intent(out) :: errorCode;
	
	g_rowsIn = rowsIn
	g_colsIn = colsIn
	g_samplingRateIn = samplingRateIn	
	
! Here rowOut=rowIn+1 since we want to add a channel in this example
	g_rowsOut = rowsIn + 1 
	g_colsOut = colsIn
	g_samplingRateOut = samplingRateIn
	
	write(*,*) "  Process header: Dims", g_rowsIn, g_colsIn,  "->", g_rowsOut, g_colsOut
	write(*,*) "                  Freq", g_SamplingRateIn, "->", g_samplingRateOut
	
	rowsOut = g_rowsOut
	colsOut = g_colsOut
	
	samplingRateOut = g_samplingRateOut
	
	errorCode = 0;
end subroutine box_process_header
	
! this routine is runs once for each matrix chunk of data.
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine box_process(matIn,matOut,errorCode) bind(C)
	!DEC$ ATTRIBUTES DLLEXPORT :: box_process
	use iso_c_binding
	REAL(C_DOUBLE), intent(in) :: matIn(g_rowsIn*g_colsIn)	
	REAL(C_DOUBLE), intent(out) :: matOut(g_rowsOut*g_colsOut)
	INTEGER(C_INT32_T), intent(out) :: errorCode	
	REAL, DIMENSION(1:g_rowsIn,1:g_colsIn) :: A; 
	REAL, DIMENSION(1:g_rowsOut,1:g_colsOut) :: B; 	
	! write(*,*) "box_process() in Fortran" , rows, " ", cols
  
	! matrix from C/C++ is in row-major order, reshape
	A = reshape(matIn, (/ g_rowsIn, g_colsIn /), ORDER = (/ 2, 1 /) )
   
	! Add +nChannel to each channel
	do i=1,g_rowsIn
		do j=1,g_colsIn
			B(i,j) = A(i,j) + i;
		end do
	end do

	! the new channel just has a simple pattern
	do j=1,g_colsOut
	   B(g_rowsOut,j) = j;
	end do
 
	! print the matrix
	!WRITE(*,*)
	!DO I = LBOUND(B,1), UBOUND(B,1)
	!  WRITE(*,*) (B(I,J), J = LBOUND(B,2), UBOUND(B,2))
	!END DO
   
	matOut = reshape( TRANSPOSE(B), (/ g_rowsOut * g_colsOut /) )
   
	errorCode = 0
end subroutine box_process

! this routine is run once at the end of playback. it can be used to deallocate resources (memory, files, ...).
! set errorCode to 0 on success, a non-zero code otherwise.
subroutine box_uninit(errorCode) bind(C)
	!DEC$ ATTRIBUTES DLLEXPORT :: box_uninit
	use iso_c_binding
	INTEGER(C_INT32_T), intent(out) :: errorCode
	
	! in this example, the routine does nothing much
	
	write(*,*) "box_uninit() in Fortran"
	
	errorCode = 0	
end subroutine box_uninit

end module box

