# 
# The MIT License (MIT)
# 
# Copyright (c) 2016 dannylion
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 

import sys
import IPython
import win32api
import win32file
from ctypes import (
	sizeof,
	c_uint64)
from construct import (
	Container, 
	Struct,
	ULInt32,
	ULInt64)
	
import MsrCodes

# Windows constants
FILE_DEVICE_UNKNOWN = 0x22
METHOD_BUFFERED 	= 0
METHOD_IN_DIRECT 	= 1
METHOD_OUT_DIRECT 	= 2
FILE_ANY_ACCESS 	= 0

def CTL_CODE(DeviceType, Function, Method, Access):
	"""Calculate a DeviceIoControl code just like in the driver's C code"""
	return (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

class BaseDriverIO(object):
	def openHandle(self):
		if None is self._driverHandle:
			self._driverHandle = win32file.CreateFile(
				"\\\\.\\pyasm", 
				win32file.GENERIC_READ | win32file.GENERIC_WRITE, 
				0, 
				None, 
				win32file.OPEN_EXISTING, 
				0, 
				None)
			
	def closeHandle(self):
		if None is not self._driverHandle:
			win32file.CloseHandle(self._driverHandle)
			self._driverHandle = None
	
	def __init__(self, bOpenHandle = True, logFunc = lambda s: sys.stdout.write("%s\n" % s)):
		self.log = logFunc
		self._driverHandle = None
		if bOpenHandle:
			self.openHandle()

	def __del__(self):
		self.closeHandle()
	
	def needOpenHandle(func):
		"""Raise an exception if decorated function is called when handle is not open"""
		def needOpenHandle_inner(self, *args, **kwargs):
			if None is self._driverHandle: 
				raise BaseDriverIOException('Need to open handle to driver (See openHandle)')
			return func(self, *args, **kwargs)
		return needOpenHandle_inner
		
	@needOpenHandle
	def _ioctl(self, ioctlCode, buffer = "", outSize = 0):
		self.log("Sending ioctl %s: '%s'" % (
			hex(ioctlCode), 
			buffer.encode("hex"),))
		return win32file.DeviceIoControl(self._driverHandle, ioctlCode, buffer, outSize)

class PyasmDriverIO(BaseDriverIO):
	# IOCTL codes
	_IOCTL_RDMSR	= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS)
	_IOCTL_WRMSR	= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1001, METHOD_BUFFERED, FILE_ANY_ACCESS)
	
	# IOCTL Structures
	_IOCTL_RDMSR_STRUCT	= Struct(
		"_IOCTL_RDMSR",
		ULInt32("dwMsrCode"))
	
	_IOCTL_WRMSR_STRUCT	= Struct(
		"_IOCTL_WRMSR",
		ULInt32("dwMsrCode"),
		ULInt64("qwValue"))
	
	def __init__(self, bOpenHandle = True, logFunc = lambda s: sys.stdout.write("%s\n" % s)):
		super(PyasmDriverIO, self).__init__(bOpenHandle, logFunc)
		
	def rdmsr(self, dwMsrCode):
		raw_data = self._ioctl(
			self._IOCTL_RDMSR, 
			self._IOCTL_RDMSR_STRUCT.build(Container(dwMsrCode = dwMsrCode)),
			sizeof(c_uint64))
		return int(raw_data[::-1].encode("hex"), 16)
			
	def wrmsr(self, dwMsrCode, qwValue):
		return self._ioctl(
			self._IOCTL_WRMSR, 
			self._IOCTL_WRMSR_STRUCT.build(Container(dwMsrCode = dwMsrCode, qwValue = qwValue)),
			0)

def main():
	pyasm = PyasmDriverIO()
	
	print "\n" + "="*70
	print "Use 'pyasm' object to perform assembly instruction in kernel, such as 'rdmsr', 'wrmsr', ..."
	print "Happy Hacking ;)"
	print "="*70 + "\n"
	
	IPython.embed()
	
if '__main__' == __name__:
	main()
