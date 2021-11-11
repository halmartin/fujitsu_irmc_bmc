#!/usr/bin/env python

#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#-------------------------------------------------------------------------------------------------------

KernelVersion="3.14.17-ami"

#-------------------------------------------------------------------------------------------------------
#			Rules for Extracting Source
#-------------------------------------------------------------------------------------------------------
def extract_source():

	return 0

#-------------------------------------------------------------------------------------------------------
#			Rules for Clean Source Directory
#-------------------------------------------------------------------------------------------------------
def clean_source():


	retval = Py_Cwd(PrjVars["KERNEL_SRC"]+"/linux")
	if retval != 0:
		return retval 

	retval=Py_SetEnv("ARCH","arm")			# Change to linux specific ARCH name 
	if retval != 0:
		return retval 

	retval =Py_RunMake("clean");
	if retval != 0:
		return retval 

	retval =Py_RunMake("distclean");
	if retval != 0:
		return retval 

	retval=Py_SetEnv("ARCH","ARM")			# Restore SPX spcecific ARCH name
	if retval != 0:
		return retval 

	return 0

#-------------------------------------------------------------------------------------------------------
#			Rules for Building Source
#-------------------------------------------------------------------------------------------------------
def build_source():

	
	SrcFile="%s/%s/spx/DEFCONFIG"%(PrjVars["BUILD"],PrjVars["PACKAGE"])
	DestFile="%s/linux/.config"%(PrjVars["KERNEL_SRC"])
	TempDir="%s/%s"%(PrjVars["TEMPDIR"],PrjVars["PACKAGE"])

	Py_AddMacro("SPX_KERNEL_VERSION",KernelVersion)

	retval = Py_CopyFile(SrcFile,DestFile)
	if retval != 0:
		return retval 

	retval = Py_Cwd(PrjVars["KERNEL_SRC"]+"/linux")
	if retval != 0:
		return retval 

	retval=Py_SetEnv("ARCH","arm")
	if retval != 0:
		return retval 

	retval =Py_RunMake("oldconfig");
	if retval != 0:
		return retval 

	retval =Py_RunMake("uImage KALLSYMS_EXTRA_PASS=1");
	if retval != 0:
		return retval 

	retval =Py_RunMake("modules");
	if retval != 0:
		return retval 


	retval = Py_MkdirClean("%s/modules"%(TempDir))
	if retval != 0:
		return retval 

	retval =Py_RunMake("modules_install INSTALL_MOD_PATH=%s/modules"%(TempDir))
	if retval != 0:
		return retval 

	retval = Py_MkdirClean("%s/kernel-dev"%(TempDir))
	if retval != 0:
		return retval 

	retval =Py_RunMake("headers_install INSTALL_HDR_PATH=%s/kernel-dev"%(TempDir))
	if retval != 0:
		return retval 

	retval=Py_SetEnv("ARCH","ARM")
	if retval != 0:
		return retval 

	return 0


#-------------------------------------------------------------------------------------------------------
#			 Rules for Creating (Packing) Binary Package
#-------------------------------------------------------------------------------------------------------
def build_package_Kernel_ex():
	return Py_PackSPX("./boot/kernel","%s/linux/arch/arm/boot/uImage"%(PrjVars["KERNEL_SRC"]))

def build_package_Kernel_dev_ex():
	TempDir="%s/%s"%(PrjVars["TEMPDIR"],PrjVars["PACKAGE"])
		
	retval = Py_MkdirClean("%s/tmp"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/usr/include/linux"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/scripts"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/arch/arm/include"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/arch/arm/mach-hornet/include"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/linux"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/include/mach"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/fs/proc/"%(TempDir))
	if retval != 0:
		return retval
	retval = Py_MkdirClean("%s/tmp/drivers/mtd/spichips/"%(TempDir)) 
	if retval != 0:
		return retval
	Py_CopyDir("%s/linux/include"%(PrjVars["KERNEL_SRC"]),"%s/tmp/usr/include/linux"%(TempDir))
	Py_CopyDir("%s/linux/scripts"%(PrjVars["KERNEL_SRC"]),"%s/tmp/scripts"%(TempDir))
	Py_CopyFile("%s/linux/arch/arm/Makefile"%(PrjVars["KERNEL_SRC"]),"%s/tmp/arch/arm/"%(TempDir))
	Py_CopyDir("%s/linux/arch/arm/include"%(PrjVars["KERNEL_SRC"]),"%s/tmp/arch/arm/include"%(TempDir))
	Py_CopyFile("%s/linux/arch/arm/mach-hornet/Makefile"%(PrjVars["KERNEL_SRC"]),"%s/tmp/arch/arm/mach-hornet/Makefile"%(TempDir))
	Py_CopyDir("%s/linux/arch/arm/mach-pilot/include/mach"%(PrjVars["KERNEL_SRC"]),"%s/tmp/include/mach"%(TempDir))
	Py_CopyFile("%s/linux/Makefile"%(PrjVars["KERNEL_SRC"]),"%s/tmp/Makefile"%(TempDir))
	Py_CopyFile("%s/linux/Module.symvers"%(PrjVars["KERNEL_SRC"]),"%s/tmp/Module.symvers"%(TempDir))
	Py_CopyDir("%s/kernel-dev/include/linux"%(TempDir),"%s/tmp/linux"%(TempDir))
	Py_CopyFile("%s/linux/fs/proc/internal.h"%(PrjVars["KERNEL_SRC"]),"%s/tmp/fs/proc/"%(TempDir))
	Py_CopyFile("%s/linux/.config"%(PrjVars["KERNEL_SRC"]),"%s/tmp/.config"%(TempDir))
	Py_CopyFile("%s/linux/COPYING"%(PrjVars["KERNEL_SRC"]),"%s/tmp/COPYING"%(TempDir))
	Py_CopyFile("%s/linux/drivers/mtd/spichips/spiflash.h"%(PrjVars["KERNEL_SRC"]),"%s/tmp/drivers/mtd/spichips/"%(TempDir))
	return Py_PackSPX("./","%s/tmp"%(TempDir))

def build_package_Kernel_modules_ex():

	TempDir="%s/%s"%(PrjVars["TEMPDIR"],PrjVars["PACKAGE"])
	KernelVer=PrjVars["SPX_KERNEL_VERSION"]

	retval = Py_Delete("%s/modules/lib/modules/%s/source"%(TempDir,KernelVer))
	if retval != 0:
		return retval 

	retval = Py_Delete("%s/modules/lib/modules/%s/build"%(TempDir,KernelVer))
	if retval != 0:
		return retval 

	return Py_PackSPX("./","%s/modules"%(TempDir))

#-------------------------------------------------------------------------------------------------------


