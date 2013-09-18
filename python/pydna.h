/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */
#ifndef PYDNA_H
#define PYDNA_H

#include "../dnausb/dnausb.h"
#include <Python.h>

#ifdef _WIN32
 #define PYDNA_API __declspec(dllexport)
#else
 #define PYDNA_API
#endif

//------------------------------------------------------------------------------
class PYDNA_API PyDNA
{
public:

	static PyObject* Init();
	
	static PyMethodDef m_methods[];

private:

	PyDNA() {}

	static PyObject* PyAbout( PyObject *self, PyObject *args );
	static PyObject* PyOpen( PyObject *self, PyObject *args );
	static PyObject* PyClose( PyObject *self, PyObject *args );
	static PyObject* PyFlashClient( PyObject *self, PyObject *args );

	static DNADEVICE m_device;
};

extern PYDNA_API PyDNA* pyDNA;

#endif

