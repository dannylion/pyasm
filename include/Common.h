/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2016 dannylion
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <fltKernel.h>
#include <intrin.h>

#ifndef STATIC
#define STATIC static
#endif

#define DEVICE_NAME			L"\\Device\\pyasm"
#define DEVICE_DOS_NAME		L"\\DosDevices\\pyasm"

// DriverIoControl codes
#define IOCTL_RDMSR	(CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_WRMSR	(CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1001, METHOD_BUFFERED, FILE_ANY_ACCESS))

#pragma pack(push, 1)

typedef struct _WRMSR_REQUEST
{
	UINT32	dwMsrCode;
	UINT64	qwValue;
} WRMSR_REQUEST, *PWRMSR_REQUEST;

#pragma pack(pop)

typedef struct _DRIVER_GLOBALS
{
	PDEVICE_OBJECT	ptDeviceObject;
	BOOLEAN			bSymbolicLinkCreated;
} DRIVER_GLOBALS, *PDRIVER_GLOBALS;

EXTERN_C DRIVER_GLOBALS g_tDriverGlobals;

#define IO_COMPLETE_IRP(ptIrp, eStatus, dwInformation)			\
	{															\
		if (NULL != ptIrp)										\
		{														\
			(ptIrp)->IoStatus.Information = (dwInformation);	\
			(ptIrp)->IoStatus.Status = (eStatus);				\
			IoCompleteRequest((ptIrp), IO_NO_INCREMENT);		\
		}														\
	}
