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

#include <fltKernel.h>
#include <windef.h>
#include <InitGuid.h>
#include <devguid.h>
#include <wdmsec.h>
#include <intrin.h>

#include "Common.h"
#include "Asm.h"

DRIVER_GLOBALS g_tDriverGlobals = {0};

STATIC
NTSTATUS
DriverEmptyIrpStub(
	_In_ PDEVICE_OBJECT ptDeviceObject,
	_In_ PIRP ptIrp
)
{
	NTSTATUS eStatus = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(ptDeviceObject);

	PAGED_CODE();

	IO_COMPLETE_IRP(ptIrp, eStatus, 0);
	return eStatus;
}

STATIC
VOID
DriverUnload(
	PDRIVER_OBJECT ptDriverObject
)
{
	UNICODE_STRING usDosDeviceName = RTL_CONSTANT_STRING(DEVICE_DOS_NAME);

	UNREFERENCED_PARAMETER(ptDriverObject);

	if (g_tDriverGlobals.bSymbolicLinkCreated)
	{
		IoDeleteSymbolicLink(&usDosDeviceName);
		g_tDriverGlobals.bSymbolicLinkCreated = FALSE;
	}
	if (NULL != g_tDriverGlobals.ptDeviceObject)
	{
		IoDeleteDevice(g_tDriverGlobals.ptDeviceObject);
		g_tDriverGlobals.ptDeviceObject = NULL;
	}
}

STATIC
NTSTATUS
DriverIoControl(
	__in PDEVICE_OBJECT ptDeviceObject,
	__in PIRP ptIrp
)
{
	PIO_STACK_LOCATION ptIoStackLocation = NULL;
	NTSTATUS eStatus = STATUS_UNSUCCESSFUL;
	DWORD dwInformation = 0;

	UNREFERENCED_PARAMETER(ptDeviceObject);

	PAGED_CODE();

	if (NULL == ptIrp)
	{
		eStatus = STATUS_INVALID_PARAMETER;
		goto lblCleanup;
	}

	ptIoStackLocation = IoGetCurrentIrpStackLocation(ptIrp);
	if (NULL == ptIoStackLocation)
	{
		eStatus = STATUS_UNEXPECTED_IO_ERROR;
		goto lblCleanup;
	}
	switch (ptIoStackLocation->Parameters.DeviceIoControl.IoControlCode)
	{
	
	case IOCTL_RDMSR:
		{
			DWORD dwMsrCode = 0;
			
			if (sizeof(UINT32) > ptIoStackLocation->Parameters.DeviceIoControl.InputBufferLength)
			{
				eStatus = STATUS_INVALID_PARAMETER;
				goto lblCleanup;
			}
			if (sizeof(UINT64) > ptIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength)
			{
				eStatus = STATUS_BUFFER_TOO_SMALL;
				goto lblCleanup;
			}

			dwMsrCode = *((PUINT32)ptIrp->AssociatedIrp.SystemBuffer);
			*((PUINT64)ptIrp->AssociatedIrp.SystemBuffer) = __readmsr(dwMsrCode);
			dwInformation = sizeof(UINT64);
		}
		break;
	case IOCTL_WRMSR:
		{
			PWRMSR_REQUEST ptWrmsrRequest = NULL;

			if (sizeof(WRMSR_REQUEST) > ptIoStackLocation->Parameters.DeviceIoControl.InputBufferLength)
			{
				eStatus = STATUS_INVALID_PARAMETER;
				goto lblCleanup;
			}
			ptWrmsrRequest = (PWRMSR_REQUEST)ptIrp->AssociatedIrp.SystemBuffer;

			__writemsr(ptWrmsrRequest->dwMsrCode, ptWrmsrRequest->qwValue);
		}
		break;
	default:
		eStatus = STATUS_INVALID_DEVICE_REQUEST;
		goto lblCleanup;
	}

	eStatus = STATUS_SUCCESS;

lblCleanup:
	IO_COMPLETE_IRP(ptIrp, eStatus, dwInformation);
	return eStatus;
}


NTSTATUS
DriverEntry(
	PDRIVER_OBJECT ptDriverObject,
	PUNICODE_STRING RegistryPath
)
{
	NTSTATUS		eStatus			= STATUS_UNSUCCESSFUL;
	UNICODE_STRING	usDeviceName	= RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING	usDosDeviceName	= RTL_CONSTANT_STRING(DEVICE_DOS_NAME);

	PAGED_CODE();
	NT_ASSERT(NULL != ptDriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	ptDriverObject->DriverUnload = DriverUnload;

	eStatus = IoCreateDeviceSecure(
		ptDriverObject, 
		0, 
		&usDeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX,
		&GUID_DEVCLASS_SYSTEM,
		&g_tDriverGlobals.ptDeviceObject);
	if (!NT_SUCCESS(eStatus))
	{
		goto lblCleanup;
	}

	eStatus = IoCreateSymbolicLink(&usDosDeviceName, &usDeviceName);
	if (!NT_SUCCESS(eStatus))
	{
		goto lblCleanup;
	}
	g_tDriverGlobals.bSymbolicLinkCreated = TRUE;

	// We must provide these 2 major functions, even if they don't do anything
	ptDriverObject->MajorFunction[IRP_MJ_CREATE]	= &DriverEmptyIrpStub;
	ptDriverObject->MajorFunction[IRP_MJ_CLOSE]		= &DriverEmptyIrpStub;

	ptDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &DriverIoControl;

	g_tDriverGlobals.ptDeviceObject->Flags&=~DO_DEVICE_INITIALIZING;
	g_tDriverGlobals.ptDeviceObject->Flags|=DO_BUFFERED_IO;

	eStatus = STATUS_SUCCESS;

lblCleanup:
	if (!NT_SUCCESS(eStatus))
	{
		DriverUnload(ptDriverObject);
	}

	return eStatus;
}
