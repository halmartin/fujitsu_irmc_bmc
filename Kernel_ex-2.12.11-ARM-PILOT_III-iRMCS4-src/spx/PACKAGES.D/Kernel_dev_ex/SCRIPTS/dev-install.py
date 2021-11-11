#!/usr/bin/env python

#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *


#--------------------------------------- Extra Python modules -------------------------------------------
#
#-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():

	retval = Py_MkdirClean("%s/linux"%(PrjVars["SPXINC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/linux"%(PrjVars["SPXINC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/include"%(PrjVars["KERNEL_SRC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/scripts"%(PrjVars["KERNEL_SRC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/arch/arm/include"%(PrjVars["KERNEL_SRC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/arch/arm/mach-hornet/include"%(PrjVars["KERNEL_SRC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/include/mach"%(PrjVars["KERNEL_SRC"]))
	if retval != 0: 
		return retval
	retval = Py_MkdirClean("%s/linux/fs"%(PrjVars["KERNEL_SRC"]))
	if retval != 0:
		return retval
	TempDir="%s/%s"%(PrjVars["TEMPDIR"],PrjVars["PACKAGE"])
	Py_CopyDir("./usr/include/linux","%s/linux"%(PrjVars["SPXINC"]))
	Py_CopyDir("./linux","%s/linux/linux"%(PrjVars["SPXINC"]))
	Py_CopyDir("./usr/include/linux","%s/linux/include"%(PrjVars["KERNEL_SRC"]))
	Py_CopyDir("./scripts","%s/linux/scripts"%(PrjVars["KERNEL_SRC"]))
	Py_CopyFile("./arch/arm/Makefile","%s/linux/arch/arm/Makefile"%(PrjVars["KERNEL_SRC"]))
	Py_CopyDir("./arch/arm/include","%s/linux/arch/arm/include"%(PrjVars["KERNEL_SRC"]))
	Py_CopyDir("./include/mach","%s/linux/include/mach"%(PrjVars["KERNEL_SRC"]))
	Py_CopyFile("./arch/arm/mach-hornet/Makefile","%s/linux/arch/arm/mach-hornet/Makefile"%(PrjVars["KERNEL_SRC"]))
	Py_CopyDir("./arch/arm/mach-hornet/include","%s/linux/arch/arm/mach-hornet/include"%(PrjVars["KERNEL_SRC"]))
	Py_CopyFile("./Makefile","%s/linux/Makefile"%(PrjVars["KERNEL_SRC"]))
	Py_CopyDir("./fs","%s/linux/fs"%(PrjVars["KERNEL_SRC"]))
	Py_CopyDir("./drivers","%s/linux/drivers"%(PrjVars["KERNEL_SRC"]))
	Py_CopyFile("./.config","%s/linux/.config"%(PrjVars["KERNEL_SRC"]))
	Py_CopyFile("./COPYING","%s/linux/COPYING"%(PrjVars["KERNEL_SRC"]))
	return Py_CopyFile("./Module.symvers","%s/linux/Module.symvers"%(PrjVars["KERNEL_SRC"]))

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install
def debug_install():
	return 0



#-------------------------------------------------------------------------------------------------------
