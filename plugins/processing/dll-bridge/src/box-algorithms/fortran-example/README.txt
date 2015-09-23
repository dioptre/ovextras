
Example how to use a Fortran component to do signal processing

On Windows,

- Compile the Fortran code with a 32(!) bit fortran compiler. 
The Intel Fortran compiler has been observed to work. e.g.

> where ifort
C:\Program Files (x86)\Intel\Composer XE 2013 SP1\bin\ia32\ifort

> ifort /static /dll fortranbox_dll.f90

The resulting .dll file can be specified to the DLL Bridge box. 
Remember to set the full path correctly.

Using the /static flag allows you to distribute your built dll
to other computers more easily.


On Linux,

# gfortran -shared fortranbox_dll.f90 -o fortranbox_dll.so

Then specify the .so file to the DLL Bridge box.


