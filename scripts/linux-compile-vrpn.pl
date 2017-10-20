#!/usr/bin/perl

# Intended to be run from linux-install_dependencies.pl
# 
# Variables are wrt that parent scope
#

if (!$no_install) {
  
    my $old_dir = Cwd::getcwd();
 
    my $vrpn_src_dir = $dependencies_arch_dir . "/vrpn";

    # fetch the packages
    chdir "$dependencies_arch_dir";
    if (! -e "vrpn_07_31-ov.zip") {
      system('wget "http://openvibe.inria.fr/dependencies/linux-x86/vrpn_07_31-ov.zip"');
      ($CHILD_ERROR != 0) and die ("Could not download the itpp external sources [$CHILD_ERROR]");
    }
    if (! -e $vrpn_src_dir) {
      system('unzip "vrpn_07_31-ov.zip"');
      ($CHILD_ERROR != 0) and die ("Could not extract the itpp external archive");
    }
    
    # compile
  
    print "Compiling vrpn ...\n";

    my $bitness = `uname -m`;
    chomp($bitness);

    my $bitness_token = "";
    if($bitness == "x86_64") {
	# 64bit
	$bitness_token = "pc_linux64";
        
        system('sed -i "s/#HW_OS := pc_linux$/HW_OS := pc_linux64/g" vrpn/quat/Makefile');
        system('sed -i "s/#HW_OS := pc_linux$/HW_OS := pc_linux64/g" vrpn/Makefile');
    } else {
	$bitness_token = "pc_linux";
 
	system('sed  -i "s/#HW_OS := pc_linux$/HW_OS := pc_linux/g" vrpn/quat/Makefile');
        system('sed  -i "s/CC := gcc/CC := gcc -fPIC/g" vrpn/quat/Makefile');
        system('sed  -i "s/#HW_OS := pc_linux$/HW_OS := pc_linux/g" vrpn/Makefile');
        system('sed  -i "s/CC := gcc/CC := gcc -fPIC/g" vrpn/Makefile');
        system('sed  -i "s/CC := g++/CC := g++ -fPIC/g" vrpn/Makefile');
    }

    system("cd vrpn/quat && make >make-quat.log 2>&1");
    system("cd vrpn && make >make-vrpn.log 2>&1");

    mkdir("$dependencies_dir/include");
    mkdir("$dependencies_dir/lib");
    
    system("chmod a-x vrpn/quat/*.h vrpn/*.h");
    system("cp vrpn/quat/*.h vrpn/*.h $dependencies_dir/include");
    system("cp vrpn/quat/$bitness_token/*.a vrpn/$bitness_token/*.a $dependencies_dir/lib");

    chdir $old_dir
};

